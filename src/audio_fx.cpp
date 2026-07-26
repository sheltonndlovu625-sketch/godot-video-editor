#include "audio_fx.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

using namespace godot;

// ============================================================================
// DSP Primitives
// ============================================================================

struct BiquadFilter {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() { z1 = z2 = 0.0f; }

    float process(float input) {
        float output = z1 + b0 * input;
        z1 = z2 + b1 * input - a1 * output;
        z2 = b2 * input - a2 * output;
        return output;
    }

    void set_peaking(float freq, float q, float gain_db, float sample_rate) {
        float A = std::pow(10.0f, gain_db / 40.0f);
        float w0 = 2.0f * M_PI * freq / sample_rate;
        float cos_w0 = std::cos(w0);
        float sin_w0 = std::sin(w0);
        float alpha = sin_w0 / (2.0f * q);
        float b0_ = 1.0f + alpha * A;
        float b1_ = -2.0f * cos_w0;
        float b2_ = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a0_inv = 1.0f / a0;
        b0 = b0_ * a0_inv; b1 = b1_ * a0_inv; b2 = b2_ * a0_inv;
        a1 = (-2.0f * cos_w0) * a0_inv;
        a2 = (1.0f - alpha / A) * a0_inv;
    }

    void set_lowpass(float freq, float q, float sample_rate) {
        float w0 = 2.0f * M_PI * freq / sample_rate;
        float cos_w0 = std::cos(w0);
        float sin_w0 = std::sin(w0);
        float alpha = sin_w0 / (2.0f * q);
        float b0_ = (1.0f - cos_w0) / 2.0f;
        float b1_ = 1.0f - cos_w0;
        float b2_ = (1.0f - cos_w0) / 2.0f;
        float a0 = 1.0f + alpha;
        float a0_inv = 1.0f / a0;
        b0 = b0_ * a0_inv; b1 = b1_ * a0_inv; b2 = b2_ * a0_inv;
        a1 = (-2.0f * cos_w0) * a0_inv;
        a2 = (1.0f - alpha) * a0_inv;
    }

    void set_highpass(float freq, float q, float sample_rate) {
        float w0 = 2.0f * M_PI * freq / sample_rate;
        float cos_w0 = std::cos(w0);
        float sin_w0 = std::sin(w0);
        float alpha = sin_w0 / (2.0f * q);
        float b0_ = (1.0f + cos_w0) / 2.0f;
        float b1_ = -(1.0f + cos_w0);
        float b2_ = (1.0f + cos_w0) / 2.0f;
        float a0 = 1.0f + alpha;
        float a0_inv = 1.0f / a0;
        b0 = b0_ * a0_inv; b1 = b1_ * a0_inv; b2 = b2_ * a0_inv;
        a1 = (-2.0f * cos_w0) * a0_inv;
        a2 = (1.0f - alpha) * a0_inv;
    }
};

struct DelayEngine {
    std::vector<float> buffer;
    size_t write_idx = 0;
    int delay_samples = 0;
    bool initialized = false;

    void init(int sample_rate, float delay_ms) {
        delay_samples = std::max(1, (int)(sample_rate * delay_ms / 1000.0f));
        buffer.assign((size_t)delay_samples + 4, 0.0f);
        write_idx = 0;
        initialized = true;
    }

    float process(float input, float feedback) {
        if (!initialized) return input;
        size_t read_idx = (write_idx + buffer.size() - (size_t)delay_samples) % buffer.size();
        float output = buffer[read_idx];
        buffer[write_idx] = input + output * CLAMP(feedback, 0.0f, 0.98f);
        write_idx = (write_idx + 1) % buffer.size();
        return output;
    }
};

