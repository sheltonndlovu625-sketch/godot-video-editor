#ifndef TIMELINE_RENDERER_H
#define TIMELINE_RENDERER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "timeline.h"
#include "video_encoder.h"
#include "video_decoder.h"

namespace godot {

class TimelineRenderer : public RefCounted {
    GDCLASS(TimelineRenderer, RefCounted)

private:
    Ref<Timeline> timeline;
    Dictionary decoders;
    double last_render_time = -1.0;
    bool preview_mode = false;          // <-- ADDED: uses proxy paths + skips audio decode

    // Reusable output texture instead of Dictionary cache
    Ref<ImageTexture> output_texture;
    int output_texture_w = 0;
    int output_texture_h = 0;

    Ref<VideoDecoder> get_decoder(const String &p_path, bool p_skip_audio);
    bool _needs_seek(double p_time);
    Ref<Image> composite_frames_fast(const Vector<Ref<Image>> &p_frames, int p_width, int p_height);
    PackedFloat32Array mix_audio(const TypedArray<PackedFloat32Array> &p_buffers);

protected:
    static void _bind_methods();

public:
    void set_timeline(const Ref<Timeline> &p_timeline);
    Ref<Timeline> get_timeline() const;

    void set_preview_mode(bool p_preview);   // <-- ADDED
    bool get_preview_mode() const;           // <-- ADDED

    Ref<Image> render_video_frame(double p_time, int p_width, int p_height);
    Ref<ImageTexture> render_video_frame_to_texture(double p_time, int p_width, int p_height);
    PackedFloat32Array render_audio(double p_time, int p_num_samples, int p_sample_rate);
    bool export_to_file(const String &p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_audio_bitrate);
    void clear_cache();

    TimelineRenderer();
    ~TimelineRenderer();
};

}

#endif
