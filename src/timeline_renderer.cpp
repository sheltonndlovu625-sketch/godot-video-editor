#include "timeline_renderer.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functi
ons.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimelineRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &TimelineRenderer::set_timeline);
    ClassDB::bind_method(D_METHOD("get_timeline"), &TimelineRenderer::get_timeline);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::OBJECT, "timeline"), "set_timeline", "get_timeline");

    ClassDB::bind_method(D_METHOD("render_video_frame", "time", "width", "height"), &TimelineRenderer::render_video_frame);
    ClassDB::bind_method(D_METHOD("render_audio", "time", "num_samples", "sample_rate"), &TimelineRenderer::render_audio);
    ClassDB::bind_method(D_METHOD("export_to_file", "path", "width", "height", "fps", "video_bitrate", "sample_rate", "audio_bitrate"), &TimelineRenderer::export_to_file);
    ClassDB::bind_method(D_METHOD("clear_cache"), &TimelineRenderer::clear_cache);
}

TimelineRenderer::TimelineRenderer() {}

TimelineRenderer::~TimelineRenderer() {
    clear_cache();
}

void TimelineRenderer::set_timeline(const Ref<Timeline> &p_timeline) {
    timeline = p_timeline;
    clear_cache();
}

Ref<Timeline> TimelineRenderer::get_timeline() const {
    return timeline;
}

Ref<VideoDecoder> TimelineRenderer::get_decoder(const String &p_path) {
    if (decoders.has(p_path)) {
        UtilityFunctions::print("[TimelineRenderer] REUSING decoder for: ", p_path);
        return decoders[p_path];
    }

    UtilityFunctions::print("[TimelineRenderer] CREATING decoder for: ", p_path);
    Ref<VideoDecoder> decoder;
    decoder.instantiate();
    if (!decoder->open(p_path)) {
        UtilityFunctions::push_error("[TimelineRenderer] Failed to open: ", p_path);
        return Ref<VideoDecoder>();
    }

    decoders[p_path] = decoder;
    return decoder;
}

bool TimelineRenderer::_needs_seek(double p_time) {
    if (last_render_time < 0.0) {
        UtilityFunctions::print("[TimelineRenderer] NEEDS_SEEK: first frame (last=", last_render_time, ")");
        return true;
    }
    double frame_duration = 1.0 / timeline->get_frame_rate();
    double delta = Math::abs(p_time - last_render_time);
    bool needs = delta > frame_duration * 2.5;
    if (needs) {
        UtilityFunctions::print("[TimelineRenderer] NEEDS_SEEK: delta=", delta, " > threshold=", frame_duration * 2.5);
    }
    return needs;
}

Ref<Image> TimelineRenderer::render_video_frame(double p_time, int p_width, int p_height) {
    if (timeline.is_null()) {
        return Ref<Image>();
    }

    uint64_t t0 = OS::get_singleton()->get_ticks_usec();

    bool seek = _needs_seek(p_time);

    TypedArray<Image> frames;
    TypedArray<TimelineTrack> video_tracks = timeline->get_video_tracks();

    Vector<Ref<TimelineTrack>> sorted_tracks;
    for (int i = 0; i < video_tracks.size(); i++) {
        sorted_tracks.push_back(video_tracks[i]);
    }

    struct TrackComparator {
        _FORCE_INLINE_ bool operator()(const Ref<TimelineTrack> &a, const Ref<TimelineTrack> &b) const {
            return a->get_layer_index() < b->get_layer_index();
        }
    };
    sorted_tracks.sort_custom<TrackComparator>();

    uint64_t t_decode_start = OS::get_singleton()->get_ticks_usec();

    for (int i = 0; i < sorted_tracks.size(); i++) {
        Ref<TimelineTrack> track = sorted_tracks[i];
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path());
        if (decoder.is_null()) continue;

        if (seek) {
            uint64_t t_seek = OS::get_singleton()->get_ticks_usec();
            if (!decoder->seek(source_time)) {
                continue;
            }
            UtilityFunctions::print("[TimelineRenderer] SEEK took ", (OS::get_singleton()->get_ticks_usec() - t_seek) / 1000.0, " ms");
        }

        uint64_t t_read = OS::get_singleton()->get_ticks_usec();
        Ref<Image> frame = decoder->read_video_frame();
        UtilityFunctions::print("[TimelineRenderer] READ took ", (OS::get_singleton()->get_ticks_usec() - t_read) / 1000.0, " ms");

        if (frame.is_null()) continue;

        if (frame->get_width() != p_width || frame->get_height() != p_height) {
            uint64_t t_resize = OS::get_singleton()->get_ticks_usec();
            frame->resize(p_width, p_height, Image::INTERPOLATE_BILINEAR);
            UtilityFunctions::print("[TimelineRenderer] RESIZE took ", (OS::get_singleton()->get_ticks_usec() - t_resize) / 1000.0, " ms");
        }

        frames.push_back(frame);
    }

    last_render_time = p_time;

    uint64_t total = OS::get_singleton()->get_ticks_usec() - t0;
    UtilityFunctions::print("[TimelineRenderer] TOTAL render_video_frame took ", total / 1000.0, " ms");

    if (frames.is_empty()) {
        Ref<Image> black = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        black->fill(Color(0, 0, 0, 1));
        return black;
    }

    uint64_t t_comp = OS::get_singleton()->get_ticks_usec();
    Ref<Image> result = composite_frames(frames, p_width, p_height);
    UtilityFunctions::print("[TimelineRenderer] COMPOSITE took ", (OS::get_singleton()->get_ticks_usec() - t_comp) / 1000.0, " ms");

    return result;
}

