#include "video_decoder.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>

using namespace godot;

namespace {
    String resolve_path(String p_path) {
        if (p_path.begins_with("user://") || p_path.begins_with("res://")) {
            ProjectSettings *ps = ProjectSettings::get_singleton();
            if (ps) {
                return ps->globalize_path(p_path);
            }
        }
        return p_path;
    }

    void log_av_error(const char *prefix, int errnum) {
        char errbuf[256];
        av_strerror(errnum, errbuf, sizeof(errbuf));
        UtilityFunctions::push_error(String("[VideoDecoder] ") + prefix + ": " + errbuf);
    }
} // anonymous namespace

void VideoDecoder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open", "path"), &VideoDecoder::open);
    ClassDB::bind_method(D_METHOD("read_video_frame"), &VideoDecoder::read_video_frame);
    ClassDB::bind_method(D_METHOD("read_video_frame_scaled", "width", "height"), &VideoDecoder::read_video_frame_scaled);
    ClassDB::bind_method(D_METHOD("read_audio_samples", "max_samples"), &VideoDecoder::read_audio_samples);
    ClassDB::bind_method(D_METHOD("seek", "time_seconds"), &VideoDecoder::seek);
    ClassDB::bind_method(D_METHOD("get_duration"), &VideoDecoder::get_duration);
    ClassDB::bind_method(D_METHOD("get_current_time"), &VideoDecoder::get_current_time);
    ClassDB::bind_method(D_METHOD("get_video_width"), &VideoDecoder::get_video_width);
    ClassDB::bind_method(D_METHOD("get_video_height"), &VideoDecoder::get_video_height);
    ClassDB::bind_method(D_METHOD("get_video_fps"), &VideoDecoder::get_video_fps);
    ClassDB::bind_method(D_METHOD("has_audio"), &VideoDecoder::has_audio);
    ClassDB::bind_method(D_METHOD("close"), &VideoDecoder::close);
    ClassDB::bind_method(D_METHOD("is_open"), &VideoDecoder::is_open);
}

VideoDecoder::VideoDecoder() {}

VideoDecoder::~VideoDecoder() {
    if (initialized) {
        close();
    }
}

bool VideoDecoder::try_hwaccel(const AVCodec **p_codec, AVCodecContext *p_ctx) {
    #if defined(__ANDROID__)
        const AVCodec *hw_codec = avcodec_find_decoder_by_name("h264_mediacodec");
        if (!hw_codec) {
            hw_codec = avcodec_find_decoder_by_name("hevc_mediacodec");
        }
        if (hw_codec) {
            *p_codec = hw_codec;
            UtilityFunctions::print("[VideoDecoder] Using Android MediaCodec hardware decoder: ", hw_codec->name);
            return true;
        }
    #elif defined(__APPLE__)
        const AVCodec *hw_codec = avcodec_find_decoder_by_name("h264_videotoolbox");
        if (!hw_codec) {
            hw_codec = avcodec_find_decoder_by_name("hevc_videotoolbox");
        }
        if (hw_codec) {
            *p_codec = hw_codec;
            UtilityFunctions::print("[VideoDecoder] Using VideoToolbox hardware decoder: ", hw_codec->name);
            return true;
        }
    #endif
    return false;
}

