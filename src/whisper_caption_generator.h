#ifndef WHISPER_CAPTION_GENERATOR_H
#define WHISPER_CAPTION_GENERATOR_H

#include "auto_caption_generator.h"
#include <whisper.h>

namespace godot {

class WhisperCaptionGenerator : public AutoCaptionGenerator {
    GDCLASS(WhisperCaptionGenerator, AutoCaptionGenerator)

private:
    whisper_context *ctx = nullptr;
    String model_path;

protected:
    static void _bind_methods();

public:
    void set_model_path(const String &p_path);
    String get_model_path() const;

    virtual TypedArray<CaptionSegment> generate_captions(const String &p_video_path) override;

    WhisperCaptionGenerator();
    ~WhisperCaptionGenerator();
};

}

#endif
