#ifndef AUDIO_FX_H
#define AUDIO_FX_H

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class AudioFX : public Resource {
    GDCLASS(AudioFX, Resource)

private:
    // ── Volume ──
    float volume_db = 0.0f;

    // ── Reverb (Freeverb-style Schroeder) ──
    bool reverb_enabled = false;
    float reverb_room_size = 0.5f;   // 0..1
    float reverb_damping = 0.5f;     // 0..1
    float reverb_wet_level = 0.3f;   // 0..1
    float reverb_width = 0.5f;       // 0..1 (stereo spread)

    // ── Delay ──
    bool delay_enabled = false;
    float delay_time_ms = 250.0f;
    float delay_feedback = 0.3f;     // 0..0.95
    float delay_mix = 0.3f;          // 0..1

    // ── 3-Band EQ ──
    bool eq_enabled = false;
    float eq_low_gain = 0.0f;        // dB
    float eq_mid_gain = 0.0f;
    float eq_high_gain = 0.0f;
    float eq_low_freq = 250.0f;      // Hz
    float eq_mid_freq = 1000.0f;     // Hz
    float eq_high_freq = 4000.0f;    // Hz

    // ── Compressor ──
    bool compressor_enabled = false;
    float compressor_threshold = -20.0f; // dB
    float compressor_ratio = 4.0f;
    float compressor_attack_ms = 5.0f;
    float compressor_release_ms = 100.0f;
    float compressor_makeup_db = 0.0f;

    // ── Filters ──
    bool lowpass_enabled = false;
    float lowpass_cutoff = 20000.0f; // Hz
    bool highpass_enabled = false;
    float highpass_cutoff = 20.0f;   // Hz

    // ── Limiter ──
    bool limiter_enabled = false;
    float limiter_threshold = -1.0f; // dB

    // ── Noise Gate ──
    bool gate_enabled = false;
    float gate_threshold = -50.0f;   // dB

    // ── Stereo Width ──
    bool stereo_width_enabled = false;
    float stereo_width = 1.0f;       // 0=mono, 1=normal, 2=wide

    // Internal DSP state (opaque pointer to keep header clean)
    struct InternalState;
    InternalState *state = nullptr;

    void _ensure_state(int p_sample_rate, int p_channels);
    void _clear_state();

protected:
    static void _bind_methods();

public:
    AudioFX();
    ~AudioFX();

    // Volume
    void set_volume_db(float p_db);
    float get_volume_db() const;

    // Reverb
    void set_reverb_enabled(bool p_enabled);
    bool is_reverb_enabled() const;
    void set_reverb_room_size(float p_size);
    float get_reverb_room_size() const;
    void set_reverb_damping(float p_damping);
    float get_reverb_damping() const;
    void set_reverb_wet_level(float p_level);
    float get_reverb_wet_level() const;
    void set_reverb_width(float p_width);
    float get_reverb_width() const;

    // Delay
    void set_delay_enabled(bool p_enabled);
    bool is_delay_enabled() const;
    void set_delay_time_ms(float p_ms);
    float get_delay_time_ms() const;
    void set_delay_feedback(float p_feedback);
    float get_delay_feedback() const;
    void set_delay_mix(float p_mix);
    float get_delay_mix() const;

    // EQ
    void set_eq_enabled(bool p_enabled);
    bool is_eq_enabled() const;
    void set_eq_low_gain(float p_db);
    float get_eq_low_gain() const;
    void set_eq_mid_gain(float p_db);
    float get_eq_mid_gain() const;
    void set_eq_high_gain(float p_db);
    float get_eq_high_gain() const;
    void set_eq_low_freq(float p_hz);
    float get_eq_low_freq() const;
    void set_eq_mid_freq(float p_hz);
    float get_eq_mid_freq() const;
    void set_eq_high_freq(float p_hz);
    float get_eq_high_freq() const;

    // Compressor
    void set_compressor_enabled(bool p_enabled);
    bool is_compressor_enabled() const;
    void set_compressor_threshold(float p_db);
    float get_compressor_threshold() const;
    void set_compressor_ratio(float p_ratio);
    float get_compressor_ratio() const;
    void set_compressor_attack_ms(float p_ms);
    float get_compressor_attack_ms() const;
    void set_compressor_release_ms(float p_ms);
    float get_compressor_release_ms() const;
    void set_compressor_makeup_db(float p_db);
    float get_compressor_makeup_db() const;

    // Filters
    void set_lowpass_enabled(bool p_enabled);
    bool is_lowpass_enabled() const;
    void set_lowpass_cutoff(float p_hz);
    float get_lowpass_cutoff() const;
    void set_highpass_enabled(bool p_enabled);
    bool is_highpass_enabled() const;
    void set_highpass_cutoff(float p_hz);
    float get_highpass_cutoff() const;

    // Limiter
    void set_limiter_enabled(bool p_enabled);
    bool is_limiter_enabled() const;
    void set_limiter_threshold(float p_db);
    float get_limiter_threshold() const;

    // Gate
    void set_gate_enabled(bool p_enabled);
    bool is_gate_enabled() const;
    void set_gate_threshold(float p_db);
    float get_gate_threshold() const;

    // Stereo Width
    void set_stereo_width_enabled(bool p_enabled);
    bool is_stereo_width_enabled() const;
    void set_stereo_width(float p_width);
    float get_stereo_width() const;

    // Process audio buffer (real-time preview path)
    PackedFloat32Array process_audio(const PackedFloat32Array &p_buffer, int p_sample_rate, int p_channels);

    // Build FFmpeg filter string (export/offline path)
    String get_ffmpeg_filter_string() const;
};

}

#endif