bool VideoDecoder::open(String p_path) {
    String resolved_path = resolve_path(p_path);
    CharString path_utf8 = resolved_path.utf8();
    const char *path_cstr = path_utf8.get_data();

    int ret = avformat_open_input(&format_ctx, path_cstr, nullptr, nullptr);
    if (ret < 0) {
        log_av_error("avformat_open_input failed", ret);
        UtilityFunctions::push_error("[VideoDecoder] Could not open input file: ", resolved_path);
        return false;
    }

    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        log_av_error("Could not find stream info", ret);
        avformat_close_input(&format_ctx);
        return false;
    }

    if (format_ctx->duration != AV_NOPTS_VALUE) {
        duration = (double)format_ctx->duration / AV_TIME_BASE;
    }

    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        AVStream *stream = format_ctx->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_index < 0) {
            video_stream_index = i;
        } else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_index < 0) {
            audio_stream_index = i;
        }
    }

    if (video_stream_index < 0) {
        UtilityFunctions::push_error("[VideoDecoder] No video stream found");
        avformat_close_input(&format_ctx);
        return false;
    }

    AVStream *vstream = format_ctx->streams[video_stream_index];
    const AVCodec *vcodec = avcodec_find_decoder(vstream->codecpar->codec_id);
    if (!vcodec) {
        UtilityFunctions::push_error("[VideoDecoder] Video codec not found");
        avformat_close_input(&format_ctx);
        return false;
    }

    const AVCodec *hw_codec = nullptr;
    if (try_hwaccel(&hw_codec, video_codec_ctx)) {
        vcodec = hw_codec;
        use_hwaccel = true;
    }

    video_codec_ctx = avcodec_alloc_context3(vcodec);
    if (!video_codec_ctx) {
        UtilityFunctions::push_error("[VideoDecoder] Could not allocate video codec context");
        avformat_close_input(&format_ctx);
        return false;
    }

    ret = avcodec_parameters_to_context(video_codec_ctx, vstream->codecpar);
    if (ret < 0) {
        log_av_error("Could not copy video codec params", ret);
        close();
        return false;
    }

    ret = avcodec_open2(video_codec_ctx, vcodec, nullptr);
    if (ret < 0) {
        log_av_error("Could not open video codec", ret);
        close();
        return false;
    }

    original_width = video_codec_ctx->width;
    original_height = video_codec_ctx->height;

    video_frame = av_frame_alloc();
    hw_frame = av_frame_alloc();
    rgba_frame = av_frame_alloc();
    rgba_frame->format = AV_PIX_FMT_RGBA;
    rgba_frame->width = original_width;
    rgba_frame->height = original_height;
    ret = av_frame_get_buffer(rgba_frame, 0);
    if (ret < 0) {
        log_av_error("Could not allocate RGBA frame", ret);
        close();
        return false;
    }

    sws_ctx = sws_getContext(
        original_width, original_height, video_codec_ctx->pix_fmt,
        original_width, original_height, AV_PIX_FMT_RGBA,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        UtilityFunctions::push_error("[VideoDecoder] Could not create scaler context");
        close();
        return false;
    }

    if (audio_stream_index >= 0) {
        AVStream *astream = format_ctx->streams[audio_stream_index];
        const AVCodec *acodec = avcodec_find_decoder(astream->codecpar->codec_id);
        if (!acodec) {
            UtilityFunctions::push_warning("[VideoDecoder] Audio codec not found, continuing without audio");
            audio_stream_index = -1;
        } else {
            audio_codec_ctx = avcodec_alloc_context3(acodec);
            if (!audio_codec_ctx) {
                UtilityFunctions::push_warning("[VideoDecoder] Could not allocate audio codec context");
                audio_stream_index = -1;
            } else {
                ret = avcodec_parameters_to_context(audio_codec_ctx, astream->codecpar);
                if (ret < 0) {
                    log_av_error("Could not copy audio codec params", ret);
                    avcodec_free_context(&audio_codec_ctx);
                    audio_stream_index = -1;
                } else {
                    ret = avcodec_open2(audio_codec_ctx, acodec, nullptr);
                    if (ret < 0) {
                        log_av_error("Could not open audio codec", ret);
                        avcodec_free_context(&audio_codec_ctx);
                        audio_stream_index = -1;
                    } else {
                        sample_rate = audio_codec_ctx->sample_rate;
                        channels = audio_codec_ctx->ch_layout.nb_channels;

                        swr_ctx = swr_alloc();
                        if (!swr_ctx) {
                            UtilityFunctions::push_warning("[VideoDecoder] Could not allocate resampler");
                            avcodec_free_context(&audio_codec_ctx);
                            audio_stream_index = -1;
                        } else {
                            AVChannelLayout dst_ch_layout;
                            av_channel_layout_default(&dst_ch_layout, channels);

                            ret = swr_alloc_set_opts2(&swr_ctx,
                                &dst_ch_layout, AV_SAMPLE_FMT_FLT, sample_rate,
                                &audio_codec_ctx->ch_layout, audio_codec_ctx->sample_fmt, sample_rate,
                                0, nullptr);

                            if (ret < 0 || swr_init(swr_ctx) < 0) {
                                log_av_error("Could not init resampler", ret < 0 ? ret : AVERROR_UNKNOWN);
                                swr_free(&swr_ctx);
                                avcodec_free_context(&audio_codec_ctx);
                                audio_stream_index = -1;
                            } else {
                                audio_frame = av_frame_alloc();
                                UtilityFunctions::print("[VideoDecoder] Audio: ", channels, " channels @ ", sample_rate, " Hz");
                            }
                        }
                    }
                }
            }
        }
    }

    initialized = true;
    current_time = 0.0;
    UtilityFunctions::print("[VideoDecoder] Opened: ", resolved_path, " (", original_width, "x", original_height, ")");
    UtilityFunctions::print("[VideoDecoder] Video stream index: ", video_stream_index, " Audio stream index: ", audio_stream_index);
    UtilityFunctions::print("[VideoDecoder] Hardware acceleration: ", use_hwaccel ? "YES" : "NO");
    return true;
}

