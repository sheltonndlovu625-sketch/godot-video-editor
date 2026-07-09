#include "timeline_renderer.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/math.hpp>

using namespace godot;

void TimelineRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &TimelineRenderer::set_timeline);
    ClassDB::bind_method(D_METHOD("get_timeline"), &TimelineRenderer::get_timeline);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::OBJECT, "timeline"), "set_timeline", "get_timeline");

    ClassDB::bind_method(D_METHOD("set_preview_mode", "preview"), &TimelineRenderer::set_preview_mode);
    ClassDB::bind_method(D_METHOD("get_preview_mode"), &TimelineRenderer::get_preview_mode);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::BOOL, "preview_mode"), "set_preview_mode", "get_preview_mode");

    ClassDB::bind_method(D_METHOD("render_video_frame", "time", "width", "height"), &TimelineRenderer::render_video_frame);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_texture", "time", "width", "height"), &TimelineRenderer::render_video_frame_to_texture);
    ClassDB::bind_method(D_METHOD("render_audio", "time", "num_samples", "sample_rate"), &TimelineRenderer::render_audio);
    ClassDB::bind_method(D_METHOD("export_to_file", "path", "width", "height", "fps", "video_bitrate", "sample_rate", "audio_bitrate"), &TimelineRenderer::export_to_file);
    ClassDB::bind_method(D_METHOD("clear_cache"), &TimelineRenderer::clear_cache);
}

TimelineRenderer::TimelineRenderer() {}
TimelineRenderer::~TimelineRenderer() { clear_cache(); }

void TimelineRenderer::set_timeline(const Ref<Timeline> &p_timeline) {
    timeline = p_timeline;
    clear_cache();
}

Ref<Timeline> TimelineRenderer::get_timeline() const { return timeline; }

void TimelineRenderer::set_preview_mode(bool p_preview) {
    if (preview_mode != p_preview) {
        preview_mode = p_preview;
        clear_cache(); // decoders must reopen with/without audio
    }
}
bool TimelineRenderer::get_preview_mode() const { return preview_mode; }

Ref<VideoDecoder> TimelineRenderer::get_decoder(const String &p_path, bool p_skip_audio) {
    String key = p_path + (p_skip_audio ? "|noaudio" : "|audio");
    if (decoders.has(key)) {
        return decoders[key];
    }

    Ref<VideoDecoder> decoder;
    decoder.instantiate();
    decoder->set_skip_audio(p_skip_audio);
    if (!decoder->open(p_path)) {
        UtilityFunctions::push_error("[TimelineRenderer] Failed to open: ", p_path);
        return Ref<VideoDecoder>();
    }

    decoders[key] = decoder;
    return decoder;
}

bool TimelineRenderer::_needs_seek(double p_time) {
    if (last_render_time < 0.0) return true;
    double frame_duration = 1.0 / timeline->get_frame_rate();
    double delta = Math::abs(p_time - last_render_time);
    // Only seek if we jump backwards or forward more than 1 frame
    return p_time < last_render_time || delta > frame_duration * 1.5;
}

Ref<Image> TimelineRenderer::render_video_frame(double p_time, int p_width, int p_height) {
    if (timeline.is_null()) return Ref<Image>();

    bool seek = _needs_seek(p_time);
    TypedArray<TimelineTrack> video_tracks = timeline->get_video_tracks();

    if (video_tracks.is_empty()) {
        Ref<Image> black = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        black->fill(Color(0, 0, 0, 1));
        return black;
    }

    // Find layer range without allocating a new Vector
    int min_layer = INT_MAX;
    int max_layer = INT_MIN;
    for (int i = 0; i < video_tracks.size(); i++) {
        Ref<TimelineTrack> track = video_tracks[i];
        if (track.is_null()) continue;
        int layer = track->get_layer_index();
        if (layer < min_layer) min_layer = layer;
        if (layer > max_layer) max_layer = layer;
    }

    Vector<Ref<Image>> frames;
    for (int layer = min_layer; layer <= max_layer; layer++) {
        for (int i = 0; i < video_tracks.size(); i++) {
            Ref<TimelineTrack> track = video_tracks[i];
            if (track.is_null() || track->get_layer_index() != layer) continue;

            Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
            if (clip.is_null()) continue;

            double local_time = p_time - clip->get_timeline_start();
            double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

            String path = clip->get_effective_path(preview_mode);
            Ref<VideoDecoder> decoder = get_decoder(path, preview_mode); // skip audio in preview
            if (decoder.is_null()) continue;

            if (seek) decoder->seek(source_time);

            Ref<Image> frame = decoder->read_video_frame_scaled(p_width, p_height);
            if (frame.is_valid()) {
                frames.push_back(frame);
            }
        }
    }

    last_render_time = p_time;

    if (frames.is_empty()) {
        Ref<Image> black = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        black->fill(Color(0, 0, 0, 1));
        return black;
    }
    if (frames.size() == 1) return frames[0];

    return composite_frames_fast(frames, p_width, p_height);
}

