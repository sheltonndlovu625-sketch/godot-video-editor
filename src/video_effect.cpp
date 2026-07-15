#include "video_effect.h"

using namespace godot;

void VideoEffect::_bind_methods() {
    ClassDB::bind_method(D_METHOD("apply_to_image", "input", "width", "height"), &VideoEffect::apply_to_image);
}

Ref<Image> VideoEffect::apply_to_image(const Ref<Image> &p_input, int p_width, int p_height) {
    // Default: pass-through
    return p_input;
}

RID VideoEffect::apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height) {
    // Default: pass-through
    return p_input;
}

VideoEffect::VideoEffect() {}
VideoEffect::~VideoEffect() {}
