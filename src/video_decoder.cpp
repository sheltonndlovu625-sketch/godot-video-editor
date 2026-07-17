#include "video_decoder.h"
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace {
    String resolve_path(String p_path) {
        if (p_path.begins_with("user://") || p_path.begins_with("res://")) {
            if (ProjectSettings *ps = ProjectSettings::get_singleton()) {
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

    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                            const enum AVPixelFormat *pix_fmts) {
        const enum AVPixelFormat *p;
        for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == AV_PIX_FMT_MEDIACODEC) return *p;
            if (*p == AV_PIX_FMT_VIDEOTOOLBOX) return *p;
        }
        return pix_fmts[0];
    }
}

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
VideoDecoder::~VideoDecoder() { if (initialized) close(); }

void VideoDecoder::_queue_packet(AVPacket *p_packet) {
    AVPacket *copy = av_packet_alloc();
    if (!copy) return;
    if (av_packet_ref(copy, p_packet) < 0) {
        av_packet_free(&copy);
        return;
    }

    if (p_packet->stream_index == video_stream_index) {
        video_packet_queue.push_back(copy);
    } else if (p_packet->stream_index == audio_stream_index) {
        audio_packet_queue.push_back(copy);
    } else {
        av_packet_unref(copy);
        av_packet_free(&copy);
    }
}

void VideoDecoder::_flush_packet_queues() {
    for (int i = 0; i < video_packet_queue.size(); i++) {
        av_packet_free(&video_packet_queue[i]);
    }
    video_packet_queue.clear();
    for (int i = 0; i < audio_packet_queue.size(); i++) {
        av_packet_free(&audio_packet_queue[i]);
    }
    audio_packet_queue.clear();
}

bool VideoDecoder::open(String p_path) {
    String resolved_path = resolve_path(p_path);
    CharString path_utf8 = resolved_path.utf8();
    const char *path_cstr = path_utf8.get_data();

    int ret = avformat_open_input(&format_ctx, path_cstr, nullptr, nullptr);
    if (ret < 0) {
        log_av_error("avformat_open_input failed", ret);
        return false;
    }

    ret = avformat_find_stream_info(format_ctx, nullptr);
    if (ret < 0) {
        avformat_close_input(&format_ctx);
        return false;
    }

    if (format_ctx->duration != AV_NOPTS_VALUE) {
        duration = static_cast<double>(format_ctx->duration) / AV_TIME_BASE;
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
        avformat_close_input(&format_ctx);
        return false;
    }

    AVStream *vstream = format_ctx->streams[video_stream_index];
    const AVCodec *vcodec = avcodec_find_decoder(vstream->codecpar->codec_id);
    if (!vcodec) {
        avformat_close_input(&format_ctx);
        return false;
    }

    const AVCodec *hw_codec = nullptr;
#if defined(__ANDROID__)
    hw_codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (!hw_codec) hw_codec = avcodec_find_decoder_by_name("hevc_mediacodec");
#elif defined(__APPLE__)
    hw_codec = avcodec_find_decoder_by_name("h264_videotoolbox");
    if (!hw_codec) hw_codec = avcodec_find_decoder_by_name("hevc_videotoolbox");
#endif

    bool hw_opened = false;
    if (hw_codec) {
        video_codec_ctx = avcodec_alloc_context3(hw_codec);
        if (video_codec_ctx) {
            ret = avcodec_parameters_to_context(video_codec_ctx, vstream->codecpar);
            if (ret >= 0) {
                video_codec_ctx->get_format = get_hw_format;
                ret = avcodec_open2(video_codec_ctx, hw_codec, nullptr);
                if (ret >= 0) {
                    hw_opened = true;
                    use_hwaccel = true;
                    hw_pix_fmt = video_codec_ctx->pix_fmt;
                }
            }
            if (!hw_opened) {
                avcodec_free_context(&video_codec_ctx);
            }
        }
    }

    if (!hw_opened) {
        use_hwaccel = false;
        hw_pix_fmt = AV_PIX_FMT_NONE;
        video_codec_ctx = avcodec_alloc_context3(vcodec);
        if (!video_codec_ctx) {
            avformat_close_input(&format_ctx);
            return false;
        }

        int cpu_count = OS::get_singleton()->get_processor_count();
        video_codec_ctx->thread_count = (cpu_count > 0) ? cpu_count : 4;
        video_codec_ctx->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

        ret = avcodec_parameters_to_context(video_codec_ctx, vstream->codecpar);
        if (ret < 0) {
            close();
            return false;
        }

        ret = avcodec_open2(video_codec_ctx, vcodec, nullptr);
        if (ret < 0) {
            close();
            return false;
        }
    }

    original_width = video_codec_ctx->width;
    original_height = video_codec_ctx->height;

    video_frame = av_frame_alloc();
    hw_frame = av_frame_alloc();

    for (int i = 0; i < 2; i++) {
        native_buffers[i] = Image::create(original_width, original_height, false, Image::FORMAT_RGBA8);
    }
    native_write_idx = 0;

    packet = av_packet_alloc();

    if (audio_stream_index >= 0) {
        AVStream *astream = format_ctx->streams[audio_stream_index];
        const AVCodec *acodec = avcodec_find_decoder(astream->codecpar->codec_id);
        if (acodec) {
            audio_codec_ctx = avcodec_alloc_context3(acodec);
            if (audio_codec_ctx) {
                ret = avcodec_parameters_to_context(audio_codec_ctx, astream->codecpar);
                if (ret >= 0) {
                    ret = avcodec_open2(audio_codec_ctx, acodec, nullptr);
                    if (ret >= 0) {
                        sample_rate = audio_codec_ctx->sample_rate;
                        channels = audio_codec_ctx->ch_layout.nb_channels;
                        swr_ctx = swr_alloc();
                        if (swr_ctx) {
                            AVChannelLayout dst_ch_layout;
                            av_channel_layout_default(&dst_ch_layout, channels);
                            ret = swr_alloc_set_opts2(&swr_ctx,
                                &dst_ch_layout, AV_SAMPLE_FMT_FLT, sample_rate,
                                &audio_codec_ctx->ch_layout, audio_codec_ctx->sample_fmt, sample_rate,
                                0, nullptr);
                            if (ret >= 0 && swr_init(swr_ctx) >= 0) {
                                audio_frame = av_frame_alloc();
                            }
                        }
                    }
                }
            }
        }
    }

    initialized = true;
    current_time = 0.0;
    eof_reached = false;
    return true;
}

