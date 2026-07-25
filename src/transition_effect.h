#ifndef TRANSITION_EFFECT_H
#define TRANSITION_EFFECT_H

#include "video_effect.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class TransitionEffect : public VideoEffect {
    GDCLASS(TransitionEffect, VideoEffect)

private:
    String mask_path;
    Ref<Image> cached_mask;
    Ref<ImageTexture> mask_texture;
    bool mask_loaded = false;
    bool resources_dirty = true;

    float progress = 0.0f;
    Color tint_color = Color(0.0, 1.0, 0.0, 1.0); // Defaulting to green

    RID viewport;
    RID canvas;
    RID canvas_item;
    RID shader;
    RID material;
    int cached_width = 0;
    int cached_height = 0;

    void _ensure_resources(RenderingServer *p_rs, int p_width, int p_height);
    void _free_resources();

protected:
    static void _bind_methods();

public:
    void set_mask_path(const String &p_path);
    String get_mask_path() const;

    void set_progress(float p_val);
    float get_progress() const;

    void set_tint_color(const Color &p_color);
    Color get_tint_color() const;

    virtual Ref<Image> apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) override;
    virtual RID apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) override;

    TransitionEffect();
    ~TransitionEffect();
};

}

#endif