#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace godot {

class VideoEncoder : public RefCounted {
    GDCLASS(VideoEncoder, RefCounted)

private:
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *video_codec_ctx = nullptr;
    AVStream *video_stream = nullptr;
    AVFrame *video_frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;

    AVCodecContext *audio_codec_ctx = nullptr;
    AVStream *audio_stream = nullptr;
    AVFrame *audio_frame = nullptr;
    SwrContext *swr_ctx = nullptr;
    AVAudioFifo *audio_fifo = nullptr;

    int width = 0;
    int height = 0;
    int fps = 30;
    int64_t video_frame_count = 0;
    int64_t audio_samples_count = 0;
    bool initialized = false;
    bool has_audio = false;

    int sample_rate = 48000;
    int channels = 2;
    int audio_bitrate = 128000;

protected:
    static void _bind_methods();

public:
    bool open(String p_path, int p_width, int p_height, int p_fps, int p_bitrate);
    bool open_with_audio(String p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_channels, int p_audio_bitrate);
    bool write_frame(Ref<Image> p_image);
    bool write_audio(PackedFloat32Array p_samples);
    void close();
    bool is_open() const;

    VideoEncoder();
    ~VideoEncoder();
};

}

#endif