Ref<Image> VideoDecoder::_decode_one_frame(int p_width, int p_height,
                                           SwsContext *&r_sws,
                                           enum AVPixelFormat &r_last_src_fmt,
                                           Ref<Image> p_target) {
    Ref<Image> result;
    int ret;

    while (true) {
        ret = avcodec_receive_frame(video_codec_ctx, video_frame);
        if (ret == 0) {
            break;
        }
        if (ret == AVERROR_EOF) {
            eof_reached = true;
            return result;
        }
        if (ret != AVERROR(EAGAIN)) {
            log_av_error("avcodec_receive_frame failed", ret);
            return result;
        }

        AVPacket *pkt_to_send = nullptr;
        bool from_queue = false;

        if (!video_packet_queue.is_empty()) {
            pkt_to_send = video_packet_queue[0];
            video_packet_queue.remove_at(0);
            from_queue = true;
        } else {
            ret = av_read_frame(format_ctx, packet);
            if (ret < 0) {
                if (ret == AVERROR_EOF && !eof_reached) {
                    eof_reached = true;
                    avcodec_send_packet(video_codec_ctx, nullptr);
                    continue;
                }
                return result;
            }

            if (packet->stream_index != video_stream_index) {
                _queue_packet(packet);
                av_packet_unref(packet);
                continue;
            }
            pkt_to_send = packet;
        }

        ret = avcodec_send_packet(video_codec_ctx, pkt_to_send);
        if (ret == AVERROR(EAGAIN)) {
            if (from_queue) {
                video_packet_queue.insert(0, pkt_to_send);
            } else {
                _queue_packet(pkt_to_send);
                av_packet_unref(pkt_to_send);
            }
            continue;
        }

        if (from_queue) {
            av_packet_free(&pkt_to_send);
        } else {
            av_packet_unref(pkt_to_send);
        }

        if (ret < 0) {
            continue;
        }
    }

    if (video_frame->pts != AV_NOPTS_VALUE) {
        AVStream *stream = format_ctx->streams[video_stream_index];
        current_time = video_frame->pts * av_q2d(stream->time_base);
    }

    AVFrame *frame_to_convert = video_frame;
    if (use_hwaccel && video_frame->format == hw_pix_fmt) {
        ret = av_hwframe_transfer_data(hw_frame, video_frame, 0);
        if (ret >= 0) {
            frame_to_convert = hw_frame;
        } else {
            log_av_error("HW transfer failed, using original frame", ret);
        }
    }

    enum AVPixelFormat src_fmt = static_cast<enum AVPixelFormat>(frame_to_convert->format);
    if (!r_sws || r_last_src_fmt != src_fmt) {
        if (r_sws) {
            sws_freeContext(r_sws);
            r_sws = nullptr;
        }
        r_last_src_fmt = src_fmt;
    }

    if (!r_sws) {
        r_sws = sws_getContext(
            original_width, original_height, src_fmt,
            p_width, p_height, AV_PIX_FMT_RGBA,
            SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
        );
        if (!r_sws) {
            UtilityFunctions::push_error("[VideoDecoder] sws_getContext failed");
            return result;
        }
    }

    uint8_t *dst_data[1] = { p_target->ptrw() };
    int dst_linesize[1] = { p_width * 4 };

    sws_scale(r_sws, frame_to_convert->data, frame_to_convert->linesize, 0,
              original_height, dst_data, dst_linesize);

    return p_target;
}

