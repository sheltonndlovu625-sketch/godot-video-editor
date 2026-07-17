#ifndef TIMELINE_RENDERER_H
#define TIMELINE_RENDERER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/canvas_item_material.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include "timeline.h"
#include "video_encoder.h"
#include "video_decoder.h"
#include "video_effect.h"
#include "text_overlay.h"
#include "image_overlay.h"

namespace godot {

class TimelineRenderer : public RefCounted {
    GDCLASS(TimelineRenderer, RefCounted)

public:
    enum AspectRatioMode {
        ASPECT_FILL,
        ASPECT_FIT,
        ASPECT_STRETCH
    };

private:
    Ref<Timeline> timeline;
    Dictionary decoders;
    double last_render_time = -1.0;

    Ref<ImageTexture> preview_texture;
    RID preview_texture_rid;
    int preview_tex_w = 0;
    int preview_tex_h = 0;

    Ref<Image> composite_buffer;
    Ref<Image> black_frame;

    RID comp_viewport;
    RID comp_canvas;
    int comp_w = 0;
    int comp_h = 0;
    Vector<RID> layer_items;

    Ref<CanvasItemMaterial> mat_normal;
    Ref<CanvasItemMaterial> mat_add;
    Ref<CanvasItemMaterial> mat_multiply;
    Ref<CanvasItemMaterial> mat_subtract;

    AspectRatioMode aspect_ratio_mode = ASPECT_FILL;

    Ref<VideoDecoder> get_decoder(const String &p_path);
    bool _needs_seek(double p_time);
    Ref<Image> composite_frames(const TypedArray<Image> &p_frames, int p_width, int p_height);
    Ref<Image> composite_frames_fast(const Vector<Ref<Image>> &p_frames, int p_width, int p_height);
    PackedFloat32Array mix_audio(const TypedArray<PackedFloat32Array> &p_buffers);

    void _ensure_gpu_compositor(RenderingServer *p_rs, int p_width, int p_height);
    void _free_gpu_compositor();
    void _ensure_layer_items(RenderingServer *p_rs, int p_count);
    void _free_layer_items();
    void _ensure_blend_materials();
    RID _get_blend_material(int p_blend_mode) const;
    RID _composite_gpu_with_transforms(RenderingServer *p_rs,
        const Vector<RID> &p_textures,
        const Vector<Transform2D> &p_transforms,
        const Vector<int> &p_blend_modes,
        const Vector<float> &p_opacities,
        int p_width, int p_height);

    Vector2i _get_decode_size(int p_src_w, int p_src_h, int p_dst_w, int p_dst_h) const;
    void _cpu_blit_normal(Image *p_dst, const Image *p_src, int p_dx, int p_dy, float p_opacity);
    void _cpu_blit_add(Image *p_dst, const Image *p_src, int p_dx, int p_dy, float p_opacity);
    void _cpu_blit_multiply(Image *p_dst, const Image *p_src, int p_dx, int p_dy, float p_opacity);
    void _cpu_blit_subtract(Image *p_dst, const Image *p_src, int p_dx, int p_dy, float p_opacity);
    void _cpu_blit(Image *p_dst, const Image *p_src, int p_dx, int p_dy, float p_opacity, int p_blend_mode);
    Ref<Image> _cpu_render_text_overlay(const Ref<TextOverlay> &p_overlay, int p_canvas_w, int p_canvas_h, double p_time);
    Ref<Image> _cpu_render_image_overlay(const Ref<ImageOverlay> &p_overlay, int p_canvas_w, int p_canvas_h, double p_time);
    Ref<Image> _cpu_composite_frame(double p_time, int p_width, int p_height);

    Ref<Image> _apply_cpu_effects(const Ref<Image> &p_frame, const TypedArray<VideoEffect> &p_effects, int p_width, int p_height);

protected:
    static void _bind_methods();

public:
    void set_timeline(const Ref<Timeline> &p_timeline);
    Ref<Timeline> get_timeline() const;

    void set_aspect_ratio_mode(int p_mode);
    int get_aspect_ratio_mode() const;

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

VARIANT_ENUM_CAST(TimelineRenderer::AspectRatioMode);

#endif