Ref<ImageTexture> TimelineRenderer::render_video_frame_to_texture(double p_time, int p_width, int p_height) {
    Ref<Image> img = render_video_frame(p_time, p_width, p_height);
    if (img.is_null()) return Ref<ImageTexture>();

    // Reuse one texture object — only recreate if size changes
    if (output_texture.is_null() || output_texture_w != p_width || output_texture_h != p_height) {
        output_texture.instantiate();
        output_texture->set_image(img);
        output_texture_w = p_width;
        output_texture_h = p_height;
    } else {
        output_texture->update(img);
    }

    return output_texture;
}

Ref<Image> TimelineRenderer::composite_frames_fast(const Vector<Ref<Image>> &p_frames, int p_width, int p_height) {
    if (p_frames.is_empty()) return Ref<Image>();
    Ref<Image> result = p_frames[0]->duplicate();
    if (p_frames.size() == 1) return result;

    for (int i = 1; i < p_frames.size(); i++) {
        Ref<Image> top = p_frames[i];
        if (top.is_null()) continue;
        result->blit_rect(top, Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
    }
    return result;
}

PackedFloat32Array TimelineRenderer::render_audio(double p_time, int p_num_samples, int p_sample_rate) {
    PackedFloat32Array result;
    if (timeline.is_null() || p_num_samples <= 0 || preview_mode) return result; // no audio in preview

    bool seek = _needs_seek(p_time);
    TypedArray<TimelineTrack> audio_tracks = timeline->get_audio_tracks();
    TypedArray<PackedFloat32Array> buffers;

    for (int i = 0; i < audio_tracks.size(); i++) {
        Ref<TimelineTrack> track = audio_tracks[i];
        if (track.is_null()) continue;
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path(), false);
        if (decoder.is_null() || !decoder->has_audio()) continue;

        if (seek) decoder->seek(source_time);

        PackedFloat32Array samples = decoder->read_audio_samples(p_num_samples);
        if (samples.size() > 0) buffers.push_back(samples);
    }

    return mix_audio(buffers);
}

PackedFloat32Array TimelineRenderer::mix_audio(const TypedArray<PackedFloat32Array> &p_buffers) {
    PackedFloat32Array result;
    if (p_buffers.is_empty()) return result;

    int max_size = 0;
    for (int i = 0; i < p_buffers.size(); i++) {
        PackedFloat32Array buf = p_buffers[i];
        if (buf.size() > max_size) max_size = buf.size();
    }
    if (max_size == 0) return result;

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

    // Force full quality for export
    bool was_preview = preview_mode;
    set_preview_mode(false);

    Ref<VideoEncoder> encoder;
    encoder.instantiate();

    bool has_audio = timeline->get_audio_tracks().size() > 0;
    bool ok = has_audio
        ? encoder->open_with_audio(p_path, p_width, p_height, p_fps, p_video_bitrate, p_sample_rate, 2, p_audio_bitrate)
        : encoder->open(p_path, p_width, p_height, p_fps, p_video_bitrate);

    if (!ok) {
        UtilityFunctions::push_error("[TimelineRenderer] Failed to open encoder");
        set_preview_mode(was_preview);
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
            set_preview_mode(was_preview);
            return false;
        }

        if (!encoder->write_frame(img)) {
            UtilityFunctions::push_error("[TimelineRenderer] Failed to write frame ", frame);
            encoder->close();
            set_preview_mode(was_preview);
            return false;
        }

        if (has_audio) {
            PackedFloat32Array audio = render_audio(time, audio_samples_per_frame, p_sample_rate);
            if (audio.size() > 0) encoder->write_audio(audio);
        }

        if (frame % 30 == 0) {
            UtilityFunctions::print("  Exported frame ", frame, "/", total_frames);
        }
    }

    encoder->close();
    clear_cache();
    set_preview_mode(was_preview);

    UtilityFunctions::print("[TimelineRenderer] Export complete: ", p_path);
    return true;
}

void TimelineRenderer::clear_cache() {
    Array keys = decoders.keys();
    for (int i = 0; i < keys.size(); i++) {
        Ref<VideoDecoder> decoder = decoders[keys[i]];
        if (decoder.is_valid()) decoder->close();
    }
    decoders.clear();
    output_texture.unref();
    output_texture_w = 0;
    output_texture_h = 0;
    last_render_time = -1.0;
}
