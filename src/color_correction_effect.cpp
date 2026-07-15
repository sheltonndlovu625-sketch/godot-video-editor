#include "color_correction_effect.h"
#include <godot_cpp/variant/color.hpp>

using namespace godot;

void ColorCorrectionEffect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_brightness", "val"), &ColorCorrectionEffect::set_brightness);
    ClassDB::bind_method(D_METHOD("get_brightness"), &ColorCorrectionEffect::get_brightness);
    ClassDB::add_property("ColorCorrectionEffect", PropertyInfo(Variant::FLOAT, "brightness"), "set_brightness", "get_brightness");

    ClassDB::bind_method(D_METHOD("set_contrast", "val"), &ColorCorrectionEffect::set_contrast);
    ClassDB::bind_method(D_METHOD("get_contrast"), &ColorCorrectionEffect::get_contrast);
    ClassDB::add_property("ColorCorrectionEffect", PropertyInfo(Variant::FLOAT, "contrast"), "set_contrast", "get_contrast");

    ClassDB::bind_method(D_METHOD("set_saturation", "val"), &ColorCorrectionEffect::set_saturation);
    ClassDB::bind_method(D_METHOD("get_saturation"), &ColorCorrectionEffect::get_saturation);
    ClassDB::add_property("ColorCorrectionEffect", PropertyInfo(Variant::FLOAT, "saturation"), "set_saturation", "get_saturation");
}

void ColorCorrectionEffect::set_brightness(float p_val) { brightness = p_val; }
float ColorCorrectionEffect::get_brightness() const { return brightness; }
void ColorCorrectionEffect::set_contrast(float p_val) { contrast = p_val; }
float ColorCorrectionEffect::get_contrast() const { return contrast; }
void ColorCorrectionEffect::set_saturation(float p_val) { saturation = p_val; }
float ColorCorrectionEffect::get_saturation() const { return saturation; }

Ref<Image> ColorCorrectionEffect::apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) {
    if (p_input.is_null()) return p_input;

    Ref<Image> result = p_input->duplicate();
    int w = result->get_width();
    int h = result->get_height();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color c = result->get_pixel(x, y);

            // Brightness
            c.r += brightness;
            c.g += brightness;
            c.b += brightness;

            // Contrast
            c.r = (c.r - 0.5f) * contrast + 0.5f;
            c.g = (c.g - 0.5f) * contrast + 0.5f;
            c.b = (c.b - 0.5f) * contrast + 0.5f;

            // Saturation (simple luma-based)
            float luma = 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
            c.r = luma + (c.r - luma) * saturation;
            c.g = luma + (c.g - luma) * saturation;
            c.b = luma + (c.b - luma) * saturation;

            c.r = CLAMP(c.r, 0.0f, 1.0f);
            c.g = CLAMP(c.g, 0.0f, 1.0f);
            c.b = CLAMP(c.b, 0.0f, 1.0f);

            result->set_pixel(x, y, c);
        }
    }
    return result;
}

ColorCorrectionEffect::ColorCorrectionEffect() {}
