// preview_player.cpp
#include "preview_player.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void PreviewPlayer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_timeline_renderer", "renderer"), &PreviewPlayer::set_timeline_renderer);
    ClassDB::bind_method(D_METHOD("get_timeline_renderer"), &PreviewPlayer::get_timeline_renderer);
    ClassDB::add_property("PreviewPlayer", PropertyInfo(Variant::OBJECT, "timeline_renderer"), "set_timeline_renderer", "get_timeline_renderer");

    ClassDB::bind_method(D_METHOD("set_preview_size", "width", "height"), &PreviewPlayer::set_preview_size);
    ClassDB::bind_method(D_METHOD("get_preview_size"), &PreviewPlayer::get_preview_size);

    ClassDB::bind_method(D_METHOD("set_playback_fps", "fps"), &PreviewPlayer::set_playback_fps);
    ClassDB::bind_method(D_METHOD("get_playback_fps"), &PreviewPlayer::get_playback_fps);
    ClassDB::add_property("PreviewPlayer", PropertyInfo(Variant::FLOAT, "playback_fps"), "set_playback_fps", "get_playback_fps");

    ClassDB::bind_method(D_METHOD("set_loop", "loop"), &PreviewPlayer::set_loop);
    ClassDB::bind_method(D_METHOD("get_loop"), &PreviewPlayer::get_loop);
    ClassDB::add_property("PreviewPlayer", PropertyInfo(Variant::BOOL, "loop"), "set_loop", "get_loop");

    ClassDB::bind_method(D_METHOD("play"), &PreviewPlayer::play);
    ClassDB::bind_method(D_METHOD("pause"), &PreviewPlayer::pause);
    ClassDB::bind_method(D_METHOD("stop"), &PreviewPlayer::stop);
    ClassDB::bind_method(D_METHOD("seek", "time"), &PreviewPlayer::seek);
    ClassDB::bind_method(D_METHOD("get_playback_state"), &PreviewPlayer::get_playback_state);
    ClassDB::bind_method(D_METHOD("get_current_frame_texture"), &PreviewPlayer::get_current_frame_texture);

    ADD_SIGNAL(MethodInfo("frame_ready", PropertyInfo(Variant::OBJECT, "texture")));
    ADD_SIGNAL(MethodInfo("playback_started"));
    ADD_SIGNAL(MethodInfo("playback_paused"));
    ADD_SIGNAL(MethodInfo("playback_stopped"));
    ADD_SIGNAL(MethodInfo("time_changed", PropertyInfo(Variant::FLOAT, "time")));

    BIND_ENUM_CONSTANT(STATE_STOPPED);
    BIND_ENUM_CONSTANT(STATE_PLAYING);
    BIND_ENUM_CONSTANT(STATE_PAUSED);
}

PreviewPlayer::PreviewPlayer() {}

void PreviewPlayer::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        audio_player = memnew(AudioStreamPlayer);
        add_child(audio_player, false, INTERNAL_MODE_FRONT);
        audio_generator.instantiate();
        audio_generator->set_mix_rate(audio_sample_rate);
        audio_generator->set_buffer_length(0.1); // 100 ms ring buffer
        audio_player->set_stream(audio_generator);
    }
    if (p_what == NOTIFICATION_EXIT_TREE) {
        if (audio_player) {
            audio_player->queue_free();
            audio_player = nullptr;
        }
    }
}