struct ReverbEngine {
    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_ALLPASSES = 4;
    static constexpr int COMB_DELAYS[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static constexpr int ALLPASS_DELAYS[4] = {225, 341, 441, 556};

    std::vector<float> comb_buffer[8];
    size_t comb_idx[8];
    float comb_state[8];

    std::vector<float> allpass_buffer[4];
    size_t allpass_idx[4];

    float feedback = 0.84f;
    float damping = 0.2f;
    bool initialized = false;

    void init(int sample_rate, float scale) {
        float sr_scale = ((float)sample_rate / 44100.0f) * scale;
        for (int i = 0; i < NUM_COMBS; i++) {
            int delay = std::max(1, (int)(COMB_DELAYS[i] * sr_scale));
            comb_buffer[i].assign((size_t)delay, 0.0f);
            comb_idx[i] = 0;
            comb_state[i] = 0.0f;
        }
        for (int i = 0; i < NUM_ALLPASSES; i++) {
            int delay = std::max(1, (int)(ALLPASS_DELAYS[i] * sr_scale));
            allpass_buffer[i].assign((size_t)delay, 0.0f);
            allpass_idx[i] = 0;
        }
        initialized = true;
    }

    void set_params(float room_size, float damping_val) {
        feedback = room_size * 0.28f + 0.7f;
        damping = damping_val;
    }

    float process(float input) {
        if (!initialized) return input;
        float output = 0.0f;
        for (int i = 0; i < NUM_COMBS; i++) {
            size_t idx = comb_idx[i];
            float buf_out = comb_buffer[i][idx];
            comb_state[i] = buf_out * (1.0f - damping) + comb_state[i] * damping;
            comb_buffer[i][idx] = input + comb_state[i] * feedback;
            comb_idx[i] = (idx + 1) % comb_buffer[i].size();
            output += buf_out;
        }
        output *= 0.125f;
        for (int i = 0; i < NUM_ALLPASSES; i++) {
            size_t idx = allpass_idx[i];
            float buf_out = allpass_buffer[i][idx];
            float buf_in = output + buf_out * 0.5f;
            allpass_buffer[i][idx] = buf_in;
            allpass_idx[i] = (idx + 1) % allpass_buffer[i].size();
            output = buf_out - buf_in * 0.5f;
        }
        return output;
    }
};

struct CompressorEngine {
    float envelope = 0.0f;
    float attack_coeff = 0.0f;
    float release_coeff = 0.0f;
    float sample_rate = 44100.0f;
    float threshold_db = 0.0f;
    float ratio = 1.0f;
    float makeup_lin = 1.0f;

    void set_params(float sr, float thresh, float r, float attack_ms, float release_ms, float makeup_db) {
        sample_rate = sr;
        threshold_db = thresh;
        ratio = r;
        makeup_lin = std::pow(10.0f, makeup_db / 20.0f);
        attack_coeff = std::exp(-1.0f / (sr * std::max(0.1f, attack_ms) / 1000.0f));
        release_coeff = std::exp(-1.0f / (sr * std::max(0.1f, release_ms) / 1000.0f));
    }

    float process(float input) {
        float abs_in = std::abs(input);
        if (abs_in > envelope)
            envelope = attack_coeff * envelope + (1.0f - attack_coeff) * abs_in;
        else
            envelope = release_coeff * envelope + (1.0f - release_coeff) * abs_in;

        float threshold_lin = std::pow(10.0f, threshold_db / 20.0f);
        float gain = 1.0f;
        if (envelope > threshold_lin && ratio > 1.0f) {
            float db_over = 20.0f * std::log10(envelope / threshold_lin);
            float db_reduction = db_over * (1.0f - 1.0f / ratio);
            gain = std::pow(10.0f, -db_reduction / 20.0f);
        }
        return input * gain * makeup_lin;
    }
};

struct GateEngine {
    float envelope = 0.0f;
    float attack_coeff = 0.0f;
    float release_coeff = 0.0f;
    float threshold_lin = 0.0f;

    void set_params(float sr, float thresh_db, float attack_ms, float release_ms) {
        threshold_lin = std::pow(10.0f, thresh_db / 20.0f);
        attack_coeff = std::exp(-1.0f / (sr * std::max(0.1f, attack_ms) / 1000.0f));
        release_coeff = std::exp(-1.0f / (sr * std::max(0.1f, release_ms) / 1000.0f));
    }

    float process(float input) {
        float abs_in = std::abs(input);
        if (abs_in > envelope)
            envelope = attack_coeff * envelope + (1.0f - attack_coeff) * abs_in;
        else
            envelope = release_coeff * envelope + (1.0f - release_coeff) * abs_in;

        if (envelope < threshold_lin) {
            float reduction = envelope / threshold_lin;
            return input * reduction * reduction;
        }
        return input;
    }
};

// ============================================================================
// Internal State
// ============================================================================

struct AudioFX::InternalState {
    int sample_rate = 0;
    int channels = 0;
    ReverbEngine reverb[2];
    DelayEngine delay[2];
    BiquadFilter eq_low[2], eq_mid[2], eq_high[2];
    BiquadFilter lpf[2], hpf[2];
    CompressorEngine compressor[2];
    CompressorEngine limiter[2];
    GateEngine gate[2];

