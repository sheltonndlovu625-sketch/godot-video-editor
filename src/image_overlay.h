#ifndef IMAGE_OVERLAY_H
#define IMAGE_OVERLAY_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class ImageOverlay : public RefCounted {
    GDCLASS(ImageOverlay, RefCounted)

private:
    String texture_path;
    Ref<Image> cached_image;
    Ref<ImageTexture> cached_texture;
    bool image_loaded = false;

    Vector2 position;
    Vector2 scale = Vector2(1.0, 1.0);
    float rotation = 0.0f;
    float opacity = 1.0f;
    Vector2 anchor_point = Vector2(0.5, 0.5);

    double start_time = 0.0;
    double end_time = 5.0;

    // GPU resources
    RID overlay_viewport;
    RID overlay_canvas;
    RID overlay_item;
    int cached_w = 0;
    int cached_h = 0;
    bool resources_dirty = true;

    void _ensure_resources(RenderingServer *p_rs, int p_width, int p_height);
    void _free_resources();

protected:
    static void _bind_methods();

public:
    void set_texture_path(const String &p_path);
    String get_texture_path() const;

    void set_position(const Vector2 &p_pos);
    Vector2 get_position() const;

    void set_scale(const Vector2 &p_scale);
    Vector2 get_scale() const;

    void set_rotation(float p_rot);
    float get_rotation() const;

    void set_opacity(float p_opacity);
    float get_opacity() const;

    void set_anchor_point(const Vector2 &p_anchor);
    Vector2 get_anchor_point() const;

    void set_start_time(double p_time);
    double get_start_time() const;

    void set_end_time(double p_time);
    double get_end_time() const;

    bool is_visible_at(double p_time) const;

    RID render_to_rid(RenderingServer *p_rs, int p_canvas_w, int p_canvas_h, double p_time);
    Ref<Image> render_to_image();

    ImageOverlay();
    ~ImageOverlay();
};

}

#endif
