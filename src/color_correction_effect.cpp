#include "color_correction_effect.h"
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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

ColorCorrectionEffect::ColorCorrectionEffect() {}
ColorCorrectionEffect::~ColorCorrectionEffect() { _free_resources(); }

void ColorCorrectionEffect::_ensure_resources(RenderingServer *p_rs, int p_width, int p_height) {
    if (viewport.is_valid() && cached_width == p_width && cached_height == p_height) {
        return;
    }

    _free_resources();

    viewport = p_rs->viewport_create();
    p_rs->viewport_set_size(viewport, p_width, p_height);
    p_rs->viewport_set_transparent_background(viewport, false);
    p_rs->viewport_set_active(viewport, true);

    canvas = p_rs->canvas_create();
    canvas_item = p_rs->canvas_item_create();
    p_rs->canvas_item_set_parent(canvas_item, canvas);
    p_rs->viewport_attach_canvas(viewport, canvas);

    shader = p_rs->shader_create();
    const char *code = R"(
shader_type canvas_item;
uniform float brightness : hint_range(-1.0, 1.0) = 0.0;
uniform float contrast : hint_range(0.0, 3.0) = 1.0;
uniform float saturation : hint_range(0.0, 3.0) = 1.0;

void fragment() {
    vec3 c = texture(TEXTURE, UV).rgb;
    c += brightness;
    c = (c - 0.5) * contrast + 0.5;
    float luma = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(luma), c, saturation);
    COLOR = vec4(clamp(c, 0.0, 1.0), texture(TEXTURE, UV).a);
}
)";
    p_rs->shader_set_code(shader, code);

    material = p_rs->material_create();
    p_rs->material_set_shader(material, shader);

    cached_width = p_width;
    cached_height = p_height;
}

void ColorCorrectionEffect::_free_resources() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return;
    if (material.is_valid()) { p_rs->free_rid(material); material = RID(); }
    if (shader.is_valid()) { p_rs->free_rid(shader); shader = RID(); }
    if (canvas_item.is_valid()) { p_rs->free_rid(canvas_item); canvas_item = RID(); }
    if (canvas.is_valid()) { p_rs->free_rid(canvas); canvas = RID(); }
    if (viewport.is_valid()) { p_rs->free_rid(viewport); viewport = RID(); }
    cached_width = 0;
    cached_height = 0;
}

RID ColorCorrectionEffect::apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) {
    _ensure_resources(p_rs, p_width, p_height);

    p_rs->material_set_param(material, "brightness", brightness);
    p_rs->material_set_param(material, "contrast", contrast);
    p_rs->material_set_param(material, "saturation", saturation);

    p_rs->canvas_item_clear(canvas_item);
    p_rs->canvas_item_set_material(canvas_item, material);
    p_rs->canvas_item_add_texture_rect(canvas_item, Rect2(0, 0, p_width, p_height), p_input);

    p_rs->viewport_set_update_mode(viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    return p_rs->viewport_get_texture(viewport);
}

Ref<Image> ColorCorrectionEffect::apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) {
    if (p_input.is_null()) return p_input;

    Ref<Image> result = p_input->duplicate();
    int w = result->get_width();
    int h = result->get_height();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color c = result->get_pixel(x, y);
            c.r += brightness;
            c.g += brightness;
            c.b += brightness;
            c.r = (c.r - 0.5f) * contrast + 0.5f;
            c.g = (c.g - 0.5f) * contrast + 0.5f;
            c.b = (c.b - 0.5f) * contrast + 0.5f;
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
