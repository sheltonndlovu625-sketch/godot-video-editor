#ifndef VIDEO_STREAM_PLAYBACK_FFMPEG_H
#define VIDEO_STREAM_PLAYBACK_FFMPEG_H

#include <godot_cpp/classes/video_stream_playback.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/texture_2d.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <mutex>
#include "video_decoder.h"

namespace godot {

class VideoStreamPlaybackFFmpeg : public VideoStreamPlayback {
    GDCLASS(VideoStreamPlaybackFFmpeg, VideoStreamPlayback)

public:
    struct Clip {
        String path;
        double start_time = 0.0;
        double duration = 0.0;
        int width = 0;
        int height = 0;
    };

private:
    Vector<Clip> clips;
    double total_duration = 0.0;
    double playback_position = 0.0;
    bool playing = false;
    bool paused = false;
    bool needs_seek = false;

    int current_clip_idx = -1;
    Ref<VideoDecoder> video_decoder;
    Ref<VideoDecoder> audio_decoder;

    Ref<ImageTexture> frame_texture;

    int audio_channels = 2;
    int audio_sample_rate = 48000;

    std::mutex state_mutex;

    int _find_clip(double p_time) const;
    void _switch_clip(int p_idx);

protected:
    static void _bind_methods();

public:
    void setup(const Vector<Clip> &p_clips, double p_total_duration);

    virtual void _play() override;
    virtual void _stop() override;
    virtual bool _is_playing() const override;
    virtual void _set_paused(bool p_paused) override;
    virtual bool _is_paused() const override;
    virtual void _seek(double p_time) override;
    virtual double _get_length() const override;
    virtual double _get_playback_position() const override;
    virtual Ref<Texture2D> _get_texture() const override;
    virtual void _update(double p_delta) override;

    virtual int _get_channels() const override;
    virtual int _get_mix_rate() const override;
    virtual int mix_audio(int p_frames, PackedFloat32Array p_buffer, int p_offset) override;

    VideoStreamPlaybackFFmpeg();
    ~VideoStreamPlaybackFFmpeg();
};

}

#endif
