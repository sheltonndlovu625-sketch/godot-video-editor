#include "transition_effect.h"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void TransitionEffect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_mask_path", "path"), &TransitionEffect::set_mask_path);
    ClassDB::bind_method(D_METHOD("get_mask_path"), &TransitionEffect::get_mask_path);
    ClassDB::add_property("TransitionEffect", PropertyInfo(Variant::STRING, "mask_path"), "set_mask_path", "get_mask_path");

    ClassDB::bind_method(D_METHOD("set_progress", "val"), &TransitionEffect::set_progress);
    ClassDB::bind_method(D_METHOD("get_progress"), &TransitionEffect::get_progress);
    ClassDB::add_property("TransitionEffect", PropertyInfo(Variant::FLOAT, "progress", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_progress", "get_progress");

    ClassDB::bind_method(D_METHOD("set_tint_color", "color"), &TransitionEffect::set_tint_color);
    ClassDB::bind_method(D_METHOD("get_tint_color"), &TransitionEffect::get_tint_color);
    ClassDB::add_property("TransitionEffect", PropertyInfo(Variant::COLOR, "tint_color"), "set_tint_color", "get_tint_color");
}

TransitionEffect::TransitionEffect() {}

TransitionEffect::~TransitionEffect() { 
    _free_resources(); 
}

void TransitionEffect::set_mask_path(const String &p_path) {
    mask_path = p_path;
    mask_loaded = false;
    cached_mask.unref();
    mask_texture.unref();
    resources_dirty = true;
}

String TransitionEffect::get_mask_path() const { 
    return mask_path; 
}

void TransitionEffect::set_progress(float p_val) { 
    progress = CLAMP(p_val, 0.0f, 1.0f); 
}

float TransitionEffect::get_progress() const { 
    return progress; 
}

void TransitionEffect::set_tint_color(const Color &p_color) { 
    tint_color = p_color; 
}

Color TransitionEffect::get_tint_color() const { 
    return tint_color; 
}

void TransitionEffect::_ensure_resources(RenderingServer *p_rs, int p_width, int p_height) {
    if (!mask_loaded && !mask_path.is_empty()) {
        String resolved = mask_path;
        if (resolved.begins_with("user://") || resolved.begins_with("res://")) {
            ProjectSettings *ps = ProjectSettings::get_singleton();
            if (ps) resolved = ps->globalize_path(resolved);
        }
        cached_mask = Image::load_from_file(resolved);
        if (cached_mask.is_valid()) {
            mask_texture.instantiate();
            mask_texture->set_image(cached_mask);
            mask_loaded = true;
        } else {
            UtilityFunctions::push_error("[TransitionEffect] Failed to load mask: ", mask_path);
        }
    }

    if (viewport.is_valid() && cached_width == p_width && cached_height == p_height && !resources_dirty) {
        return;
    }

    _free_resources();

    viewport = p_rs->viewport_create();
    p_rs->viewport_set_size(viewport, p_width, p_height);
    p_rs->viewport_set_transparent_background(viewport, false);
    p_rs->viewport_set_active(viewport, true);
    p_rs->viewport_set_update_mode(viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);

    canvas = p_rs->canvas_create();
    canvas_item = p_rs->canvas_item_create();
    p_rs->canvas_item_set_parent(canvas_item, canvas);
    p_rs->viewport_attach_canvas(viewport, canvas);

    shader = p_rs->shader_create();
    const char *code = R"(
shader_type canvas_item;
uniform sampler2D mask_texture;
uniform float progress : hint_range(0.0, 1.0) = 0.0;
uniform vec4 tint_color : source_color = vec4(0.0, 1.0, 0.0, 1.0);

void fragment() {
    vec4 c = texture(TEXTURE, UV);
    float mask_val = texture(mask_texture, UV).r;
    
    // Smoothstep for a cleaner transition band based on the thumbnail image
    float alpha = smoothstep(mask_val - 0.1, mask_val + 0.1, progress);
    
    // Tint customized transition
    vec4 final_color = mix(c, c * tint_color, alpha * progress);
    
    COLOR = vec4(clamp(final_color.rgb, 0.0, 1.0), c.a);
}
)";
    p_rs->shader_set_code(shader, code);

    material = p_rs->material_create();
    p_rs->material_set_shader(material, shader);

    cached_width = p_width;
    cached_height = p_height;
    resources_dirty = false;
}

void TransitionEffect::_free_resources() {
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

RID TransitionEffect::apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) {
    _ensure_resources(p_rs, p_width, p_height);

    p_rs->material_set_param(material, "progress", progress);
    p_rs->material_set_param(material, "tint_color", tint_color);
    if (mask_texture.is_valid()) {
        p_rs->material_set_param(material, "mask_texture", mask_texture->get_rid());
    }

    p_rs->canvas_item_clear(canvas_item);
    p_rs->canvas_item_set_material(canvas_item, material);
    p_rs->canvas_item_add_texture_rect(canvas_item, Rect2(0, 0, p_width, p_height), p_input);

    p_rs->viewport_set_update_mode(viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    return p_rs->viewport_get_texture(viewport);
}

Ref<Image> TransitionEffect::apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) {
    if (p_input.is_null()) return p_input;

    Ref<Image> result = p_input->duplicate();
    if (!cached_mask.is_valid() || progress <= 0.0f) return result;

    int w = result->get_width();
    int h = result->get_height();
    int mw = cached_mask->get_width();
    int mh = cached_mask->get_height();

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Color c = result->get_pixel(x, y);
            
            float mask_val = 0.0f;
            if (mw > 0 && mh > 0) {
                int mx = x * mw / w;
                int my = y * mh / h;
                mask_val = cached_mask->get_pixel(mx, my).r;
            }
            
            if (mask_val <= progress) {
                c = c.lerp(c * tint_color, progress);
            }
            
            result->set_pixel(x, y, c);
        }
    }
    return result;
}