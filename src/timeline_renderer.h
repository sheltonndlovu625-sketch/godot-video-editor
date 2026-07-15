#ifndef TIMELINE_RENDERER_H
#define TIMELINE_RENDERER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include "timeline.h"
#include "video_encoder.h"
#include "video_decoder.h"
#include "video_effect.h"

namespace godot {

class TimelineRenderer : public RefCounted {
    GDCLASS(TimelineRenderer, RefCounted)

private:
    Ref<Timeline> timeline;
    Dictionary decoders;
    double last_render_time = -1.0;

    // Persistent preview resources (create once, update forever)
    Ref<ImageTexture> preview_texture;
    RID preview_texture_rid;
    int preview_tex_w = 0;
    int preview_tex_h = 0;

    // Reusable compositing buffers
    Ref<Image> composite_buffer;
    Ref<Image> black_frame;

    // GPU compositor for preview with effects
    RID comp_viewport;
    RID comp_canvas;
    RID comp_canvas_item;
    int comp_w = 0;
    int comp_h = 0;

    Ref<VideoDecoder> get_decoder(const String &p_path);
    bool _needs_seek(double p_time);
    Ref<Image> composite_frames(const TypedArray<Image> &p_frames, int p_width, int p_height);
    Ref<Image> composite_frames_fast(const Vector<Ref<Image>> &p_frames, int p_width, int p_height);
    PackedFloat32Array mix_audio(const TypedArray<PackedFloat32Array> &p_buffers);

    // GPU compositor helpers
    void _ensure_gpu_compositor(RenderingServer *p_rs, int p_width, int p_height);
    void _free_gpu_compositor();
    RID _composite_gpu(RenderingServer *p_rs, const Vector<RID> &p_clip_textures, int p_width, int p_height);

    // Export helper: apply CPU effects to a frame
    Ref<Image> _apply_cpu_effects(const Ref<Image> &p_frame, const TypedArray<VideoEffect> &p_effects, int p_width, int p_height);

protected:
    static void _bind_methods();

public:
    void set_timeline(const Ref<Timeline> &p_timeline);
    Ref<Timeline> get_timeline() const;

    Ref<Image> render_video_frame(double p_time, int p_width, int p_height);
    Ref<ImageTexture> render_video_frame_to_texture(double p_time, int p_width, int p_height);
    RID render_video_frame_to_rid(double p_time, int p_width, int p_height);
    PackedFloat32Array render_audio(double p_time, int p_num_samples, int p_sample_rate);
    bool export_to_file(const String &p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_audio_bitrate);
    void clear_cache();

    TimelineRenderer();
    ~TimelineRenderer();
};

}

#endif
