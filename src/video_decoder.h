#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace godot {

class VideoDecoder : public RefCounted {
    GDCLASS(VideoDecoder, RefCounted)

private:
    AVFormatContext *format_ctx = nullptr;

    AVCodecContext *video_codec_ctx = nullptr;
    int video_stream_index = -1;
    AVFrame *video_frame = nullptr;
    AVFrame *rgba_frame = nullptr;
    SwsContext *sws_ctx = nullptr;

    AVCodecContext *audio_codec_ctx = nullptr;
    int audio_stream_index = -1;
    AVFrame *audio_frame = nullptr;
    SwrContext *swr_ctx = nullptr;

    PackedFloat32Array audio_buffer;
    double duration = 0.0;
    double current_time = 0.0;
    int sample_rate = 0;
    int channels = 0;
    bool initialized = false;

protected:
    static void _bind_methods();

public:
    bool open(String p_path);
    Ref<Image> read_video_frame();
    PackedFloat32Array read_audio_samples(int p_max_samples);
    bool seek(double p_time_seconds);
    double get_duration() const;
    double get_current_time() const;
    int get_video_width() const;
    int get_video_height() const;
    double get_video_fps() const;
    bool has_audio() const;
    void close();
    bool is_open() const;

    VideoDecoder();
    ~VideoDecoder();
};

}

#endif