Ref<Image> VideoDecoder::read_video_frame() {
    if (!initialized || video_stream_index < 0) return Ref<Image>();

    if (eof_reached) seek(0.0);

    Ref<Image> target = native_buffers[native_write_idx];
    Ref<Image> result = _decode_one_frame(original_width, original_height,
                                          sws_ctx, sws_src_fmt, target);
    if (result.is_valid()) {
        native_write_idx = 1 - native_write_idx;
    }
    return result;
}

Ref<Image> VideoDecoder::read_video_frame_scaled(int p_width, int p_height) {
    if (!initialized || video_stream_index < 0) return Ref<Image>();

    if (p_width == original_width && p_height == original_height) {
        return read_video_frame();
    }

    if (p_width != scaled_buf_w || p_height != scaled_buf_h || !sws_ctx_scaled) {
        if (sws_ctx_scaled) {
            sws_freeContext(sws_ctx_scaled);
            sws_ctx_scaled = nullptr;
        }
        for (int i = 0; i < 2; i++) {
            scaled_buffers[i] = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        }
        scaled_buf_w = p_width;
        scaled_buf_h = p_height;
        scaled_write_idx = 0;
        scaled_width = p_width;
        scaled_height = p_height;
        sws_scaled_src_fmt = AV_PIX_FMT_NONE;
    }

    if (eof_reached) seek(0.0);

    Ref<Image> target = scaled_buffers[scaled_write_idx];
    Ref<Image> result = _decode_one_frame(p_width, p_height,
                                          sws_ctx_scaled, sws_scaled_src_fmt, target);
    if (result.is_valid()) {
        scaled_write_idx = 1 - scaled_write_idx;
    }
    return result;
}

PackedFloat32Array VideoDecoder::read_audio_samples(int p_max_samples) {
    PackedFloat32Array result;
    if (!initialized || audio_stream_index < 0 || !audio_codec_ctx) return result;

    if (audio_buffer.size() > 0) {
        int to_return = MIN(p_max_samples, audio_buffer.size());
        result.resize(to_return);
        memcpy(result.ptrw(), audio_buffer.ptr(), to_return * sizeof(float));
        if (to_return < audio_buffer.size()) {
            memmove(audio_buffer.ptrw(), audio_buffer.ptr() + to_return,
                    (audio_buffer.size() - to_return) * sizeof(float));
            audio_buffer.resize(audio_buffer.size() - to_return);
        } else {
            audio_buffer.resize(0);
        }
        if (to_return == p_max_samples) return result;
        p_max_samples -= to_return;
    }

    bool file_eof = false;
    bool decoder_eof = false;

    while (audio_buffer.size() < p_max_samples && !decoder_eof) {
        int ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);

        if (ret == 0) {
            int out_samples = swr_get_out_samples(swr_ctx, audio_frame->nb_samples);
            if (out_samples > 0) {
                if (out_samples > audio_convert_buf_samples) {
                    av_freep((void**)&audio_convert_buf);
                    av_samples_alloc((uint8_t**)&audio_convert_buf, nullptr, channels,
                                     out_samples, AV_SAMPLE_FMT_FLT, 0);
                    audio_convert_buf_samples = out_samples;
                }
                int converted = swr_convert(swr_ctx, (uint8_t**)&audio_convert_buf, out_samples,
                                            (const uint8_t **)audio_frame->data, audio_frame->nb_samples);
                if (converted > 0) {
                    int old_size = audio_buffer.size();
                    audio_buffer.resize(old_size + converted * channels);
                    memcpy(audio_buffer.ptrw() + old_size, audio_convert_buf,
                           converted * channels * sizeof(float));
                }
            }
            continue;
        }

        if (ret == AVERROR_EOF) {
            decoder_eof = true;
            break;
        }

        if (ret != AVERROR(EAGAIN)) {
            log_av_error("avcodec_receive_frame (audio) failed", ret);
            break;
        }

        AVPacket *pkt_to_send = nullptr;
        bool from_queue = false;

        if (!audio_packet_queue.is_empty()) {
            pkt_to_send = audio_packet_queue[0];
            audio_packet_queue.remove_at(0);
            from_queue = true;
        } else if (!file_eof) {
            ret = av_read_frame(format_ctx, packet);
            if (ret < 0) {
                if (ret == AVERROR_EOF) {
                    file_eof = true;
                    avcodec_send_packet(audio_codec_ctx, nullptr);
                } else {
                    log_av_error("av_read_frame (audio) failed", ret);
                }
                continue;
            }

            if (packet->stream_index == audio_stream_index) {
                pkt_to_send = packet;
            } else {
                _queue_packet(packet);
                av_packet_unref(packet);
                continue;
            }
        } else {
            continue;
        }

        ret = avcodec_send_packet(audio_codec_ctx, pkt_to_send);
        if (ret == AVERROR(EAGAIN)) {
            if (from_queue) {
                audio_packet_queue.insert(0, pkt_to_send);
            } else {
                _queue_packet(pkt_to_send);
                av_packet_unref(pkt_to_send);
            }
            continue;
        }

        if (from_queue) {
            av_packet_free(&pkt_to_send);
        } else {
            av_packet_unref(pkt_to_send);
        }

        if (ret < 0) {
            continue;
        }
    }

    if (audio_buffer.size() > 0) {
        int to_return = MIN(p_max_samples, audio_buffer.size());
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
    if (!initialized) return false;

    _flush_packet_queues();

    int64_t ts = static_cast<int64_t>(p_time_seconds * AV_TIME_BASE);
    int ret = avformat_seek_file(format_ctx, -1, INT64_MIN, ts, INT64_MAX, 0);
    if (ret < 0) {
        ret = av_seek_frame(format_ctx, -1, ts, 0);
    }
    if (ret < 0) {
        log_av_error("Seek failed", ret);
        return false;
    }

    if (video_codec_ctx) avcodec_flush_buffers(video_codec_ctx);
    if (audio_codec_ctx) avcodec_flush_buffers(audio_codec_ctx);
    audio_buffer.resize(0);
    eof_reached = false;
    return true;
}

