#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace godot {

class VideoEncoder : public RefCounted {
    GDCLASS(VideoEncoder, RefCounted)

private:
    AVFormatContext *format_ctx = nullptr;
    AVCodecContext *codec_ctx = nullptr;
    AVStream *stream = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *sws_ctx = nullptr;

    int width = 0;
    int height = 0;
    int fps = 30;
    int64_t frame_count = 0;
    bool initialized = false;

protected:
    static void _bind_methods();

public:
    bool open(String p_path, int p_width, int p_height, int p_fps, int p_bitrate);
    bool write_frame(Ref<Image> p_image);
    void close();
    bool is_open() const;

    VideoEncoder();
    ~VideoEncoder();
};

}

#endif
