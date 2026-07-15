#ifndef SHADER_VIDEO_EFFECT_H
#define SHADER_VIDEO_EFFECT_H

#include "video_effect.h"
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class ShaderVideoEffect : public VideoEffect {
    GDCLASS(ShaderVideoEffect, VideoEffect)

private:
    String shader_code;
    Dictionary parameters;

    // Cached GPU resources (recreated if size changes)
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
    void set_shader_code(const String &p_code);
    String get_shader_code() const;

    void set_parameters(const Dictionary &p_params);
    Dictionary get_parameters() const;

    // GPU path: render input texture through shader viewport
    RID apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) override;

    // CPU path: not supported for shaders (requires GPU readback)
    Ref<Image> apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) override;

    ShaderVideoEffect();
    ~ShaderVideoEffect();
};

}

#endif
