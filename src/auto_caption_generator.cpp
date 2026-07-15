#include "auto_caption_generator.h"
#include "text_overlay.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void AutoCaptionGenerator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate_captions", "video_path"), &AutoCaptionGenerator::generate_captions);
    ClassDB::bind_method(D_METHOD("generate_text_overlays", "video_path", "font", "font_size", "color"), &AutoCaptionGenerator::generate_text_overlays);
}

AutoCaptionGenerator::AutoCaptionGenerator() {}

TypedArray<CaptionSegment> AutoCaptionGenerator::generate_captions(const String &p_video_path) {
    // Default: no-op. Subclass this and override generate_captions to integrate:
    //   - whisper.cpp (on-device, no network)
    //   - Google Speech-to-Text API (cloud, accurate)
    //   - Android SpeechRecognizer (platform native)
    //   - iOS SFSpeechRecognizer (platform native)
    UtilityFunctions::print("[AutoCaptionGenerator] Default implementation returns empty. Subclass and override generate_captions() to add STT.");
    return TypedArray<CaptionSegment>();
}

TypedArray<TextOverlay> AutoCaptionGenerator::generate_text_overlays(const String &p_video_path,
    const Ref<Font> &p_font, int p_font_size, const Color &p_color) {

    TypedArray<CaptionSegment> segments = generate_captions(p_video_path);
    TypedArray<TextOverlay> overlays;

    for (int i = 0; i < segments.size(); i++) {
        Ref<CaptionSegment> seg = segments[i];
        if (seg.is_null()) continue;

        Ref<TextOverlay> overlay;
        overlay.instantiate();
        overlay->set_text(seg->get_text());
        overlay->set_start_time(seg->get_start_time());
        overlay->set_end_time(seg->get_end_time());
        overlay->set_font(p_font);
        overlay->set_font_size(p_font_size);
        overlay->set_color(p_color);
        overlay->set_outline_size(2);
        overlay->set_outline_color(Color(0, 0, 0, 1));
        overlay->set_position(Vector2(540, 1600));  // Bottom-center for 1080x1920
        overlay->set_anchor_point(Vector2(0.5, 1.0));

        overlays.push_back(overlay);
    }

    return overlays;
}
