#include "shader_video_effect.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void ShaderVideoEffect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_shader_code", "code"), &ShaderVideoEffect::set_shader_code);
    ClassDB::bind_method(D_METHOD("get_shader_code"), &ShaderVideoEffect::get_shader_code);
    ClassDB::add_property("ShaderVideoEffect", PropertyInfo(Variant::STRING, "shader_code", PROPERTY_HINT_MULTILINE_TEXT), "set_shader_code", "get_shader_code");

    ClassDB::bind_method(D_METHOD("set_parameters", "params"), &ShaderVideoEffect::set_parameters);
    ClassDB::bind_method(D_METHOD("get_parameters"), &ShaderVideoEffect::get_parameters);
    ClassDB::add_property("ShaderVideoEffect", PropertyInfo(Variant::DICTIONARY, "parameters"), "set_parameters", "get_parameters");
}

void ShaderVideoEffect::set_shader_code(const String &p_code) {
    shader_code = p_code;
    if (shader.is_valid()) {
        RenderingServer::get_singleton()->shader_set_code(shader, shader_code);
    }
}

String ShaderVideoEffect::get_shader_code() const {
    return shader_code;
}

void ShaderVideoEffect::set_parameters(const Dictionary &p_params) {
    parameters = p_params;
}

Dictionary ShaderVideoEffect::get_parameters() const {
    return parameters;
}

void ShaderVideoEffect::_ensure_resources(RenderingServer *p_rs, int p_width, int p_height) {
    if (viewport.is_valid() && cached_width == p_width && cached_height == p_height) {
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

    shader = p_rs->shader_create();
    p_rs->shader_set_code(shader, shader_code);

    material = p_rs->material_create();
    p_rs->material_set_shader(material, shader);

    p_rs->viewport_attach_canvas(viewport, canvas);

    cached_width = p_width;
    cached_height = p_height;
}

void ShaderVideoEffect::_free_resources() {
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

RID ShaderVideoEffect::apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) {
    _ensure_resources(p_rs, p_width, p_height);

    // Push parameters
    Array keys = parameters.keys();
    for (int i = 0; i < keys.size(); i++) {
        p_rs->material_set_param(material, keys[i], parameters[keys[i]]);
    }

    // Rebuild draw command (input texture changes every frame)
    p_rs->canvas_item_clear(canvas_item);
    p_rs->canvas_item_set_material(canvas_item, material);
    p_rs->canvas_item_add_texture_rect(canvas_item, Rect2(0, 0, p_width, p_height), p_input);

    // Trigger render on next frame
    p_rs->viewport_set_update_mode(viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);

    // Return the viewport's output texture. Godot renders this during the draw phase.
    return p_rs->viewport_get_texture(viewport);
}

Ref<Image> ShaderVideoEffect::apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) {
    // Shaders are GPU-only. For export, use ColorCorrectionEffect or implement async GPU readback.
    UtilityFunctions::push_error("[ShaderVideoEffect] apply_to_image not supported. Use ColorCorrectionEffect for CPU export.");
    return p_input;
}

ShaderVideoEffect::ShaderVideoEffect() {}
ShaderVideoEffect::~ShaderVideoEffect() {
    _free_resources();
}
