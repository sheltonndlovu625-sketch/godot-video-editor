#include "video_encoder.h"

using namespace godot;

void VideoEncoder::_bind_methods() {
    ClassDB::bind_method(
        D_METHOD("open", "path", "width", "height", "fps", "bitrate"),
        &VideoEncoder::open
    );
    ClassDB::bind_method(
        D_METHOD("write_frame", "image"),
        &VideoEncoder::write_frame
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
    width = p_width;
    height = p_height;
    fps = p_fps;

    // Allocate output context (auto-detect format from extension, e.g. .mp4)
    avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, p_path.utf8().get_data());
    if (!format_ctx) {
        UtilityFunctions::push_error("[VideoEncoder] Could not allocate output context");
        return false;
    }

    // Find H.264 encoder
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        UtilityFunctions::push_error("[VideoEncoder] H.264 encoder not found");
        return false;
    }

    // Create stream
    stream = avformat_new_stream(format_ctx, nullptr);
    if (!stream) {
        UtilityFunctions::push_error("[VideoEncoder] Could not create stream");
        return false;
    }
    stream->id = format_ctx->nb_streams - 1;

    // Allocate codec context
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        UtilityFunctions::push_error("[VideoEncoder] Could not allocate codec context");
        return false;
    }

    // Configure encoder
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = (AVRational){1, fps};
    codec_ctx->framerate = (AVRational){fps, 1};
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->bit_rate = p_bitrate;
    codec_ctx->gop_size = 12;
    codec_ctx->max_b_frames = 2;

    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Open codec
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "preset", "fast", 0);
    av_dict_set(&opts, "crf", "23", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);

    int ret = avcodec_open2(codec_ctx, codec, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Could not open codec");
        return false;
    }

    // Copy codec parameters to stream
    ret = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Could not copy codec params");
        return false;
    }
    stream->time_base = codec_ctx->time_base;

    // Open output file
    ret = avio_open(&format_ctx->pb, p_path.utf8().get_data(), AVIO_FLAG_WRITE);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Could not open output file");
        return false;
    }

    // Write header
    ret = avformat_write_header(format_ctx, nullptr);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Error writing header");
        return false;
    }

    // Allocate frame
    frame = av_frame_alloc();
    frame->format = AV_PIX_FMT_YUV420P;
    frame->width = width;
    frame->height = height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Could not allocate frame buffer");
        return false;
    }

    packet = av_packet_alloc();

    // Setup scaler: RGBA -> YUV420P
    sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        UtilityFunctions::push_error("[VideoEncoder] Could not create scaler context");
        return false;
    }

    initialized = true;
    frame_count = 0;
    UtilityFunctions::print("[VideoEncoder] Opened: ", p_path);
    return true;
}

bool VideoEncoder::write_frame(Ref<Image> p_image) {
    if (!initialized || p_image.is_null()) {
        return false;
    }

    // Get raw RGBA bytes from Godot Image
    PackedByteArray data = p_image->get_data();
    if (data.size() == 0) {
        return false;
    }

    const uint8_t *src_data[1] = { data.ptr() };
    int src_linesize[1] = { 4 * width }; // RGBA = 4 bytes per pixel

    // Convert to YUV420P
    sws_scale(sws_ctx, src_data, src_linesize, 0, height, frame->data, frame->linesize);

    frame->pts = frame_count++;

    // Send frame to encoder
    int ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        UtilityFunctions::push_error("[VideoEncoder] Error sending frame");
        return false;
    }

    // Receive packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            UtilityFunctions::push_error("[VideoEncoder] Error encoding frame");
            return false;
        }

        av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
        packet->stream_index = stream->index;

        ret = av_interleaved_write_frame(format_ctx, packet);
        av_packet_unref(packet);
        if (ret < 0) {
            UtilityFunctions::push_error("[VideoEncoder] Error writing packet");
            return false;
        }
    }

    return true;
}

void VideoEncoder::close() {
    if (!initialized) {
        return;
    }

    // Flush encoder
    avcodec_send_frame(codec_ctx, nullptr);

    int ret;
    while ((ret = avcodec_receive_packet(codec_ctx, packet)) >= 0) {
        av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);
        packet->stream_index = stream->index;
        av_interleaved_write_frame(format_ctx, packet);
        av_packet_unref(packet);
    }

    // Write trailer
    av_write_trailer(format_ctx);

    // Close output
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&format_ctx->pb);
    }

    // Cleanup
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    sws_freeContext(sws_ctx);
    avformat_free_context(format_ctx);

    initialized = false;
    UtilityFunctions::print("[VideoEncoder] Closed successfully.");
}

bool VideoEncoder::is_open() const {
    return initialized;
}