Ref<Image> VideoDecoder::read_video_frame() {
    if (!initialized || video_stream_index < 0) {
        return Ref<Image>();
    }

    uint64_t t0 = Time::get_singleton()->get_ticks_msec();
    int packet_count = 0;

    AVPacket *pkt = av_packet_alloc();
    Ref<Image> result;

    while (av_read_frame(format_ctx, pkt) >= 0) {
        packet_count++;
        if (pkt->stream_index == video_stream_index) {
            int ret = avcodec_send_packet(video_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) continue;

            ret = avcodec_receive_frame(video_codec_ctx, video_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            if (ret < 0) {
                log_av_error("Error receiving video frame", ret);
                break;
            }

            if (video_frame->pts != AV_NOPTS_VALUE) {
                AVStream *stream = format_ctx->streams[video_stream_index];
                current_time = video_frame->pts * av_q2d(stream->time_base);
            }

            AVFrame *frame_to_convert = video_frame;

            if (use_hwaccel && video_frame->format == AV_PIX_FMT_MEDIACODEC) {
                ret = av_hwframe_transfer_data(hw_frame, video_frame, 0);
                if (ret < 0) {
                    log_av_error("Hardware frame transfer failed", ret);
                    continue;
                }
                frame_to_convert = hw_frame;
            }

            uint64_t t_sws = Time::get_singleton()->get_ticks_msec();
            sws_scale(sws_ctx, frame_to_convert->data, frame_to_convert->linesize, 0,
                video_codec_ctx->height, rgba_frame->data, rgba_frame->linesize);
            uint64_t sws_ms = Time::get_singleton()->get_ticks_msec() - t_sws;

            // FIX: row-by-row copy to strip FFmpeg line padding
            PackedByteArray bytes;
            bytes.resize(video_codec_ctx->width * video_codec_ctx->height * 4);
            uint8_t *dst = bytes.ptrw();
            for (int y = 0; y < video_codec_ctx->height; y++) {
                memcpy(dst + y * video_codec_ctx->width * 4,
                       rgba_frame->data[0] + y * rgba_frame->linesize[0],
                       video_codec_ctx->width * 4);
            }

            result = Image::create_from_data(
                video_codec_ctx->width, video_codec_ctx->height,
                false, Image::FORMAT_RGBA8, bytes
            );

            uint64_t total_ms = Time::get_singleton()->get_ticks_msec() - t0;
            UtilityFunctions::print("[VideoDecoder] read_video_frame: ", packet_count, " packets, sws=", sws_ms, "ms, total=", total_ms, "ms");
            break;
        } else if (pkt->stream_index == audio_stream_index && audio_codec_ctx) {
            int ret = avcodec_send_packet(audio_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) continue;

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;

                int out_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);
                if (out_samples > 0) {
                    uint8_t **dst_data = nullptr;
                    int dst_linesize;
                    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, channels,
                        out_samples, AV_SAMPLE_FMT_FLT, 0);

                    int converted = swr_convert(swr_ctx, dst_data, out_samples,
                        (const uint8_t **)audio_frame->data, audio_frame->nb_samples);

                    if (converted > 0) {
                        float *float_data = (float *)dst_data[0];
                        int old_size = audio_buffer.size();
                        audio_buffer.resize(old_size + converted * channels);
                        memcpy(audio_buffer.ptrw() + old_size, float_data, converted * channels * sizeof(float));
                    }

                    av_freep(&dst_data[0]);
                    av_freep(&dst_data);
                }
            }
        } else {
            av_packet_unref(pkt);
        }
    }

    if (result.is_null()) {
        avcodec_send_packet(video_codec_ctx, nullptr);
        int ret = avcodec_receive_frame(video_codec_ctx, video_frame);
        if (ret >= 0) {
            if (video_frame->pts != AV_NOPTS_VALUE) {
                AVStream *stream = format_ctx->streams[video_stream_index];
                current_time = video_frame->pts * av_q2d(stream->time_base);
            }

            AVFrame *frame_to_convert = video_frame;
            if (use_hwaccel && video_frame->format == AV_PIX_FMT_MEDIACODEC) {
                ret = av_hwframe_transfer_data(hw_frame, video_frame, 0);
                if (ret >= 0) {
                    frame_to_convert = hw_frame;
                }
            }

            sws_scale(sws_ctx, frame_to_convert->data, frame_to_convert->linesize, 0,
                video_codec_ctx->height, rgba_frame->data, rgba_frame->linesize);

            // FIX: row-by-row copy to strip FFmpeg line padding
            PackedByteArray bytes;
            bytes.resize(video_codec_ctx->width * video_codec_ctx->height * 4);
            uint8_t *dst = bytes.ptrw();
            for (int y = 0; y < video_codec_ctx->height; y++) {
                memcpy(dst + y * video_codec_ctx->width * 4,
                       rgba_frame->data[0] + y * rgba_frame->linesize[0],
                       video_codec_ctx->width * 4);
            }

            result = Image::create_from_data(
                video_codec_ctx->width, video_codec_ctx->height,
                false, Image::FORMAT_RGBA8, bytes
            );
        }
    }

    av_packet_free(&pkt);
    uint64_t total_ms = Time::get_singleton()->get_ticks_msec() - t0;
    UtilityFunctions::print("[VideoDecoder] read_video_frame EXIT: total=", total_ms, "ms, packets=", packet_count);
    return result;
}