    void init(int sr, int ch) {
        sample_rate = sr;
        channels = ch;
        for (int c = 0; c < 2; c++) {
            float scale = (c == 0) ? 1.0f : 1.02f;
            reverb[c].init(sr, scale);
            delay[c].init(sr, 250.0f);
            eq_low[c].reset(); eq_mid[c].reset(); eq_high[c].reset();
            lpf[c].reset(); hpf[c].reset();
            compressor[c].set_params(sr, 0.0f, 1.0f, 5.0f, 100.0f, 0.0f);
            limiter[c].set_params(sr, 0.0f, 100.0f, 1.0f, 50.0f, 0.0f);
            gate[c].set_params(sr, 0.0f, 1.0f, 50.0f);
        }
    }
};

// ============================================================================
// AudioFX Implementation
// ============================================================================

void AudioFX::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_volume_db", "db"), &AudioFX::set_volume_db);
    ClassDB::bind_method(D_METHOD("get_volume_db"), &AudioFX::get_volume_db);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-60.0,24.0,0.1"), "set_volume_db", "get_volume_db");

    ClassDB::bind_method(D_METHOD("set_reverb_enabled", "enabled"), &AudioFX::set_reverb_enabled);
    ClassDB::bind_method(D_METHOD("is_reverb_enabled"), &AudioFX::is_reverb_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "reverb_enabled"), "set_reverb_enabled", "is_reverb_enabled");

    ClassDB::bind_method(D_METHOD("set_reverb_room_size", "size"), &AudioFX::set_reverb_room_size);
    ClassDB::bind_method(D_METHOD("get_reverb_room_size"), &AudioFX::get_reverb_room_size);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "reverb_room_size", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_reverb_room_size", "get_reverb_room_size");

    ClassDB::bind_method(D_METHOD("set_reverb_damping", "damping"), &AudioFX::set_reverb_damping);
    ClassDB::bind_method(D_METHOD("get_reverb_damping"), &AudioFX::get_reverb_damping);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "reverb_damping", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_reverb_damping", "get_reverb_damping");

    ClassDB::bind_method(D_METHOD("set_reverb_wet_level", "level"), &AudioFX::set_reverb_wet_level);
    ClassDB::bind_method(D_METHOD("get_reverb_wet_level"), &AudioFX::get_reverb_wet_level);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "reverb_wet_level", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_reverb_wet_level", "get_reverb_wet_level");

    ClassDB::bind_method(D_METHOD("set_reverb_width", "width"), &AudioFX::set_reverb_width);
    ClassDB::bind_method(D_METHOD("get_reverb_width"), &AudioFX::get_reverb_width);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "reverb_width", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_reverb_width", "get_reverb_width");

    ClassDB::bind_method(D_METHOD("set_delay_enabled", "enabled"), &AudioFX::set_delay_enabled);
    ClassDB::bind_method(D_METHOD("is_delay_enabled"), &AudioFX::is_delay_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "delay_enabled"), "set_delay_enabled", "is_delay_enabled");

    ClassDB::bind_method(D_METHOD("set_delay_time_ms", "ms"), &AudioFX::set_delay_time_ms);
    ClassDB::bind_method(D_METHOD("get_delay_time_ms"), &AudioFX::get_delay_time_ms);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "delay_time_ms", PROPERTY_HINT_RANGE, "1.0,5000.0,1.0"), "set_delay_time_ms", "get_delay_time_ms");

    ClassDB::bind_method(D_METHOD("set_delay_feedback", "feedback"), &AudioFX::set_delay_feedback);
    ClassDB::bind_method(D_METHOD("get_delay_feedback"), &AudioFX::get_delay_feedback);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "delay_feedback", PROPERTY_HINT_RANGE, "0.0,0.98,0.01"), "set_delay_feedback", "get_delay_feedback");

    ClassDB::bind_method(D_METHOD("set_delay_mix", "mix"), &AudioFX::set_delay_mix);
    ClassDB::bind_method(D_METHOD("get_delay_mix"), &AudioFX::get_delay_mix);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "delay_mix", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_delay_mix", "get_delay_mix");

    ClassDB::bind_method(D_METHOD("set_eq_enabled", "enabled"), &AudioFX::set_eq_enabled);
    ClassDB::bind_method(D_METHOD("is_eq_enabled"), &AudioFX::is_eq_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "eq_enabled"), "set_eq_enabled", "is_eq_enabled");

    ClassDB::bind_method(D_METHOD("set_eq_low_gain", "db"), &AudioFX::set_eq_low_gain);
    ClassDB::bind_method(D_METHOD("get_eq_low_gain"), &AudioFX::get_eq_low_gain);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_low_gain", PROPERTY_HINT_RANGE, "-24.0,24.0,0.1"), "set_eq_low_gain", "get_eq_low_gain");

    ClassDB::bind_method(D_METHOD("set_eq_mid_gain", "db"), &AudioFX::set_eq_mid_gain);
    ClassDB::bind_method(D_METHOD("get_eq_mid_gain"), &AudioFX::get_eq_mid_gain);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_mid_gain", PROPERTY_HINT_RANGE, "-24.0,24.0,0.1"), "set_eq_mid_gain", "get_eq_mid_gain");

    ClassDB::bind_method(D_METHOD("set_eq_high_gain", "db"), &AudioFX::set_eq_high_gain);
    ClassDB::bind_method(D_METHOD("get_eq_high_gain"), &AudioFX::get_eq_high_gain);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_high_gain", PROPERTY_HINT_RANGE, "-24.0,24.0,0.1"), "set_eq_high_gain", "get_eq_high_gain");

    ClassDB::bind_method(D_METHOD("set_eq_low_freq", "hz"), &AudioFX::set_eq_low_freq);
    ClassDB::bind_method(D_METHOD("get_eq_low_freq"), &AudioFX::get_eq_low_freq);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_low_freq", PROPERTY_HINT_RANGE, "20.0,1000.0,1.0"), "set_eq_low_freq", "get_eq_low_freq");

    ClassDB::bind_method(D_METHOD("set_eq_mid_freq", "hz"), &AudioFX::set_eq_mid_freq);
    ClassDB::bind_method(D_METHOD("get_eq_mid_freq"), &AudioFX::get_eq_mid_freq);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_mid_freq", PROPERTY_HINT_RANGE, "200.0,8000.0,1.0"), "set_eq_mid_freq", "get_eq_mid_freq");

    ClassDB::bind_method(D_METHOD("set_eq_high_freq", "hz"), &AudioFX::set_eq_high_freq);
    ClassDB::bind_method(D_METHOD("get_eq_high_freq"), &AudioFX::get_eq_high_freq);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "eq_high_freq", PROPERTY_HINT_RANGE, "1000.0,20000.0,1.0"), "set_eq_high_freq", "get_eq_high_freq");

    ClassDB::bind_method(D_METHOD("set_compressor_enabled", "enabled"), &AudioFX::set_compressor_enabled);
    ClassDB::bind_method(D_METHOD("is_compressor_enabled"), &AudioFX::is_compressor_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "compressor_enabled"), "set_compressor_enabled", "is_compressor_enabled");

    ClassDB::bind_method(D_METHOD("set_compressor_threshold", "db"), &AudioFX::set_compressor_threshold);
    ClassDB::bind_method(D_METHOD("get_compressor_threshold"), &AudioFX::get_compressor_threshold);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "compressor_threshold", PROPERTY_HINT_RANGE, "-60.0,0.0,0.1"), "set_compressor_threshold", "get_compressor_threshold");

    ClassDB::bind_method(D_METHOD("set_compressor_ratio", "ratio"), &AudioFX::set_compressor_ratio);
    ClassDB::bind_method(D_METHOD("get_compressor_ratio"), &AudioFX::get_compressor_ratio);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "compressor_ratio", PROPERTY_HINT_RANGE, "1.0,30.0,0.1"), "set_compressor_ratio", "get_compressor_ratio");

    ClassDB::bind_method(D_METHOD("set_compressor_attack_ms", "ms"), &AudioFX::set_compressor_attack_ms);
    ClassDB::bind_method(D_METHOD("get_compressor_attack_ms"), &AudioFX::get_compressor_attack_ms);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "compressor_attack_ms", PROPERTY_HINT_RANGE, "0.1,200.0,0.1"), "set_compressor_attack_ms", "get_compressor_attack_ms");

    ClassDB::bind_method(D_METHOD("set_compressor_release_ms", "ms"), &AudioFX::set_compressor_release_ms);
    ClassDB::bind_method(D_METHOD("get_compressor_release_ms"), &AudioFX::get_compressor_release_ms);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "compressor_release_ms", PROPERTY_HINT_RANGE, "1.0,2000.0,1.0"), "set_compressor_release_ms", "get_compressor_release_ms");

    ClassDB::bind_method(D_METHOD("set_compressor_makeup_db", "db"), &AudioFX::set_compressor_makeup_db);
    ClassDB::bind_method(D_METHOD("get_compressor_makeup_db"), &AudioFX::get_compressor_makeup_db);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "compressor_makeup_db", PROPERTY_HINT_RANGE, "-12.0,24.0,0.1"), "set_compressor_makeup_db", "get_compressor_makeup_db");

    ClassDB::bind_method(D_METHOD("set_lowpass_enabled", "enabled"), &AudioFX::set_lowpass_enabled);
    ClassDB::bind_method(D_METHOD("is_lowpass_enabled"), &AudioFX::is_lowpass_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "lowpass_enabled"), "set_lowpass_enabled", "is_lowpass_enabled");

    ClassDB::bind_method(D_METHOD("set_lowpass_cutoff", "hz"), &AudioFX::set_lowpass_cutoff);
    ClassDB::bind_method(D_METHOD("get_lowpass_cutoff"), &AudioFX::get_lowpass_cutoff);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "lowpass_cutoff", PROPERTY_HINT_RANGE, "20.0,20000.0,1.0"), "set_lowpass_cutoff", "get_lowpass_cutoff");

    ClassDB::bind_method(D_METHOD("set_highpass_enabled", "enabled"), &AudioFX::set_highpass_enabled);
    ClassDB::bind_method(D_METHOD("is_highpass_enabled"), &AudioFX::is_highpass_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "highpass_enabled"), "set_highpass_enabled", "is_highpass_enabled");

    ClassDB::bind_method(D_METHOD("set_highpass_cutoff", "hz"), &AudioFX::set_highpass_cutoff);
    ClassDB::bind_method(D_METHOD("get_highpass_cutoff"), &AudioFX::get_highpass_cutoff);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "highpass_cutoff", PROPERTY_HINT_RANGE, "20.0,20000.0,1.0"), "set_highpass_cutoff", "get_highpass_cutoff");

    ClassDB::bind_method(D_METHOD("set_limiter_enabled", "enabled"), &AudioFX::set_limiter_enabled);
    ClassDB::bind_method(D_METHOD("is_limiter_enabled"), &AudioFX::is_limiter_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "limiter_enabled"), "set_limiter_enabled", "is_limiter_enabled");

    ClassDB::bind_method(D_METHOD("set_limiter_threshold", "db"), &AudioFX::set_limiter_threshold);
    ClassDB::bind_method(D_METHOD("get_limiter_threshold"), &AudioFX::get_limiter_threshold);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "limiter_threshold", PROPERTY_HINT_RANGE, "-30.0,0.0,0.1"), "set_limiter_threshold", "get_limiter_threshold");

    ClassDB::bind_method(D_METHOD("set_gate_enabled", "enabled"), &AudioFX::set_gate_enabled);
    ClassDB::bind_method(D_METHOD("is_gate_enabled"), &AudioFX::is_gate_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "gate_enabled"), "set_gate_enabled", "is_gate_enabled");

    ClassDB::bind_method(D_METHOD("set_gate_threshold", "db"), &AudioFX::set_gate_threshold);
    ClassDB::bind_method(D_METHOD("get_gate_threshold"), &AudioFX::get_gate_threshold);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "gate_threshold", PROPERTY_HINT_RANGE, "-80.0,0.0,0.1"), "set_gate_threshold", "get_gate_threshold");

    ClassDB::bind_method(D_METHOD("set_stereo_width_enabled", "enabled"), &AudioFX::set_stereo_width_enabled);
    ClassDB::bind_method(D_METHOD("is_stereo_width_enabled"), &AudioFX::is_stereo_width_enabled);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::BOOL, "stereo_width_enabled"), "set_stereo_width_enabled", "is_stereo_width_enabled");

    ClassDB::bind_method(D_METHOD("set_stereo_width", "width"), &AudioFX::set_stereo_width);
    ClassDB::bind_method(D_METHOD("get_stereo_width"), &AudioFX::get_stereo_width);
    ClassDB::add_property("AudioFX", PropertyInfo(Variant::FLOAT, "stereo_width", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"), "set_stereo_width", "get_stereo_width");

    ClassDB::bind_method(D_METHOD("process_audio", "buffer", "sample_rate", "channels"), &AudioFX::process_audio);
    ClassDB::bind_method(D_METHOD("get_ffmpeg_filter_string"), &AudioFX::get_ffmpeg_filter_string);
}

AudioFX::AudioFX() {}
AudioFX::~AudioFX() { _clear_state(); }

void AudioFX::_ensure_state(int p_sample_rate, int p_channels) {
    if (!state || state->sample_rate != p_sample_rate || state->channels != p_channels) {
        _clear_state();
        state = new InternalState();
        state->init(p_sample_rate, p_channels);
    }
}

void AudioFX::_clear_state() {
    if (state) {
        delete state;
        state = nullptr;
    }
}

// ── Volume ──
void AudioFX::set_volume_db(float p_db) { volume_db = p_db; }
float AudioFX::get_volume_db() const { return volume_db; }

// ── Reverb ──
void AudioFX::set_reverb_enabled(bool p_enabled) { reverb_enabled = p_enabled; }
bool AudioFX::is_reverb_enabled() const { return reverb_enabled; }
void AudioFX::set_reverb_room_size(float p_size) { reverb_room_size = CLAMP(p_size, 0.0f, 1.0f); }
float AudioFX::get_reverb_room_size() const { return reverb_room_size; }
void AudioFX::set_reverb_damping(float p_damping) { reverb_damping = CLAMP(p_damping, 0.0f, 1.0f); }
float AudioFX::get_reverb_damping() const { return reverb_damping; }
void AudioFX::set_reverb_wet_level(float p_level) { reverb_wet_level = CLAMP(p_level, 0.0f, 1.0f); }
float AudioFX::get_reverb_wet_level() const { return reverb_wet_level; }
void AudioFX::set_reverb_width(float p_width) { reverb_width = CLAMP(p_width, 0.0f, 1.0f); }
float AudioFX::get_reverb_width() const { return reverb_width; }

// ── Delay ──
void AudioFX::set_delay_enabled(bool p_enabled) { delay_enabled = p_enabled; }
bool AudioFX::is_delay_enabled() const { return delay_enabled; }
void AudioFX::set_delay_time_ms(float p_ms) { delay_time_ms = std::max(1.0f, p_ms); }
float AudioFX::get_delay_time_ms() const { return delay_time_ms; }
void AudioFX::set_delay_feedback(float p_feedback) { delay_feedback = CLAMP(p_feedback, 0.0f, 0.98f); }
float AudioFX::get_delay_feedback() const { return delay_feedback; }
void AudioFX::set_delay_mix(float p_mix) { delay_mix = CLAMP(p_mix, 0.0f, 1.0f); }
float AudioFX::get_delay_mix() const { return delay_mix; }

// ── EQ ──
void AudioFX::set_eq_enabled(bool p_enabled) { eq_enabled = p_enabled; }
bool AudioFX::is_eq_enabled() const { return eq_enabled; }
void AudioFX::set_eq_low_gain(float p_db) { eq_low_gain = p_db; }
float AudioFX::get_eq_low_gain() const { return eq_low_gain; }
void AudioFX::set_eq_mid_gain(float p_db) { eq_mid_gain = p_db; }
float AudioFX::get_eq_mid_gain() const { return eq_mid_gain; }
void AudioFX::set_eq_high_gain(float p_db) { eq_high_gain = p_db; }
float AudioFX::get_eq_high_gain() const { return eq_high_gain; }
void AudioFX::set_eq_low_freq(float p_hz) { eq_low_freq = std::max(20.0f, p_hz); }
float AudioFX::get_eq_low_freq() const { return eq_low_freq; }
void AudioFX::set_eq_mid_freq(float p_hz) { eq_mid_freq = std::max(20.0f, p_hz); }
float AudioFX::get_eq_mid_freq() const { return eq_mid_freq; }
void AudioFX::set_eq_high_freq(float p_hz) { eq_high_freq = std::max(20.0f, p_hz); }
float AudioFX::get_eq_high_freq() const { return eq_high_freq; }

// ── Compressor ──
void AudioFX::set_compressor_enabled(bool p_enabled) { compressor_enabled = p_enabled; }
bool AudioFX::is_compressor_enabled() const { return compressor_enabled; }
void AudioFX::set_compressor_threshold(float p_db) { compressor_threshold = p_db; }
float AudioFX::get_compressor_threshold() const { return compressor_threshold; }
void AudioFX::set_compressor_ratio(float p_ratio) { compressor_ratio = std::max(1.0f, p_ratio); }
float AudioFX::get_compressor_ratio() const { return compressor_ratio; }
void AudioFX::set_compressor_attack_ms(float p_ms) { compressor_attack_ms = std::max(0.1f, p_ms); }
float AudioFX::get_compressor_attack_ms() const { return compressor_attack_ms; }
void AudioFX::set_compressor_release_ms(float p_ms) { compressor_release_ms = std::max(1.0f, p_ms); }
float AudioFX::get_compressor_release_ms() const { return compressor_release_ms; }
void AudioFX::set_compressor_makeup_db(float p_db) { compressor_makeup_db = p_db; }
float AudioFX::get_compressor_makeup_db() const { return compressor_makeup_db; }

// ── Filters ──
void AudioFX::set_lowpass_enabled(bool p_enabled) { lowpass_enabled = p_enabled; }
bool AudioFX::is_lowpass_enabled() const { return lowpass_enabled; }
void AudioFX::set_lowpass_cutoff(float p_hz) { lowpass_cutoff = CLAMP(p_hz, 20.0f, 20000.0f); }
float AudioFX::get_lowpass_cutoff() const { return lowpass_cutoff; }
void AudioFX::set_highpass_enabled(bool p_enabled) { highpass_enabled = p_enabled; }
bool AudioFX::is_highpass_enabled() const { return highpass_enabled; }
void AudioFX::set_highpass_cutoff(float p_hz) { highpass_cutoff = CLAMP(p_hz, 20.0f, 20000.0f); }
float AudioFX::get_highpass_cutoff() const { return highpass_cutoff; }

// ── Limiter ──
void AudioFX::set_limiter_enabled(bool p_enabled) { limiter_enabled = p_enabled; }
bool AudioFX::is_limiter_enabled() const { return limiter_enabled; }
void AudioFX::set_limiter_threshold(float p_db) { limiter_threshold = CLAMP(p_db, -60.0f, 0.0f); }
float AudioFX::get_limiter_threshold() const { return limiter_threshold; }

// ── Gate ──
void AudioFX::set_gate_enabled(bool p_enabled) { gate_enabled = p_enabled; }
bool AudioFX::is_gate_enabled() const { return gate_enabled; }
void AudioFX::set_gate_threshold(float p_db) { gate_threshold = CLAMP(p_db, -100.0f, 0.0f); }
float AudioFX::get_gate_threshold() const { return gate_threshold; }

// ── Stereo Width ──
void AudioFX::set_stereo_width_enabled(bool p_enabled) { stereo_width_enabled = p_enabled; }
bool AudioFX::is_stereo_width_enabled() const { return stereo_width_enabled; }
void AudioFX::set_stereo_width(float p_width) { stereo_width = CLAMP(p_width, 0.0f, 2.0f); }
float AudioFX::get_stereo_width() const { return stereo_width; }

// ============================================================================
// Audio Processing
// ============================================================================

PackedFloat32Array AudioFX::process_audio(const PackedFloat32Array &p_buffer, int p_sample_rate, int p_channels) {
    if (p_buffer.is_empty() || p_channels <= 0) return p_buffer;
    _ensure_state(p_sample_rate, p_channels);

    PackedFloat32Array output = p_buffer.duplicate();
    int total_samples = output.size();
    int num_frames = total_samples / p_channels;
    if (num_frames == 0) return output;

    float vol_lin = std::pow(10.0f, volume_db / 20.0f);

    for (int c = 0; c < p_channels && c < 2; c++) {
        state->reverb[c].set_params(reverb_room_size, reverb_damping);
    }

    if (eq_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->eq_low[c].set_peaking(eq_low_freq, 0.707f, eq_low_gain, p_sample_rate);
            state->eq_mid[c].set_peaking(eq_mid_freq, 0.707f, eq_mid_gain, p_sample_rate);
            state->eq_high[c].set_peaking(eq_high_freq, 0.707f, eq_high_gain, p_sample_rate);
        }
    }
    if (lowpass_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->lpf[c].set_lowpass(lowpass_cutoff, 0.707f, p_sample_rate);
        }
    }
    if (highpass_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->hpf[c].set_highpass(highpass_cutoff, 0.707f, p_sample_rate);
        }
    }
    if (compressor_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->compressor[c].set_params(p_sample_rate, compressor_threshold, compressor_ratio,
                compressor_attack_ms, compressor_release_ms, compressor_makeup_db);
        }
    }
    if (limiter_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->limiter[c].set_params(p_sample_rate, limiter_threshold, 100.0f, 1.0f, 50.0f, 0.0f);
        }
    }
    if (gate_enabled) {
        for (int c = 0; c < p_channels && c < 2; c++) {
            state->gate[c].set_params(p_sample_rate, gate_threshold, 1.0f, 50.0f);
        }
    }

    for (int i = 0; i < num_frames; i++) {
        for (int c = 0; c < p_channels; c++) {
            int idx = i * p_channels + c;
            float sample = output[idx];

            sample *= vol_lin;

            if (highpass_enabled && c < 2) {
                sample = state->hpf[c].process(sample);
            }
            if (lowpass_enabled && c < 2) {
                sample = state->lpf[c].process(sample);
            }
            if (eq_enabled && c < 2) {
                sample = state->eq_low[c].process(sample);
                sample = state->eq_mid[c].process(sample);
                sample = state->eq_high[c].process(sample);
            }
            if (compressor_enabled && c < 2) {
                sample = state->compressor[c].process(sample);
            }
            if (reverb_enabled && c < 2) {
                float wet = state->reverb[c].process(sample);
                sample = sample * (1.0f - reverb_wet_level) + wet * reverb_wet_level;
            }
            if (delay_enabled && c < 2) {
                float wet = state->delay[c].process(sample, delay_feedback);
                sample = sample * (1.0f - delay_mix) + wet * delay_mix;
            }
            if (limiter_enabled && c < 2) {
                sample = state->limiter[c].process(sample);
            }
            if (gate_enabled && c < 2) {
                sample = state->gate[c].process(sample);
            }

            output[idx] = sample;
        }

        if (stereo_width_enabled && p_channels >= 2) {
            float left = output[i * p_channels];
            float right = output[i * p_channels + 1];
            float mid = (left + right) * 0.5f;
            float side = (right - left) * 0.5f;
            side *= stereo_width;
            output[i * p_channels] = mid - side;
            output[i * p_channels + 1] = mid + side;
        }
    }

    return output;
}

