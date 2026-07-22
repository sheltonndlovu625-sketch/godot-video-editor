// audio_waveform.cpp
#include "audio_waveform.h"
#include "video_decoder.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void AudioWaveform::_bind_methods() {
    ClassDB::bind_method(D_METHOD("generate_waveform", "samples", "width", "height", "channels", "color", "bg_color"), &AudioWaveform::generate_waveform);
    ClassDB::bind_method(D_METHOD("generate_waveform_from_decoder", "decoder", "start_time", "duration", "width", "height", "color", "bg_color"), &AudioWaveform::generate_waveform_from_decoder);
}

AudioWaveform::AudioWaveform() {}

Ref<Image> AudioWaveform::generate_waveform(const PackedFloat32Array &p_samples, int p_width, int p_height, int p_channels, const Color &p_color, const Color &p_bg_color) const {
    if (p_samples.is_empty() || p_width <= 0 || p_height <= 0 || p_channels <= 0) return Ref<Image>();

    Ref<Image> img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
    if (img.is_null()) return Ref<Image>();
    img->fill(p_bg_color);

    int total_samples = p_samples.size() / p_channels;
    int samples_per_pixel = total_samples / p_width;
    if (samples_per_pixel < 1) samples_per_pixel = 1;

    uint8_t *data = img->ptrw();
    int stride = p_width * 4;
    int half_h = p_height / 2;

    for (int x = 0; x < p_width; x++) {
        float min_val = 0.0f;
        float max_val = 0.0f;
        int start_idx = x * samples_per_pixel * p_channels;
        // FIX: explicit cast to int64_t so both args match Math::min<T>
        int64_t end_idx = Math::min((int64_t)(start_idx + samples_per_pixel * p_channels), p_samples.size());

        for (int i = start_idx; i < (int)end_idx; i += p_channels) {
            float sample = p_samples[i];
            if (sample < min_val) min_val = sample;
            if (sample > max_val) max_val = sample;
        }

        int y_min = half_h + (int)(min_val * half_h);
        int y_max = half_h + (int)(max_val * half_h);
        y_min = CLAMP(y_min, 0, p_height - 1);
        y_max = CLAMP(y_max, 0, p_height - 1);

        for (int y = y_min; y <= y_max; y++) {
            uint8_t *pixel = data + y * stride + x * 4;
            pixel[0] = (uint8_t)(p_color.r * 255);
            pixel[1] = (uint8_t)(p_color.g * 255);
            pixel[2] = (uint8_t)(p_color.b * 255);
            pixel[3] = (uint8_t)(p_color.a * 255);
        }
    }
    return img;
}

Ref<Image> AudioWaveform::generate_waveform_from_decoder(const Ref<VideoDecoder> &p_decoder, double p_start_time, double p_duration, int p_width, int p_height, const Color &p_color, const Color &p_bg_color) const {
    if (p_decoder.is_null() || !p_decoder->has_audio()) return Ref<Image>();

    p_decoder->seek(p_start_time);
    int samples_needed = (int)(p_duration * p_decoder->get_audio_sample_rate() * p_decoder->get_audio_channels());
    PackedFloat32Array samples = p_decoder->read_audio_samples(samples_needed);
    return generate_waveform(samples, p_width, p_height, p_decoder->get_audio_channels(), p_color, p_bg_color);
}