Ref<Image> VideoDecoder::read_video_frame_scaled(int p_width, int p_height) {
    if (!initialized || video_stream_index < 0) {
        return Ref<Image>();
    }

    if (p_width == original_width && p_height == original_height) {
        return read_video_frame();
    }

    if (p_width != scaled_width || p_height != scaled_height || !sws_ctx_scaled) {
        if (sws_ctx_scaled) {
            sws_freeContext(sws_ctx_scaled);
        }
        if (scaled_rgba_frame) {
            av_frame_free(&scaled_rgba_frame);
        }

        scaled_rgba_frame = av_frame_alloc();
        scaled_rgba_frame->format = AV_PIX_FMT_RGBA;
        scaled_rgba_frame->width = p_width;
        scaled_rgba_frame->height = p_height;
        int ret = av_frame_get_buffer(scaled_rgba_frame, 0);
        if (ret < 0) {
            log_av_error("Could not allocate scaled RGBA frame", ret);
            return read_video_frame();
        }

        sws_ctx_scaled = sws_getContext(
            original_width, original_height, video_codec_ctx->pix_fmt,
            p_width, p_height, AV_PIX_FMT_RGBA,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        );
        if (!sws_ctx_scaled) {
            UtilityFunctions::push_error("[VideoDecoder] Could not create scaled scaler context");
            return read_video_frame();
        }

        scaled_width = p_width;
        scaled_height = p_height;
    }

    AVPacket *pkt = av_packet_alloc();
    Ref<Image> result;

    while (av_read_frame(format_ctx, pkt) >= 0) {
        if (pkt->stream_index == video_stream_index) {
            int ret = avcodec_send_packet(video_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) continue;

            ret = avcodec_receive_frame(video_codec_ctx, video_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) continue;
            if (ret < 0) break;

            if (video_frame->pts != AV_NOPTS_VALUE) {
                AVStream *stream = format_ctx->streams[video_stream_index];
                current_time = video_frame->pts * av_q2d(stream->time_base);
            }

            AVFrame *frame_to_convert = video_frame;
            if (use_hwaccel && video_frame->format == AV_PIX_FMT_MEDIACODEC) {
                ret = av_hwframe_transfer_data(hw_frame, video_frame, 0);
                if (ret >= 0) {
                    frame_to_convert = hw_frame;
                }
            }

            sws_scale(sws_ctx_scaled, frame_to_convert->data, frame_to_convert->linesize, 0,
                video_codec_ctx->height, scaled_rgba_frame->data, scaled_rgba_frame->linesize);

            // FIX: row-by-row copy to strip FFmpeg line padding
            PackedByteArray bytes;
            bytes.resize(p_width * p_height * 4);
            uint8_t *dst = bytes.ptrw();
            for (int y = 0; y < p_height; y++) {
                memcpy(dst + y * p_width * 4,
                       scaled_rgba_frame->data[0] + y * scaled_rgba_frame->linesize[0],
                       p_width * 4);
            }

            result = Image::create_from_data(p_width, p_height, false, Image::FORMAT_RGBA8, bytes);
            break;
        } else if (pkt->stream_index == audio_stream_index && audio_codec_ctx) {
            int ret = avcodec_send_packet(audio_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) continue;
            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;
                int out_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);
                if (out_samples > 0) {
                    uint8_t **dst_data = nullptr;
                    int dst_linesize;
                    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, channels,
                        out_samples, AV_SAMPLE_FMT_FLT, 0);
                    int converted = swr_convert(swr_ctx, dst_data, out_samples,
                        (const uint8_t **)audio_frame->data, audio_frame->nb_samples);
                    if (converted > 0) {
                        float *float_data = (float *)dst_data[0];
                        int old_size = audio_buffer.size();
                        audio_buffer.resize(old_size + converted * channels);
                        memcpy(audio_buffer.ptrw() + old_size, float_data, converted * channels * sizeof(float));
                    }
                    av_freep(&dst_data[0]);
                    av_freep(&dst_data);
                }
            }
        } else {
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);
    return result;
}

