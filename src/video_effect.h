#ifndef VIDEO_EFFECT_H
#define VIDEO_EFFECT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {

class VideoEffect : public RefCounted {
    GDCLASS(VideoEffect, RefCounted)

protected:
    static void _bind_methods();

public:
    // CPU path: blocking, used for export/offline rendering
    virtual Ref<Image> apply_to_image(const Ref<Image> &p_input, int p_width, int p_height);

    // GPU path: returns a texture RID. Input is a texture RID.
    // The returned RID may be owned by the effect (viewport texture) or passed through.
    virtual RID apply_to_texture(RenderingServer *p_rs, RID p_input, int p_width, int p_height);

    VideoEffect();
    virtual ~VideoEffect();
};

}

#endif
