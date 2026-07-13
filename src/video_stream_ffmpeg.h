#ifndef VIDEO_STREAM_FFMPEG_H
#define VIDEO_STREAM_FFMPEG_H

#include <godot_cpp/classes/video_stream.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {

class VideoStreamFFmpeg : public VideoStream {
    GDCLASS(VideoStreamFFmpeg, VideoStream)

    PackedStringArray clip_paths;

protected:
    static void _bind_methods();

public:
    void set_clip_paths(const PackedStringArray &p_paths);
    PackedStringArray get_clip_paths() const;

    virtual Ref<VideoStreamPlayback> _instantiate_playback() override;
};

}

#endif