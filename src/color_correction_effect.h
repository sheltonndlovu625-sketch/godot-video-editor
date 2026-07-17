#ifndef COLOR_CORRECTION_EFFECT_H
#define COLOR_CORRECTION_EFFECT_H

#include "video_effect.h"

namespace godot {

class ColorCorrectionEffect : public VideoEffect {
    GDCLASS(ColorCorrectionEffect, VideoEffect)

private:
    float brightness = 0.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;

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
    void set_brightness(float p_val);
    float get_brightness() const;
    void set_contrast(float p_val);
    float get_contrast() const;
    void set_saturation(float p_val);
    float get_saturation() const;

    virtual Ref<Image> apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) override;
    virtual RID apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) override;

    ColorCorrectionEffect();
    ~ColorCorrectionEffect();
};

}

#endif
