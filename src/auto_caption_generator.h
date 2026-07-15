#ifndef AUTO_CAPTION_GENERATOR_H
#define AUTO_CAPTION_GENERATOR_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "caption_segment.h"

namespace godot {

class AutoCaptionGenerator : public RefCounted {
    GDCLASS(AutoCaptionGenerator, RefCounted)

protected:
    static void _bind_methods();

public:
    // Override this in GDScript or C++ subclass to implement STT.
    // Default returns empty array.
    virtual TypedArray<CaptionSegment> generate_captions(const String &p_video_path);

    // Convenience: converts CaptionSegments to TextOverlays with styling
    TypedArray<TextOverlay> generate_text_overlays(const String &p_video_path,
        const Ref<Font> &p_font, int p_font_size, const Color &p_color);

    AutoCaptionGenerator();
};

}

#endif
