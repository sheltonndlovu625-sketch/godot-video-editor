#include "video_encoder.h"
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

static String resolve_path(String p_path) {
    if (p_path.begins_with("user://") || p_path.begins_with("res://")) {
        ProjectSettings *ps = ProjectSettings::get_singleton();
        if (ps) {
            return ps->globalize_path(p_path);
        }
    }
    return p_path;
}

static void log_av_error(const char *prefix, int errnum) {
    char errbuf[256];
    av_strerror(errnum, errbuf, sizeof(errbuf));
    UtilityFunctions::push_error(String("[VideoEncoder] ") + prefix + ": " + errbuf);
}

void VideoEncoder::_bind_methods() {
    ClassDB::bind_method(
        D_METHOD("open", "path", "width", "height", "fps", "bitrate"),
        &VideoEncoder::open
    );
    ClassDB::bind_method(
        D_METHOD("open_with_audio", "path", "width", "height", "fps", "video_bitrate", "sample_rate", "channels", "audio_bitrate"),
        &VideoEncoder::open_with_audio
    );
    ClassDB::bind_method(
        D_METHOD("write_frame", "image"),
        &VideoEncoder::write_frame
    );
    ClassDB::bind_method(
        D_METHOD("write_audio", "samples"),
        &VideoEncoder::write_audio
    );
    ClassDB::bind_method(
        D_METHOD("close"),
        &VideoEncoder::close
    );
    ClassDB::bind_method(
        D_METHOD("is_open"),
        &VideoEncoder::is_open
    );
}

VideoEncoder::VideoEncoder() {}

VideoEncoder::~VideoEncoder() {
    if (initialized) {
        close();
    }
}

bool VideoEncoder::open(String p_path, int p_width, int p_height, int p_fps, int p_bitrate) {
    return open_with_audio(p_path, p_width, p_height, p_fps, p_bitrate, 0, 0, 0);
}

