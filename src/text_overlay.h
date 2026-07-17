#ifndef TEXT_OVERLAY_H
#define TEXT_OVERLAY_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class TextOverlay : public RefCounted {
    GDCLASS(TextOverlay, RefCounted)

public:
    enum AnimationType {
        ANIM_NONE,
        ANIM_FADE_IN,
        ANIM_FADE_OUT,
        ANIM_SLIDE_IN_LEFT,
        ANIM_SLIDE_IN_RIGHT,
        ANIM_SLIDE_IN_TOP,
        ANIM_SLIDE_IN_BOTTOM,
        ANIM_BOUNCE,
        ANIM_TYPEWRITER
    };

private:
    String text;
    Ref<Font> font;
    int font_size = 48;
    Color color = Color(1, 1, 1, 1);
    int outline_size = 0;
    Color outline_color = Color(0, 0, 0, 1);
    Vector2 shadow_offset = Vector2(2, 2);
    Color shadow_color = Color(0, 0, 0, 0.5);
    Color background_color = Color(0, 0, 0, 0);
    Vector2 padding = Vector2(4, 4);
    Vector2 position;
    Vector2 anchor_point = Vector2(0.5, 0.5);
    float opacity = 1.0f;
    double start_time = 0.0;
    double end_time = 5.0;
    AnimationType animation = ANIM_NONE;
    double animation_duration = 1.0;

    bool text_dirty = true;
    int cached_text_w = 0;
    int cached_text_h = 0;

    RID text_viewport;
    RID text_canvas;
    RID text_item;

    Vector2 _compute_text_size() const;
    void _free_text_resources();
    void _ensure_text_resources(RenderingServer *p_rs);
    void _apply_animation(double p_local_time, float &r_opacity, Vector2 &r_position_offset);

protected:
    static void _bind_methods();

public:
    void set_text(const String &p_text);
    String get_text() const;
    void set_font(const Ref<Font> &p_font);
    Ref<Font> get_font() const;
    void set_font_size(int p_size);
    int get_font_size() const;
    void set_color(const Color &p_color);
    Color get_color() const;
    void set_outline_size(int p_size);
    int get_outline_size() const;
    void set_outline_color(const Color &p_color);
    Color get_outline_color() const;
    void set_shadow_offset(const Vector2 &p_offset);
    Vector2 get_shadow_offset() const;
    void set_shadow_color(const Color &p_color);
    Color get_shadow_color() const;
    void set_background_color(const Color &p_color);
    Color get_background_color() const;
    void set_padding(const Vector2 &p_padding);
    Vector2 get_padding() const;
    void set_position(const Vector2 &p_pos);
    Vector2 get_position() const;
    void set_anchor_point(const Vector2 &p_anchor);
    Vector2 get_anchor_point() const;
    void set_opacity(float p_opacity);
    float get_opacity() const;
    void set_start_time(double p_time);
    double get_start_time() const;
    void set_end_time(double p_time);
    double get_end_time() const;
    bool is_visible_at(double p_time) const;
    void set_animation(int p_anim);
    int get_animation() const;
    void set_animation_duration(double p_duration);
    double get_animation_duration() const;
    void mark_dirty();

    Vector2 get_render_size() const { return Vector2((float)cached_text_w, (float)cached_text_h); }

    RID render_to_rid(RenderingServer *p_rs, int p_canvas_w, int p_canvas_h, double p_time);
    Ref<Image> render_to_image();

    TextOverlay();
    ~TextOverlay();
};

}

VARIANT_ENUM_CAST(TextOverlay::AnimationType);

#endif