PackedFloat32Array VideoDecoder::read_audio_samples(int p_max_samples) {
    PackedFloat32Array result;

    if (!initialized || audio_stream_index < 0 || !audio_codec_ctx) {
        return result;
    }

    if (audio_buffer.size() > 0) {
        int to_return = p_max_samples < audio_buffer.size() ? p_max_samples : audio_buffer.size();
        result.resize(to_return);
        memcpy(result.ptrw(), audio_buffer.ptr(), to_return * sizeof(float));

        if (to_return < audio_buffer.size()) {
            memmove(audio_buffer.ptrw(), audio_buffer.ptr() + to_return,
                (audio_buffer.size() - to_return) * sizeof(float));
            audio_buffer.resize(audio_buffer.size() - to_return);
        } else {
            audio_buffer.resize(0);
        }

        if (to_return == p_max_samples) {
            return result;
        }
        p_max_samples -= to_return;
    }

    AVPacket *pkt = av_packet_alloc();
    while (audio_buffer.size() < p_max_samples && av_read_frame(format_ctx, pkt) >= 0) {
        if (pkt->stream_index == audio_stream_index) {
            int ret = avcodec_send_packet(audio_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret < 0) continue;

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                if (ret < 0) break;

                int out_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);
                if (out_samples > 0) {
                    uint8_t **dst_data = nullptr;
                    int dst_linesize;
                    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, channels,
                        out_samples, AV_SAMPLE_FMT_FLT, 0);

                    int converted = swr_convert(swr_ctx, dst_data, out_samples,
                        (const uint8_t **)audio_frame->data, audio_frame->nb_samples);

                    if (converted > 0) {
                        float *float_data = (float *)dst_data[0];
                        int old_size = audio_buffer.size();
                        audio_buffer.resize(old_size + converted * channels);
                        memcpy(audio_buffer.ptrw() + old_size, float_data, converted * channels * sizeof(float));
                    }

                    av_freep(&dst_data[0]);
                    av_freep(&dst_data);
                }
            }
        } else if (pkt->stream_index == video_stream_index) {
            int ret = avcodec_send_packet(video_codec_ctx, pkt);
            av_packet_unref(pkt);
            if (ret >= 0) {
                while (avcodec_receive_frame(video_codec_ctx, video_frame) >= 0) {
                    if (video_frame->pts != AV_NOPTS_VALUE) {
                        AVStream *stream = format_ctx->streams[video_stream_index];
                        current_time = video_frame->pts * av_q2d(stream->time_base);
                    }
                }
            }
        } else {
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);

    if (audio_buffer.size() > 0) {
        int to_return = p_max_samples < audio_buffer.size() ? p_max_samples : audio_buffer.size();
        int result_offset = result.size();
        result.resize(result_offset + to_return);
        memcpy(result.ptrw() + result_offset, audio_buffer.ptr(), to_return * sizeof(float));

        if (to_return < audio_buffer.size()) {
            memmove(audio_buffer.ptrw(), audio_buffer.ptr() + to_return,
                (audio_buffer.size() - to_return) * sizeof(float));
            audio_buffer.resize(audio_buffer.size() - to_return);
        } else {
            audio_buffer.resize(0);
        }
    }

    return result;
}

