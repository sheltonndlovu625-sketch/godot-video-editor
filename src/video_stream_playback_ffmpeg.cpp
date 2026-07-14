#include "video_stream_playback_ffmpeg.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_function.hpp>

using namespace godot;

void VideoStreamPlaybackFFmpeg::_bind_methods() {}

VideoStreamPlaybackFFmpeg::VideoStreamPlaybackFFmpeg() {}
VideoStreamPlaybackFFmpeg::~VideoStreamPlaybackFFmpeg() {
    if (video_decoder.is_valid()) video_decoder->close();
    if (audio_decoder.is_valid()) audio_decoder->close();
}

void VideoStreamPlaybackFFmpeg::setup(const Vector<Clip> &p_clips, double p_total_duration) {
    clips = p_clips;
    total_duration = p_total_duration;
}

int VideoStreamPlaybackFFmpeg::_find_clip(double p_time) const {
    for (int i = clips.size() - 1; i >= 0; i--) {
        if (p_time >= clips[i].start_time) {
            return i;
        }
    }
    return -1;
}

void VideoStreamPlaybackFFmpeg::_switch_clip(int p_idx) {
    if (video_decoder.is_valid()) {
        video_decoder->close();
        video_decoder = Ref<VideoDecoder>();
    }
    if (audio_decoder.is_valid()) {
        audio_decoder->close();
        audio_decoder = Ref<VideoDecoder>();
    }

    current_clip_idx = p_idx;
    if (p_idx < 0 || p_idx >= clips.size()) return;

    video_decoder.instantiate();
    if (!video_decoder->open(clips[p_idx].path)) {
        UtilityFunctions::push_error("[VideoStreamPlayback] Failed to open video decoder: ", clips[p_idx].path);
        current_clip_idx = -1;
        return;
    }

    audio_decoder.instantiate();
    if (!audio_decoder->open(clips[p_idx].path)) {
        audio_decoder = Ref<VideoDecoder>();
    } else if (audio_decoder->has_audio()) {
        audio_channels = 2;
        audio_sample_rate = 48000;
    }
}

bool VideoStreamPlaybackFFmpeg::_prime_first_frame() {
    if (video_decoder.is_null() || !video_decoder->is_open()) return false;
    
    // FFmpeg decoder often needs multiple packets before producing the first frame.
    // Try up to 30 reads to buffer enough packets.
    for (int attempt = 0; attempt < 30; attempt++) {
        Ref<Image> img = video_decoder->read_video_frame();
        if (img.is_valid()) {
            frame_texture = ImageTexture::create_from_image(img);
            return true;
        }
    }
    return false;
}

void VideoStreamPlaybackFFmpeg::_play() {
    playing = true;
    paused = false;
    needs_seek = true;
    playback_position = 0.0;
    
    if (clips.size() > 0 && current_clip_idx < 0) {
        _switch_clip(0);
        if (!_prime_first_frame()) {
            UtilityFunctions::push_error("[VideoStreamPlayback] Failed to prime first frame");
        }
    }
}

void VideoStreamPlaybackFFmpeg::_stop() {
    playing = false;
    playback_position = 0.0;
    current_clip_idx = -1;
    if (video_decoder.is_valid()) video_decoder->close();
    if (audio_decoder.is_valid()) audio_decoder->close();
    frame_texture = Ref<ImageTexture>();
}

bool VideoStreamPlaybackFFmpeg::_is_playing() const {
    return playing;
}

void VideoStreamPlaybackFFmpeg::_set_paused(bool p_paused) {
    paused = p_paused;
}

bool VideoStreamPlaybackFFmpeg::_is_paused() const {
    return paused;
}

void VideoStreamPlaybackFFmpeg::_seek(double p_time) {
    playback_position = CLAMP(p_time, 0.0, total_duration);
    needs_seek = true;
    int target = _find_clip(playback_position);
    if (target != current_clip_idx) {
        _switch_clip(target);
        _prime_first_frame();
    } else if (video_decoder.is_valid()) {
        double local = playback_position - clips[target].start_time;
        video_decoder->seek(local);
        if (audio_decoder.is_valid()) audio_decoder->seek(local);
        needs_seek = false;
    }
}

double VideoStreamPlaybackFFmpeg::_get_length() const {
    return total_duration;
}

double VideoStreamPlaybackFFmpeg::_get_playback_position() const {
    return playback_position;
}

Ref<Texture2D> VideoStreamPlaybackFFmpeg::_get_texture() const {
    return frame_texture;
}

void VideoStreamPlaybackFFmpeg::_update(double p_delta) {
    if (!playing || paused) return;

    playback_position += p_delta;
    if (playback_position >= total_duration) {
        playback_position = total_duration;
        playing = false;
        return;
    }

    int target = _find_clip(playback_position);
    if (target < 0) return;

    if (target != current_clip_idx) {
        _switch_clip(target);
        needs_seek = true;
        _prime_first_frame();  // Prime immediately after clip switch
        return;  // Skip the rest this frame, we already have a texture
    }

    if (video_decoder.is_null() || !video_decoder->is_open()) return;

    double local_time = playback_position - clips[target].start_time;

    if (needs_seek) {
        video_decoder->seek(local_time);
        if (audio_decoder.is_valid()) audio_decoder->seek(local_time);
        needs_seek = false;
    }

    Ref<Image> img = video_decoder->read_video_frame();
    if (img.is_valid()) {
        if (frame_texture.is_null() ||
            frame_texture->get_width() != img->get_width() ||
            frame_texture->get_height() != img->get_height()) {
            frame_texture = ImageTexture::create_from_image(img);
        } else {
            frame_texture->update(img);
        }
    }
    // If img is null, keep the previous frame_texture (hold frame)
}

int VideoStreamPlaybackFFmpeg::_get_channels() const {
    return audio_channels;
}

int VideoStreamPlaybackFFmpeg::_get_mix_rate() const {
    return audio_sample_rate;
}

int VideoStreamPlaybackFFmpeg::mix_audio(int p_frames, PackedFloat32Array p_buffer, int p_offset) {
    if (!playing || paused || current_clip_idx < 0) return 0;
    if (audio_decoder.is_null() || !audio_decoder->is_open() || !audio_decoder->has_audio()) return 0;

    double local_time = playback_position - clips[current_clip_idx].start_time;
    double drift = Math::abs(audio_decoder->get_current_time() - local_time);
    if (drift > 0.3) {
        audio_decoder->seek(local_time);
    }

    PackedFloat32Array samples = audio_decoder->read_audio_samples(p_frames * audio_channels);
    int frames_available = samples.size() / audio_channels;
    int frames_to_mix = MIN(p_frames, frames_available);

    for (int i = 0; i < frames_to_mix * audio_channels; i++) {
        p_buffer[p_offset + i] = samples[i];
    }

    return frames_to_mix;
}