Ref<Image> TimelineRenderer::composite_frames(const TypedArray<Image> &p_frames, int p_width, int p_height) {
    if (p_frames.is_empty()) {
        return Ref<Image>();
    }

    Ref<Image> result = p_frames[0];

    for (int i = 1; i < p_frames.size(); i++) {
        Ref<Image> top = p_frames[i];
        if (top.is_null()) continue;

        for (int y = 0; y < p_height; y++) {
            for (int x = 0; x < p_width; x++) {
                Color c = top->get_pixel(x, y);
                if (c.a > 0.0) {
                    result->set_pixel(x, y, c);
                }
            }
        }
    }

    return result;
}

PackedFloat32Array TimelineRenderer::render_audio(double p_time, int p_num_samples, int p_sample_rate) {
    PackedFloat32Array result;
    if (timeline.is_null() || p_num_samples <= 0) {
        return result;
    }

    bool seek = _needs_seek(p_time);

    TypedArray<TimelineTrack> audio_tracks = timeline->get_audio_tracks();
    TypedArray<PackedFloat32Array> buffers;

    for (int i = 0; i < audio_tracks.size(); i++) {
        Ref<TimelineTrack> track = audio_tracks[i];
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path());
        if (decoder.is_null() || !decoder->has_audio()) continue;

        if (seek) {
            if (!decoder->seek(source_time)) {
                continue;
            }
        }

        PackedFloat32Array samples = decoder->read_audio_samples(p_num_samples);
        if (samples.size() > 0) {
            buffers.push_back(samples);
        }
    }

    return mix_audio(buffers);
}

PackedFloat32Array TimelineRenderer::mix_audio(const TypedArray<PackedFloat32Array> &p_buffers) {
    PackedFloat32Array result;

    if (p_buffers.is_empty()) {
        return result;
    }

    int max_size = 0;
    for (int i = 0; i < p_buffers.size(); i++) {
        PackedFloat32Array buf = p_buffers[i];
        if (buf.size() > max_size) {
            max_size = buf.size();
        }
    }

    if (max_size == 0) {
        return result;
    }

    result.resize(max_size);

    for (int i = 0; i < p_buffers.size(); i++) {
        PackedFloat32Array buf = p_buffers[i];
        for (int j = 0; j < buf.size(); j++) {
            result[j] += buf[j];
        }
    }

    for (int i = 0; i < max_size; i++) {
        if (result[i] > 1.0f) result[i] = 1.0f;
        if (result[i] < -1.0f) result[i] = -1.0f;
    }

    return result;
}

bool TimelineRenderer::export_to_file(const String &p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_audio_bitrate) {
    if (timeline.is_null()) {
        UtilityFunctions::push_error("[TimelineRenderer] No timeline set");
        return false;
    }

    double duration = timeline->get_duration();
    if (duration <= 0.0) {
        UtilityFunctions::push_error("[TimelineRenderer] Timeline has no content");
        return false;
    }

    Ref<VideoEncoder> encoder;
    encoder.instantiate();

    bool has_audio = timeline->get_audio_tracks().size() > 0;
    bool ok;

    if (has_audio) {
        ok = encoder->open_with_audio(p_path, p_width, p_height, p_fps, p_video_bitrate, p_sample_rate, 2, p_audio_bitrate);
    } else {
        ok = encoder->open(p_path, p_width, p_height, p_fps, p_video_bitrate);
    }

    if (!ok) {
        UtilityFunctions::push_error("[TimelineRenderer] Failed to open encoder");
        return false;
    }

    int total_frames = int(duration * p_fps) + 1;
    double frame_time = 1.0 / p_fps;
    int audio_samples_per_frame = p_sample_rate / p_fps;

    UtilityFunctions::print("[TimelineRenderer] Exporting ", total_frames, " frames...");

    for (int frame = 0; frame < total_frames; frame++) {
        double time = frame * frame_time;

        Ref<Image> img = render_video_frame(time, p_width, p_height);
        if (img.is_null()) {
            UtilityFunctions::push_error("[TimelineRenderer] Failed to render frame ", frame);
            encoder->close();
            return false;
        }

        if (!encoder->write_frame(img)) {
            UtilityFunctions::push_error("[TimelineRenderer] Failed to write frame ", frame);
            encoder->close();
            return false;
        }

        if (has_audio) {
            PackedFloat32Array audio = render_audio(time, audio_samples_per_frame, p_sample_rate);
            if (audio.size() > 0) {
                encoder->write_audio(audio);
            }
        }

        if (frame % 30 == 0) {
            UtilityFunctions::print("  Exported frame ", frame, "/", total_frames);
        }
    }

    encoder->close();
    clear_cache();

    UtilityFunctions::print("[TimelineRenderer] Export complete: ", p_path);
    return true;
}

void TimelineRenderer::clear_cache() {
    Array keys = decoders.keys();
    for (int i = 0; i < keys.size(); i++) {
        Ref<VideoDecoder> decoder = decoders[keys[i]];
        if (decoder.is_valid()) {
            decoder->close();
        }
    }
    decoders.clear();
    last_render_time = -1.0;
}
