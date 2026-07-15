#ifndef COLOR_CORRECTION_EFFECT_H
#define COLOR_CORRECTION_EFFECT_H

#include "video_effect.h"

namespace godot {

class ColorCorrectionEffect : public VideoEffect {
    GDCLASS(ColorCorrectionEffect, VideoEffect)

private:
    float brightness = 0.0f;   // -1.0 to 1.0
    float contrast = 1.0f;     // 0.0 to 2.0
    float saturation = 1.0f;   // 0.0 to 2.0

protected:
    static void _bind_methods();

public:
    void set_brightness(float p_val);
    float get_brightness() const;
    void set_contrast(float p_val);
    float get_contrast() const;
    void set_saturation(float p_val);
    float get_saturation() const;

    Ref<Image> apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) override;

    ColorCorrectionEffect();
};

}

#endif