bool VideoEncoder::open_with_audio(String p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_channels, int p_audio_bitrate) {
    width = p_width;
    height = p_height;
    fps = p_fps;

    if (p_sample_rate > 0) {
        has_audio = true;
        sample_rate = p_sample_rate;
        channels = p_channels;
        audio_bitrate = p_audio_bitrate;
    }

    String resolved_path = resolve_path(p_path);
    const char *path_utf8 = resolved_path.utf8().get_data();

    avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, path_utf8);
    if (!format_ctx) {
        avformat_alloc_output_context2(&format_ctx, nullptr, "mp4", path_utf8);
    }
    if (!format_ctx) {
        log_av_error("avformat_alloc_output_context2 failed", AVERROR(EINVAL));
        UtilityFunctions::push_error("[VideoEncoder] Could not allocate output context. Ensure linker uses --whole-archive for FFmpeg static libs.");
        return false;
    }

    const AVCodec *video_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!video_codec) {
        UtilityFunctions::push_error("[VideoEncoder] MPEG-4 encoder not found");
        return false;
    }

    video_stream = avformat_new_stream(format_ctx, nullptr);
    if (!video_stream) {
        UtilityFunctions::push_error("[VideoEncoder] Could not create video stream");
        return false;
    }
    video_stream->id = format_ctx->nb_streams - 1;

    video_codec_ctx = avcodec_alloc_context3(video_codec);
    if (!video_codec_ctx) {
        UtilityFunctions::push_error("[VideoEncoder] Could not allocate video codec context");
        return false;
    }

    video_codec_ctx->width = width;
    video_codec_ctx->height = height;
    video_codec_ctx->time_base = (AVRational){1, fps};
    video_codec_ctx->framerate = (AVRational){fps, 1};
    video_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    video_codec_ctx->bit_rate = p_video_bitrate;
    video_codec_ctx->gop_size = 12;
    video_codec_ctx->max_b_frames = 2;

    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        video_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    int ret = avcodec_open2(video_codec_ctx, video_codec, nullptr);
    if (ret < 0) {
        log_av_error("Could not open video codec", ret);
        return false;
    }

    ret = avcodec_parameters_from_context(video_stream->codecpar, video_codec_ctx);
    if (ret < 0) {
        log_av_error("Could not copy video codec params", ret);
        return false;
    }
    video_stream->time_base = video_codec_ctx->time_base;

    if (has_audio) {
        const AVCodec *audio_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!audio_codec) {
            UtilityFunctions::push_error("[VideoEncoder] AAC encoder not found");
            return false;
        }

        audio_stream = avformat_new_stream(format_ctx, nullptr);
        if (!audio_stream) {
            UtilityFunctions::push_error("[VideoEncoder] Could not create audio stream");
            return false;
        }
        audio_stream->id = format_ctx->nb_streams - 1;

        audio_codec_ctx = avcodec_alloc_context3(audio_codec);
        if (!audio_codec_ctx) {
            UtilityFunctions::push_error("[VideoEncoder] Could not allocate audio codec context");
            return false;
        }

        audio_codec_ctx->sample_rate = sample_rate;
        audio_codec_ctx->bit_rate = audio_bitrate;
        av_channel_layout_default(&audio_codec_ctx->ch_layout, channels);
        audio_codec_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        audio_codec_ctx->time_base = (AVRational){1, sample_rate};

        if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            audio_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret = avcodec_open2(audio_codec_ctx, audio_codec, nullptr);
        if (ret < 0) {
            log_av_error("Could not open audio codec", ret);
            return false;
        }

        ret = avcodec_parameters_from_context(audio_stream->codecpar, audio_codec_ctx);
        if (ret < 0) {
            log_av_error("Could not copy audio codec params", ret);
            return false;
        }
        audio_stream->time_base = audio_codec_ctx->time_base;
    }

    ret = avio_open(&format_ctx->pb, path_utf8, AVIO_FLAG_WRITE);
    if (ret < 0) {
        log_av_error("Could not open output file", ret);
        return false;
    }

    ret = avformat_write_header(format_ctx, nullptr);
    if (ret < 0) {
        log_av_error("Error writing header", ret);
        return false;
    }

    video_frame = av_frame_alloc();
    video_frame->format = AV_PIX_FMT_YUV420P;
    video_frame->width = width;
    video_frame->height = height;
    ret = av_frame_get_buffer(video_frame, 0);
    if (ret < 0) {
        log_av_error("Could not allocate video frame buffer", ret);
        return false;
    }

    packet = av_packet_alloc();

    sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        UtilityFunctions::push_error("[VideoEncoder] Could not create scaler context");
        return false;
    }

    if (has_audio) {
        audio_frame = av_frame_alloc();
        audio_frame->format = AV_SAMPLE_FMT_FLTP;
        audio_frame->ch_layout = audio_codec_ctx->ch_layout;
        audio_frame->sample_rate = audio_codec_ctx->sample_rate;
        audio_frame->nb_samples = audio_codec_ctx->frame_size;
        if (audio_frame->nb_samples == 0) {
            audio_frame->nb_samples = 1024;
        }
        ret = av_frame_get_buffer(audio_frame, 0);
        if (ret < 0) {
            log_av_error("Could not allocate audio frame buffer", ret);
            return false;
        }

        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            UtilityFunctions::push_error("[VideoEncoder] Could not allocate resampler");
            return false;
        }

        AVChannelLayout src_ch_layout;
        av_channel_layout_default(&src_ch_layout, channels);

        ret = swr_alloc_set_opts2(&swr_ctx,
            &audio_codec_ctx->ch_layout, AV_SAMPLE_FMT_FLTP, sample_rate,
            &src_ch_layout, AV_SAMPLE_FMT_FLT, sample_rate,
            0, nullptr);

        if (ret < 0) {
            log_av_error("Could not set resampler options", ret);
            return false;
        }

        ret = swr_init(swr_ctx);
        if (ret < 0) {
            log_av_error("Could not initialize resampler", ret);
            return false;
        }

        audio_fifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, channels, audio_frame->nb_samples);
        if (!audio_fifo) {
            UtilityFunctions::push_error("[VideoEncoder] Could not allocate audio FIFO");
            return false;
        }
    }

    initialized = true;
    video_frame_count = 0;
    audio_samples_count = 0;
    UtilityFunctions::print("[VideoEncoder] Opened: ", resolved_path);
    return true;
}

bool VideoEncoder::write_frame(Ref<Image> p_image) {
    if (!initialized || p_image.is_null()) {
        return false;
    }

    PackedByteArray data = p_image->get_data();
    if (data.size() == 0) {
        return false;
    }

    const uint8_t *src_data[1] = { data.ptr() };
    int src_linesize[1] = { 4 * width };

    sws_scale(sws_ctx, src_data, src_linesize, 0, height, video_frame->data, video_frame->linesize);

    video_frame->pts = video_frame_count++;

    int ret = avcodec_send_frame(video_codec_ctx, video_frame);
    if (ret < 0) {
        log_av_error("Error sending video frame", ret);
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(video_codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            log_av_error("Error encoding video frame", ret);
            return false;
        }

        av_packet_rescale_ts(packet, video_codec_ctx->time_base, video_stream->time_base);
        packet->stream_index = video_stream->index;

        ret = av_interleaved_write_frame(format_ctx, packet);
        av_packet_unref(packet);
        if (ret < 0) {
            log_av_error("Error writing video packet", ret);
            return false;
        }
    }

    return true;
}

