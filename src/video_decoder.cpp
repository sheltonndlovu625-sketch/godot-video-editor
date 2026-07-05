#include "video_decoder.h"

using namespace godot;

void VideoDecoder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("open", "path"), &VideoDecoder::open);
    ClassDB::bind_method(D_METHOD("read_video_frame"), &VideoDecoder::read_video_frame);
    ClassDB::bind_method(D_METHOD("read_audio_samples", "max_samples"), &VideoDecoder::read_audio_samples);
    ClassDB::bind_method(D_METHOD("seek", "time_seconds"), &VideoDecoder::seek);
    ClassDB::bind_method(D_METHOD("get_duration"), &VideoDecoder::get_duration);
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

bool VideoDecoder::open(String p_path) {
    int ret = avformat_open_input(&format_ctx, p_path.utf8().get_data(), nullptr, nullptr);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Could not open input file");
        return false;
    }

    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Could not find stream info");
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

    video_codec_ctx = avcodec_alloc_context3(vcodec);
    if (!video_codec_ctx) {
        UtilityFunctions::push_error("[VideoDecoder] Could not allocate video codec context");
        avformat_close_input(&format_ctx);
        return false;
    }

    ret = avcodec_parameters_to_context(video_codec_ctx, vstream->codecpar);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Could not copy video codec params");
        close();
        return false;
    }

    ret = avcodec_open2(video_codec_ctx, vcodec, nullptr);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Could not open video codec");
        close();
        return false;
    }

    video_frame = av_frame_alloc();
    rgba_frame = av_frame_alloc();
    rgba_frame->format = AV_PIX_FMT_RGBA;
    rgba_frame->width = video_codec_ctx->width;
    rgba_frame->height = video_codec_ctx->height;
    ret = av_frame_get_buffer(rgba_frame, 0);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Could not allocate RGBA frame");
        close();
        return false;
    }

    sws_ctx = sws_getContext(
        video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt,
        video_codec_ctx->width, video_codec_ctx->height, AV_PIX_FMT_RGBA,
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
                    UtilityFunctions::push_warning("[VideoDecoder] Could not copy audio codec params");
                    avcodec_free_context(&audio_codec_ctx);
                    audio_stream_index = -1;
                } else {
                    ret = avcodec_open2(audio_codec_ctx, acodec, nullptr);
                    if (ret < 0) {
                        UtilityFunctions::push_warning("[VideoDecoder] Could not open audio codec");
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
                            av_channel_layout_uninit(&dst_ch_layout);

                            if (ret < 0 || swr_init(swr_ctx) < 0) {
                                UtilityFunctions::push_warning("[VideoDecoder] Could not init resampler");
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
    UtilityFunctions::print("[VideoDecoder] Opened: ", p_path, " (", video_codec_ctx->width, "x", video_codec_ctx->height, ")");
    return true;
}

Ref<Image> VideoDecoder::read_video_frame() {
    if (!initialized || video_stream_index < 0) {
        return Ref<Image>();
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
            if (ret < 0) {
                UtilityFunctions::push_error("[VideoDecoder] Error receiving video frame");
                break;
            }

            sws_scale(sws_ctx, video_frame->data, video_frame->linesize, 0,
                video_codec_ctx->height, rgba_frame->data, rgba_frame->linesize);

            int img_size = rgba_frame->linesize[0] * video_codec_ctx->height;
            PackedByteArray bytes;
            bytes.resize(img_size);
            memcpy(bytes.ptrw(), rgba_frame->data[0], img_size);

            result = Image::create_from_data(
                video_codec_ctx->width, video_codec_ctx->height,
                false, Image::FORMAT_RGBA8, bytes
            );
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
            sws_scale(sws_ctx, video_frame->data, video_frame->linesize, 0,
                video_codec_ctx->height, rgba_frame->data, rgba_frame->linesize);

            int img_size = rgba_frame->linesize[0] * video_codec_ctx->height;
            PackedByteArray bytes;
            bytes.resize(img_size);
            memcpy(bytes.ptrw(), rgba_frame->data[0], img_size);

            result = Image::create_from_data(
                video_codec_ctx->width, video_codec_ctx->height,
                false, Image::FORMAT_RGBA8, bytes
            );
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
                    // Drop video frame while reading audio
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

    int64_t ts = (int64_t)(p_time_seconds * AV_TIME_BASE);
    int ret = avformat_seek_file(format_ctx, -1, INT64_MIN, ts, INT64_MAX, 0);
    if (ret < 0) {
        ret = av_seek_frame(format_ctx, -1, ts, 0);
    }

    if (ret < 0) {
        UtilityFunctions::push_error("[VideoDecoder] Seek failed");
        return false;
    }

    if (video_codec_ctx) {
        avcodec_flush_buffers(video_codec_ctx);
    }
    if (audio_codec_ctx) {
        avcodec_flush_buffers(audio_codec_ctx);
    }
    audio_buffer.resize(0);

    return true;
}

double VideoDecoder::get_duration() const {
    return duration;
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
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }

    av_frame_free(&video_frame);
    av_frame_free(&rgba_frame);
    av_frame_free(&audio_frame);

    avcodec_free_context(&video_codec_ctx);
    avcodec_free_context(&audio_codec_ctx);

    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }

    initialized = false;
    video_stream_index = -1;
    audio_stream_index = -1;
    duration = 0.0;
    sample_rate = 0;
    channels = 0;

    UtilityFunctions::print("[VideoDecoder] Closed.");
}

bool VideoDecoder::is_open() const {
    return initialized;
}
