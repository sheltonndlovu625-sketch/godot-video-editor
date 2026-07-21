// audio_waveform.h
#ifndef AUDIO_WAVEFORM_H
#define AUDIO_WAVEFORM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {

class VideoDecoder; // forward

class AudioWaveform : public RefCounted {
    GDCLASS(AudioWaveform, RefCounted)

protected:
    static void _bind_methods();

public:
    Ref<Image> generate_waveform(const PackedFloat32Array &p_samples, int p_width, int p_height, int p_channels, const Color &p_color, const Color &p_bg_color) const;
    Ref<Image> generate_waveform_from_decoder(const Ref<VideoDecoder> &p_decoder, double p_start_time, double p_duration, int p_width, int p_height, const Color &p_color, const Color &p_bg_color) const;

    AudioWaveform();
};

}

#endif
