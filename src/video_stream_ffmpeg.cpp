#include "video_stream_ffmpeg.h"
#include "video_stream_playback_ffmpeg.h"
#include "video_decoder.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void VideoStreamFFmpeg::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_clip_paths", "paths"), &VideoStreamFFmpeg::set_clip_paths);
    ClassDB::bind_method(D_METHOD("get_clip_paths"), &VideoStreamFFmpeg::get_clip_paths);
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "clip_paths"), "set_clip_paths", "get_clip_paths");
}

void VideoStreamFFmpeg::set_clip_paths(const PackedStringArray &p_paths) {
    clip_paths = p_paths;
}

PackedStringArray VideoStreamFFmpeg::get_clip_paths() const {
    return clip_paths;
}

Ref<VideoStreamPlayback> VideoStreamFFmpeg::_instantiate_playback() {
    Ref<VideoStreamPlaybackFFmpeg> playback;
    playback.instantiate();

    Vector<VideoStreamPlaybackFFmpeg::Clip> clips;
    double total = 0.0;

    for (int i = 0; i < clip_paths.size(); i++) {
        Ref<VideoDecoder> probe;
        probe.instantiate();
        if (probe->open(clip_paths[i])) {
            VideoStreamPlaybackFFmpeg::Clip c;
            c.path = clip_paths[i];
            c.start_time = total;
            c.duration = probe->get_duration();
            c.width = probe->get_video_width();
            c.height = probe->get_video_height();
            clips.push_back(c);
            total += c.duration;
            probe->close();
        }
    }

    playback->setup(clips, total);
    return playback;
}