bool VideoDecoder::seek(double p_time_seconds) {
    if (!initialized) {
        return false;
    }

    uint64_t t0 = Time::get_singleton()->get_ticks_msec();

    int64_t ts = (int64_t)(p_time_seconds * AV_TIME_BASE);
    int ret = avformat_seek_file(format_ctx, -1, INT64_MIN, ts, INT64_MAX, 0);
    if (ret < 0) {
        ret = av_seek_frame(format_ctx, -1, ts, 0);
    }

    uint64_t seek_ms = Time::get_singleton()->get_ticks_msec() - t0;

    if (ret < 0) {
        log_av_error("Seek failed", ret);
        return false;
    }

    if (video_codec_ctx) {
        avcodec_flush_buffers(video_codec_ctx);
    }
    if (audio_codec_ctx) {
        avcodec_flush_buffers(audio_codec_ctx);
    }
    audio_buffer.resize(0);
    current_time = p_time_seconds;

    UtilityFunctions::print("[VideoDecoder] seek to ", p_time_seconds, " took ", seek_ms, " ms");
    return true;
}

double VideoDecoder::get_duration() const {
    return duration;
}

double VideoDecoder::get_current_time() const {
    return current_time;
}

int VideoDecoder::get_video_width() const {
    if (!initialized || !video_codec_ctx) return 0;
    return video_codec_ctx->width;
}

int VideoDecoder::get_video_height() const {
    if (!initialized || !video_codec_ctx) return 0;
    return video_codec_ctx->height;
}

double VideoDecoder::get_video_fps() const {
    if (!initialized || video_stream_index < 0 || !format_ctx) return 0.0;
    AVStream *stream = format_ctx->streams[video_stream_index];
    if (stream->avg_frame_rate.den != 0) {
        return av_q2d(stream->avg_frame_rate);
    }
    if (stream->r_frame_rate.den != 0) {
        return av_q2d(stream->r_frame_rate);
    }
    if (stream->time_base.den != 0) {
        return 1.0 / av_q2d(stream->time_base);
    }
    return 0.0;
}

bool VideoDecoder::has_audio() const {
    return initialized && audio_stream_index >= 0 && audio_codec_ctx != nullptr;
}

void VideoDecoder::close() {
    if (!initialized) {
        return;
    }

    audio_buffer.resize(0);

    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (sws_ctx_scaled) {
        sws_freeContext(sws_ctx_scaled);
        sws_ctx_scaled = nullptr;
    }
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }

    av_frame_free(&video_frame);
    av_frame_free(&hw_frame);
    av_frame_free(&rgba_frame);
    av_frame_free(&scaled_rgba_frame);
    av_frame_free(&audio_frame);

    avcodec_free_context(&video_codec_ctx);
    avcodec_free_context(&audio_codec_ctx);

    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }

    initialized = false;
    use_hwaccel = false;
    hw_device_type = AV_HWDEVICE_TYPE_NONE;
    video_stream_index = -1;
    audio_stream_index = -1;
    duration = 0.0;
    current_time = 0.0;
    sample_rate = 0;
    channels = 0;
    scaled_width = 0;
    scaled_height = 0;

    UtilityFunctions::print("[VideoDecoder] Closed.");
}

bool VideoDecoder::is_open() const {
    return initialized;
}

// ------------------------------------------------------------------
// Belt-and-suspenders: ensure typeinfo for VideoDecoder is emitted
// ------------------------------------------------------------------
#include <typeinfo>
namespace godot {
    struct VideoDecoderTypeinfoHelper {
        VideoDecoderTypeinfoHelper() {
            const std::type_info &ti = typeid(VideoDecoder);
            (void)ti;
        }
    };
    static VideoDecoderTypeinfoHelper _video_decoder_typeinfo_helper;
}