double VideoDecoder::get_duration() const { return duration; }
double VideoDecoder::get_current_time() const { return current_time; }

int VideoDecoder::get_video_width() const {
    return initialized && video_codec_ctx ? video_codec_ctx->width : 0;
}

int VideoDecoder::get_video_height() const {
    return initialized && video_codec_ctx ? video_codec_ctx->height : 0;
}

double VideoDecoder::get_video_fps() const {
    if (!initialized || video_stream_index < 0 || !format_ctx) return 0.0;
    AVStream *stream = format_ctx->streams[video_stream_index];
    if (stream->avg_frame_rate.den != 0) return av_q2d(stream->avg_frame_rate);
    if (stream->r_frame_rate.den != 0) return av_q2d(stream->r_frame_rate);
    if (stream->time_base.den != 0) return 1.0 / av_q2d(stream->time_base);
    return 0.0;
}

bool VideoDecoder::has_audio() const {
    return initialized && audio_stream_index >= 0 && audio_codec_ctx != nullptr;
}

void VideoDecoder::close() {
    if (!initialized) return;

    _flush_packet_queues();
    audio_buffer.resize(0);

    if (sws_ctx) { sws_freeContext(sws_ctx); sws_ctx = nullptr; }
    if (sws_ctx_scaled) { sws_freeContext(sws_ctx_scaled); sws_ctx_scaled = nullptr; }
    if (swr_ctx) { swr_free(&swr_ctx); }

    av_frame_free(&video_frame);
    av_frame_free(&hw_frame);
    av_frame_free(&audio_frame);

    avcodec_free_context(&video_codec_ctx);
    avcodec_free_context(&audio_codec_ctx);

    if (format_ctx) {
        avformat_close_input(&format_ctx);
    }

    av_packet_free(&packet);
    av_freep((void**)&audio_convert_buf);
    audio_convert_buf_samples = 0;

    for (int i = 0; i < 2; i++) {
        native_buffers[i] = Ref<Image>();
        scaled_buffers[i] = Ref<Image>();
    }
    native_write_idx = 0;
    scaled_write_idx = 0;
    scaled_buf_w = 0;
    scaled_buf_h = 0;
    sws_src_fmt = AV_PIX_FMT_NONE;
    sws_scaled_src_fmt = AV_PIX_FMT_NONE;

    initialized = false;
    use_hwaccel = false;
    hw_pix_fmt = AV_PIX_FMT_NONE;
    video_stream_index = -1;
    audio_stream_index = -1;
    duration = 0.0;
    current_time = 0.0;
    sample_rate = 0;
    channels = 0;
    scaled_width = 0;
    scaled_height = 0;
    eof_reached = false;
}

bool VideoDecoder::is_open() const { return initialized; }
