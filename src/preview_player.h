// preview_player.h
#ifndef PREVIEW_PLAYER_H
#define PREVIEW_PLAYER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include "timeline_renderer.h"

namespace godot {

class PreviewPlayer : public Node {
    GDCLASS(PreviewPlayer, Node)

public:
    enum PlaybackState {
        STATE_STOPPED,
        STATE_PLAYING,
        STATE_PAUSED
    };

private:
    Ref<TimelineRenderer> renderer;
    Ref<ImageTexture> preview_texture;
    int preview_width = 1080;
    int preview_height = 1920;
    double playback_fps = 30.0;
    PlaybackState state = STATE_STOPPED;
    bool loop = true;
    double current_time = 0.0;

protected:
    static void _bind_methods();

public:
    void _process(double delta) override;

    void set_timeline_renderer(const Ref<TimelineRenderer> &p_renderer);
    Ref<TimelineRenderer> get_timeline_renderer() const;

    void set_preview_size(int p_width, int p_height);
    Vector2i get_preview_size() const;

    void set_playback_fps(double p_fps);
    double get_playback_fps() const;

    void set_loop(bool p_loop);
    bool get_loop() const;

    void play();
    void pause();
    void stop();
    void seek(double p_time);

    PlaybackState get_playback_state() const;
    Ref<ImageTexture> get_current_frame_texture() const;

    PreviewPlayer();
};

}

VARIANT_ENUM_CAST(PreviewPlayer::PlaybackState);

#endif