bool VideoEncoder::write_audio(PackedFloat32Array p_samples) {
    if (!initialized || !has_audio || p_samples.size() == 0) {
        return false;
    }

    int num_samples = p_samples.size() / channels;
    const uint8_t *src_data = (const uint8_t *)p_samples.ptr();

    int max_out_samples = swr_get_out_samples(swr_ctx, num_samples);
    if (max_out_samples <= 0) {
        return false;
    }

    uint8_t **dst_data = nullptr;
    int dst_linesize;
    av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, channels,
        max_out_samples, AV_SAMPLE_FMT_FLTP, 0);

    int converted = swr_convert(swr_ctx, dst_data, max_out_samples,
        &src_data, num_samples);

    if (converted < 0) {
        av_freep(&dst_data[0]);
        av_freep(&dst_data);
        log_av_error("Audio resampling failed", converted);
        return false;
    }

    av_audio_fifo_write(audio_fifo, (void **)dst_data, converted);

    av_freep(&dst_data[0]);
    av_freep(&dst_data);

    while (av_audio_fifo_size(audio_fifo) >= audio_codec_ctx->frame_size) {
        av_audio_fifo_read(audio_fifo, (void **)audio_frame->data, audio_codec_ctx->frame_size);

        audio_frame->pts = audio_samples_count;
        audio_samples_count += audio_codec_ctx->frame_size;

        int ret = avcodec_send_frame(audio_codec_ctx, audio_frame);
        if (ret < 0) {
            log_av_error("Error sending audio frame", ret);
            return false;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(audio_codec_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            if (ret < 0) {
                log_av_error("Error encoding audio", ret);
                return false;
            }

            av_packet_rescale_ts(packet, audio_codec_ctx->time_base, audio_stream->time_base);
            packet->stream_index = audio_stream->index;

            ret = av_interleaved_write_frame(format_ctx, packet);
            av_packet_unref(packet);
            if (ret < 0) {
                log_av_error("Error writing audio packet", ret);
                return false;
            }
        }
    }

    return true;
}

void VideoEncoder::close() {
    if (!initialized) {
        return;
    }

    if (has_audio && audio_codec_ctx) {
        int fifo_size = av_audio_fifo_size(audio_fifo);
        while (fifo_size > 0) {
            int samples_to_read = fifo_size < audio_codec_ctx->frame_size ? fifo_size : audio_codec_ctx->frame_size;
            av_audio_fifo_read(audio_fifo, (void **)audio_frame->data, samples_to_read);

            if (samples_to_read < audio_codec_ctx->frame_size) {
                av_samples_set_silence(audio_frame->data, samples_to_read,
                    audio_codec_ctx->frame_size - samples_to_read, channels, AV_SAMPLE_FMT_FLTP);
            }

            audio_frame->pts = audio_samples_count;
            audio_samples_count += audio_codec_ctx->frame_size;

            avcodec_send_frame(audio_codec_ctx, audio_frame);

            int ret;
            while ((ret = avcodec_receive_packet(audio_codec_ctx, packet)) >= 0) {
                av_packet_rescale_ts(packet, audio_codec_ctx->time_base, audio_stream->time_base);
                packet->stream_index = audio_stream->index;
                av_interleaved_write_frame(format_ctx, packet);
                av_packet_unref(packet);
            }

            fifo_size = av_audio_fifo_size(audio_fifo);
        }

        avcodec_send_frame(audio_codec_ctx, nullptr);
        int ret;
        while ((ret = avcodec_receive_packet(audio_codec_ctx, packet)) >= 0) {
            av_packet_rescale_ts(packet, audio_codec_ctx->time_base, audio_stream->time_base);
            packet->stream_index = audio_stream->index;
            av_interleaved_write_frame(format_ctx, packet);
            av_packet_unref(packet);
        }
    }

    if (video_codec_ctx) {
        avcodec_send_frame(video_codec_ctx, nullptr);
        int ret;
        while ((ret = avcodec_receive_packet(video_codec_ctx, packet)) >= 0) {
            av_packet_rescale_ts(packet, video_codec_ctx->time_base, video_stream->time_base);
            packet->stream_index = video_stream->index;
            av_interleaved_write_frame(format_ctx, packet);
            av_packet_unref(packet);
        }
    }

    if (format_ctx) {
        av_write_trailer(format_ctx);
        if (format_ctx->oformat && !(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&format_ctx->pb);
        }
    }

    av_audio_fifo_free(audio_fifo);
    audio_fifo = nullptr;
    swr_free(&swr_ctx);
    av_frame_free(&audio_frame);
    avcodec_free_context(&audio_codec_ctx);

    av_frame_free(&video_frame);
    av_packet_free(&packet);
    avcodec_free_context(&video_codec_ctx);
    sws_freeContext(sws_ctx);
    sws_ctx = nullptr;
    avformat_free_context(format_ctx);
    format_ctx = nullptr;

    initialized = false;
    has_audio = false;
    video_frame_count = 0;
    audio_samples_count = 0;
    UtilityFunctions::print("[VideoEncoder] Closed successfully.");
}

bool VideoEncoder::is_open() const {
    return initialized;
}