void PreviewPlayer::_process(double delta) {
    if (state != STATE_PLAYING) return;
    if (renderer.is_null()) return;
    if (Engine::get_singleton()->is_editor_hint()) return;

    Ref<Timeline> timeline = renderer->get_timeline();
    if (timeline.is_null()) return;

    current_time += delta;
    double duration = timeline->get_duration();

    if (current_time >= duration) {
        if (loop) {
            current_time = 0.0;
        } else {
            current_time = duration;
            stop();
            return;
        }
    }

    // ---- Video ----
    Ref<Image> frame = renderer->render_video_frame(current_time, preview_width, preview_height);
    if (frame.is_valid()) {
        if (preview_texture.is_null() || preview_texture->get_size() != Vector2(preview_width, preview_height)) {
            preview_texture.instantiate();
            preview_texture->set_image(frame);
        } else {
            preview_texture->update(frame);
        }
        emit_signal("frame_ready", preview_texture);
    }

    // ---- Audio ----
    if (!audio_playback.is_valid() && audio_player && audio_player->is_playing()) {
        audio_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(audio_player->get_stream_playback().ptr()));
    }
    if (audio_playback.is_valid() && renderer.is_valid()) {
        int frames_available = audio_playback->get_frames_available();
        if (frames_available > 0) {
            int channels = 2;
            PackedFloat32Array audio = renderer->render_audio(current_time, frames_available * channels, audio_sample_rate);
            int total_samples = audio.size();
            int frames_to_push = total_samples / channels;
            if (frames_to_push > frames_available) {
                frames_to_push = frames_available;
            }
            for (int i = 0; i < frames_to_push; i++) {
                float left  = audio[i * channels];
                float right = (channels > 1) ? audio[i * channels + 1] : left;
                audio_playback->push_frame(Vector2(left, right));
            }
        }
    }

    emit_signal("time_changed", current_time);
}

void PreviewPlayer::set_timeline_renderer(const Ref<TimelineRenderer> &p_renderer) {
    renderer = p_renderer;
}
Ref<TimelineRenderer> PreviewPlayer::get_timeline_renderer() const { return renderer; }

void PreviewPlayer::set_preview_size(int p_width, int p_height) {
    preview_width = Math::max(1, p_width);
    preview_height = Math::max(1, p_height);
}
Vector2i PreviewPlayer::get_preview_size() const { return Vector2i(preview_width, preview_height); }

void PreviewPlayer::set_playback_fps(double p_fps) { playback_fps = Math::max(1.0, p_fps); }
double PreviewPlayer::get_playback_fps() const { return playback_fps; }

void PreviewPlayer::set_loop(bool p_loop) { loop = p_loop; }
bool PreviewPlayer::get_loop() const { return loop; }

void PreviewPlayer::play() {
    if (state == STATE_PLAYING) return;
    state = STATE_PLAYING;
    if (audio_player) {
        audio_player->play();
        audio_playback = Ref<AudioStreamGeneratorPlayback>(Object::cast_to<AudioStreamGeneratorPlayback>(audio_player->get_stream_playback().ptr()));
    }
    emit_signal("playback_started");
}
void PreviewPlayer::pause() {
    if (state != STATE_PLAYING) return;
    state = STATE_PAUSED;
    if (audio_player) {
        audio_player->stop();
    }
    audio_playback = Ref<AudioStreamGeneratorPlayback>();
    emit_signal("playback_paused");
}
void PreviewPlayer::stop() {
    state = STATE_STOPPED;
    current_time = 0.0;
    if (audio_player) {
        audio_player->stop();
    }
    audio_playback = Ref<AudioStreamGeneratorPlayback>();
    emit_signal("playback_stopped");
}
void PreviewPlayer::seek(double p_time) {
    current_time = Math::max(0.0, p_time);
    if (renderer.is_valid()) {
        Ref<Timeline> tl = renderer->get_timeline();
        if (tl.is_valid()) {
            current_time = Math::min(current_time, tl->get_duration());
        }
    }
    // Flush stale audio so we don't hear pre-seek samples
    if (audio_playback.is_valid()) {
        int avail = audio_playback->get_frames_available();
        for (int i = 0; i < avail; i++) {
            audio_playback->push_frame(Vector2(0.0f, 0.0f));
        }
    }
    emit_signal("time_changed", current_time);
}
PreviewPlayer::PlaybackState PreviewPlayer::get_playback_state() const { return state; }
Ref<ImageTexture> PreviewPlayer::get_current_frame_texture() const { return preview_texture; }