// ============================================================================
// FFmpeg Filter String
// ============================================================================

String AudioFX::get_ffmpeg_filter_string() const {
    String filter = "";

    // Volume
    if (volume_db != 0.0f) {
        filter += "volume=" + String::num(volume_db) + "dB";
    }

    // High Pass
    if (highpass_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "highpass=f=" + String::num(highpass_cutoff);
    }

    // Low Pass
    if (lowpass_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "lowpass=f=" + String::num(lowpass_cutoff);
    }

    // EQ
    if (eq_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "equalizer=f=" + String::num(eq_low_freq) + ":width_type=o:width=1:g=" + String::num(eq_low_gain);
        filter += ",equalizer=f=" + String::num(eq_mid_freq) + ":width_type=o:width=1:g=" + String::num(eq_mid_gain);
        filter += ",equalizer=f=" + String::num(eq_high_freq) + ":width_type=o:width=1:g=" + String::num(eq_high_gain);
    }

    // Compressor
    if (compressor_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "acompressor=threshold=" + String::num(compressor_threshold) + "dB";
        filter += ":ratio=" + String::num(compressor_ratio);
        filter += ":attack=" + String::num(compressor_attack_ms);
        filter += ":release=" + String::num(compressor_release_ms);
        if (compressor_makeup_db != 0.0f) {
            filter += ":makeup=" + String::num(compressor_makeup_db);
        }
    }

    // Reverb (multi-tap aecho)
    if (reverb_enabled) {
        if (!filter.is_empty()) filter += ",";
        float in_gain = 1.0f - (reverb_wet_level * 0.5f);
        float out_gain = 1.0f;
        int d1 = (int)(20.0f + reverb_room_size * 40.0f);
        int d2 = (int)(40.0f + reverb_room_size * 60.0f);
        int d3 = (int)(60.0f + reverb_room_size * 80.0f);
        int d4 = (int)(80.0f + reverb_room_size * 100.0f);
        float dc1 = 0.4f + reverb_room_size * 0.3f - reverb_damping * 0.2f;
        float dc2 = 0.3f + reverb_room_size * 0.2f - reverb_damping * 0.15f;
        float dc3 = 0.2f + reverb_room_size * 0.15f - reverb_damping * 0.1f;
        float dc4 = 0.1f + reverb_room_size * 0.1f - reverb_damping * 0.05f;
        filter += "aecho=" + String::num(in_gain) + ":" + String::num(out_gain);
        filter += ":" + String::num(d1) + "|" + String::num(d2) + "|" + String::num(d3) + "|" + String::num(d4);
        filter += ":" + String::num(dc1) + "|" + String::num(dc2) + "|" + String::num(dc3) + "|" + String::num(dc4);
    }

    // Delay
    if (delay_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "aecho=1.0:1.0:" + String::num((int)delay_time_ms) + ":" + String::num(delay_feedback);
    }

    // Limiter
    if (limiter_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "alimiter=level_in=1:level_out=1:limit=" + String::num(limiter_threshold) + "dB:attack=5:release=50";
    }

    // Gate
    if (gate_enabled) {
        if (!filter.is_empty()) filter += ",";
        filter += "agate=threshold=" + String::num(gate_threshold) + "dB:attack=10:release=100";
    }

    // Stereo Width
    if (stereo_width_enabled && stereo_width != 1.0f) {
        if (!filter.is_empty()) filter += ",";
        float m = CLAMP(stereo_width, 0.0f, 2.0f);
        filter += "extrastereo=m=" + String::num(m);
    }

    if (filter.is_empty()) {
        filter = "anull";
    }

    return filter;
}
