#include "timeline_renderer.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>

using namespace godot;

void TimelineRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &TimelineRenderer::set_timeline);
    ClassDB::bind_method(D_METHOD("get_timeline"), &TimelineRenderer::get_timeline);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::OBJECT, "timeline"), "set_timeline", "get_timeline");

    ClassDB::bind_method(D_METHOD("render_video_frame", "time", "width", "height"), &TimelineRenderer::render_video_frame);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_texture", "time", "width", "height"), &TimelineRenderer::render_video_frame_to_texture);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_rid", "time", "width", "height"), &TimelineRenderer::render_video_frame_to_rid);
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
        return true;
    }
    double frame_duration = 1.0 / timeline->get_frame_rate();
    double delta = p_time - last_render_time;

    if (delta >= 0.0 && delta < frame_duration * 10.0) {
        return false;
    }
    return true;
}

// ------------------------------------------------------------------
// CPU path
// ------------------------------------------------------------------

Ref<Image> TimelineRenderer::render_video_frame(double p_time, int p_width, int p_height) {
    if (timeline.is_null()) {
        return Ref<Image>();
    }

    bool seek = _needs_seek(p_time);

    TypedArray<TimelineTrack> video_tracks = timeline->get_video_tracks();
    if (video_tracks.is_empty()) {
        if (black_frame.is_null() || black_frame->get_width() != p_width || black_frame->get_height() != p_height) {
            black_frame = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
            black_frame->fill(Color(0, 0, 0, 1));
        }
        return black_frame;
    }

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

    Vector<Ref<Image>> frames;
    for (int i = 0; i < sorted_tracks.size(); i++) {
        Ref<TimelineTrack> track = sorted_tracks[i];
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path());
        if (decoder.is_null()) continue;

        if (seek) {
            decoder->seek(source_time);
        }

        Ref<Image> frame = decoder->read_video_frame_scaled(p_width, p_height);
        if (frame.is_null()) continue;

        frames.push_back(frame);
    }

    last_render_time = p_time;

    if (frames.is_empty()) {
        if (black_frame.is_null() || black_frame->get_width() != p_width || black_frame->get_height() != p_height) {
            black_frame = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
            black_frame->fill(Color(0, 0, 0, 1));
        }
        return black_frame;
    }

    if (frames.size() == 1) {
        return frames[0];
    }

    if (composite_buffer.is_null() || composite_buffer->get_width() != p_width || composite_buffer->get_height() != p_height) {
        composite_buffer = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
    }
    composite_buffer->fill(Color(0, 0, 0, 1));

    for (int i = 0; i < frames.size(); i++) {
        Ref<Image> top = frames[i];
        if (top.is_null()) continue;
        composite_buffer->blit_rect(top, Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
    }

    return composite_buffer;
}

Ref<ImageTexture> TimelineRenderer::render_video_frame_to_texture(double p_time, int p_width, int p_height) {
    Ref<Image> img = render_video_frame(p_time, p_width, p_height);
    if (img.is_null()) {
        return Ref<ImageTexture>();
    }

    if (preview_texture.is_null() || preview_tex_w != p_width || preview_tex_h != p_height) {
        preview_texture.instantiate();
        preview_texture->set_image(img);
        preview_tex_w = p_width;
        preview_tex_h = p_height;
    } else {
        preview_texture->update(img);
    }
    return preview_texture;
}

// ------------------------------------------------------------------
// GPU Compositor Infrastructure
// ------------------------------------------------------------------

void TimelineRenderer::_ensure_gpu_compositor(RenderingServer *p_rs, int p_width, int p_height) {
    if (comp_viewport.is_valid() && comp_w == p_width && comp_h == p_height) {
        return;
    }

    _free_gpu_compositor();

    comp_viewport = p_rs->viewport_create();
    p_rs->viewport_set_size(comp_viewport, p_width, p_height);
    p_rs->viewport_set_transparent_background(comp_viewport, false);
    p_rs->viewport_set_active(comp_viewport, true);

    comp_canvas = p_rs->canvas_create();
    p_rs->viewport_attach_canvas(comp_viewport, comp_canvas);

    comp_w = p_width;
    comp_h = p_height;
}

void TimelineRenderer::_free_gpu_compositor() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return;

    _free_layer_items();

    if (comp_canvas.is_valid()) { p_rs->free_rid(comp_canvas); comp_canvas = RID(); }
    if (comp_viewport.is_valid()) { p_rs->free_rid(comp_viewport); comp_viewport = RID(); }
    comp_w = 0; comp_h = 0;
}

void TimelineRenderer::_ensure_layer_items(RenderingServer *p_rs, int p_count) {
    while (layer_items.size() > p_count) {
        p_rs->free_rid(layer_items[layer_items.size() - 1]);
        layer_items.remove_at(layer_items.size() - 1);
    }
    while (layer_items.size() < p_count) {
        RID item = p_rs->canvas_item_create();
        p_rs->canvas_item_set_parent(item, comp_canvas);
        layer_items.push_back(item);
    }
}

void TimelineRenderer::_free_layer_items() {
    RenderingServer *p_rs = RenderingServer::get_singleton();
    if (!p_rs) return;
    for (int i = 0; i < layer_items.size(); i++) {
        if (layer_items[i].is_valid()) {
            p_rs->free_rid(layer_items[i]);
        }
    }
    layer_items.clear();
}

void TimelineRenderer::_ensure_blend_materials() {
    if (mat_normal.is_valid()) return;

    mat_normal.instantiate();
    mat_normal->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX);

    mat_add.instantiate();
    mat_add->set_blend_mode(CanvasItemMaterial::BLEND_MODE_ADD);

    mat_multiply.instantiate();
    mat_multiply->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MUL);

    mat_subtract.instantiate();
    mat_subtract->set_blend_mode(CanvasItemMaterial::BLEND_MODE_SUB);
}

RID TimelineRenderer::_get_blend_material(int p_blend_mode) const {
    switch (p_blend_mode) {
        case TimelineTrack::BLEND_MODE_ADD:
            return mat_add.is_valid() ? mat_add->get_rid() : RID();
        case TimelineTrack::BLEND_MODE_MULTIPLY:
            return mat_multiply.is_valid() ? mat_multiply->get_rid() : RID();
        case TimelineTrack::BLEND_MODE_SUBTRACT:
            return mat_subtract.is_valid() ? mat_subtract->get_rid() : RID();
        case TimelineTrack::BLEND_MODE_NORMAL:
        default:
            return mat_normal.is_valid() ? mat_normal->get_rid() : RID();
    }
}

RID TimelineRenderer::_composite_gpu_with_transforms(RenderingServer *p_rs,
    const Vector<RID> &p_textures,
    const Vector<Transform2D> &p_transforms,
    const Vector<int> &p_blend_modes,
    const Vector<float> &p_opacities,
    int p_width, int p_height) {

    _ensure_gpu_compositor(p_rs, p_width, p_height);
    _ensure_layer_items(p_rs, p_textures.size());
    _ensure_blend_materials();

    for (int i = 0; i < layer_items.size(); i++) {
        p_rs->canvas_item_clear(layer_items[i]);
    }

    for (int i = 0; i < p_textures.size(); i++) {
        RID item = layer_items[i];

        p_rs->canvas_item_set_transform(item, p_transforms[i]);
        p_rs->canvas_item_set_material(item, _get_blend_material(p_blend_modes[i]));

        Color modulate = Color(1.0f, 1.0f, 1.0f, p_opacities[i]);
        p_rs->canvas_item_set_self_modulate(item, modulate);

        p_rs->canvas_item_add_texture_rect(
            item,
            Rect2(0, 0, p_width, p_height),
            p_textures[i]
        );
    }

    p_rs->viewport_set_update_mode(comp_viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
    return p_rs->viewport_get_texture(comp_viewport);
}

// ------------------------------------------------------------------
// GPU + Effects + Transforms + Blend Modes + Text Overlays Preview Path
// ------------------------------------------------------------------

RID TimelineRenderer::render_video_frame_to_rid(double p_time, int p_width, int p_height) {
    if (timeline.is_null()) return RID();

    RenderingServer *rs = RenderingServer::get_singleton();

    // Get sorted video tracks
    TypedArray<TimelineTrack> video_tracks = timeline->get_video_tracks();
    Vector<Ref<TimelineTrack>> sorted_tracks;
    for (int i = 0; i < video_tracks.size(); i++) sorted_tracks.push_back(video_tracks[i]);
    struct TrackComparator {
        _FORCE_INLINE_ bool operator()(const Ref<TimelineTrack> &a, const Ref<TimelineTrack> &b) const {
            return a->get_layer_index() < b->get_layer_index();
        }
    };
    sorted_tracks.sort_custom<TrackComparator>();

    bool seek = _needs_seek(p_time);

    // ---- Collect video clip layers ----
    Vector<RID> clip_textures;
    Vector<Transform2D> clip_transforms;
    Vector<int> clip_blend_modes;
    Vector<float> clip_opacities;
    Vector<RID> temp_textures; // Track for cleanup

    for (int i = 0; i < sorted_tracks.size(); i++) {
        Ref<TimelineTrack> track = sorted_tracks[i];
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path());
        if (decoder.is_null()) continue;
        if (seek) decoder->seek(source_time);

        Ref<Image> frame = decoder->read_video_frame_scaled(p_width, p_height);
        if (frame.is_null()) continue;

        RID frame_tex = rs->texture_2d_create(frame);
        temp_textures.push_back(frame_tex);
        RID current_tex = frame_tex;

        // Apply clip effects (GPU path)
        TypedArray<VideoEffect> fx = clip->get_effects();
        for (int e = 0; e < fx.size(); e++) {
            Ref<VideoEffect> effect = fx[e];
            if (effect.is_null()) continue;
            RID next_tex = effect->apply_to_texture(rs, current_tex, p_width, p_height);
            current_tex = next_tex;
        }

        clip_textures.push_back(current_tex);

        // Build Transform2D from clip properties
        float rot = clip->get_rotation();
        Vector2 scl = clip->get_scale();
        Vector2 pos = clip->get_position();
        Vector2 anchor = clip->get_anchor_point();

        Transform2D t;
        t[0] = Vector2(Math::cos(rot), Math::sin(rot)) * scl.x;
        t[1] = Vector2(-Math::sin(rot), Math::cos(rot)) * scl.y;
        Vector2 anchor_offset = Vector2(anchor.x * p_width, anchor.y * p_height);
        t[2] = pos - (t[0] * anchor_offset.x + t[1] * anchor_offset.y);

        clip_transforms.push_back(t);
        clip_blend_modes.push_back(track->get_blend_mode());
        clip_opacities.push_back(clip->get_opacity());
    }

    // ---- Collect text overlay layers (on top of video) ----
    TypedArray<TextOverlay> text_overlays = timeline->get_text_overlays_at_time(p_time);
    for (int i = 0; i < text_overlays.size(); i++) {
        Ref<TextOverlay> ov = text_overlays[i];
        if (ov.is_null()) continue;

        RID text_tex = ov->render_to_rid(rs, p_width, p_height, p_time);
        if (!text_tex.is_valid()) continue;

        clip_textures.push_back(text_tex);

        // Text overlay transform
        Vector2 pos = ov->get_position();
        Vector2 anchor = ov->get_anchor_point();
        float opacity = ov->get_opacity();

        // Apply animation
        float anim_opacity;
        Vector2 anim_offset;
        double local_time = p_time - ov->get_start_time();
        // We need to call _apply_animation but it's private. Instead, we approximate here.
        // For preview, the text overlay's render_to_rid handles the viewport texture.
        // Position/opacity animation should be handled by the compositor.
        // For simplicity, we use the base position and opacity here.
        // Full animation support would require exposing animation state from TextOverlay.

        Transform2D t;
        t[0] = Vector2(1, 0);
        t[1] = Vector2(0, 1);
        // Anchor offset: text overlays render at their own viewport size, not canvas size
        // So we position them directly at pos - anchor * text_size
        // But we don't know text_size here easily. We'll use pos directly and let anchor
        // be handled by the compositor offset.
        t[2] = pos;

        clip_transforms.push_back(t);
        clip_blend_modes.push_back(TimelineTrack::BLEND_MODE_NORMAL);
        clip_opacities.push_back(opacity);
    }

    last_render_time = p_time;

    if (clip_textures.is_empty()) {
        if (black_frame.is_null() || black_frame->get_width() != p_width || black_frame->get_height() != p_height) {
            black_frame = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
            black_frame->fill(Color(0, 0, 0, 1));
        }
        RID black_tex = rs->texture_2d_create(black_frame);
        temp_textures.push_back(black_tex);
        clip_textures.push_back(black_tex);
        clip_transforms.push_back(Transform2D());
        clip_blend_modes.push_back(TimelineTrack::BLEND_MODE_NORMAL);
        clip_opacities.push_back(1.0f);
    }

    RID final_tex = _composite_gpu_with_transforms(rs,
        clip_textures, clip_transforms, clip_blend_modes, clip_opacities,
        p_width, p_height);

    // Free temp textures we created (frame uploads)
    for (int i = 0; i < temp_textures.size(); i++) {
        rs->free_rid(temp_textures[i]);
    }

    return final_tex;
}

Ref<Image> TimelineRenderer::composite_frames_fast(const Vector<Ref<Image>> &p_frames, int p_width, int p_height) {
    if (p_frames.is_empty()) {
        return Ref<Image>();
    }

    Ref<Image> result = p_frames[0]->duplicate();
    if (p_frames.size() == 1) {
        return result;
    }

    for (int i = 1; i < p_frames.size(); i++) {
        Ref<Image> top = p_frames[i];
        if (top.is_null()) continue;
        result->blit_rect(top, Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
    }

    return result;
}

Ref<Image> TimelineRenderer::composite_frames(const TypedArray<Image> &p_frames, int p_width, int p_height) {
    Vector<Ref<Image>> vec;
    for (int i = 0; i < p_frames.size(); i++) {
        vec.push_back(p_frames[i]);
    }
    return composite_frames_fast(vec, p_width, p_height);
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
        if (track.is_null()) continue;
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

// ------------------------------------------------------------------
// CPU Effects helper for export
// ------------------------------------------------------------------

Ref<Image> TimelineRenderer::_apply_cpu_effects(const Ref<Image> &p_frame, const TypedArray<VideoEffect> &p_effects, int p_width, int p_height) {
    Ref<Image> result = p_frame;
    for (int i = 0; i < p_effects.size(); i++) {
        Ref<VideoEffect> effect = p_effects[i];
        if (effect.is_null()) continue;
        result = effect->apply_to_image(result, p_width, p_height);
        if (result.is_null()) break;
    }
    return result;
}

// ------------------------------------------------------------------
// Export with CPU effects + text overlay support
// ------------------------------------------------------------------

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

        // ---- Video compositing (CPU path) ----
        TypedArray<TimelineTrack> video_tracks = timeline->get_video_tracks();
        Vector<Ref<TimelineTrack>> sorted_tracks;
        for (int i = 0; i < video_tracks.size(); i++) sorted_tracks.push_back(video_tracks[i]);
        struct TrackComparator {
            _FORCE_INLINE_ bool operator()(const Ref<TimelineTrack> &a, const Ref<TimelineTrack> &b) const {
                return a->get_layer_index() < b->get_layer_index();
            }
        };
        sorted_tracks.sort_custom<TrackComparator>();

        Vector<Ref<Image>> frames;
        for (int i = 0; i < sorted_tracks.size(); i++) {
            Ref<TimelineTrack> track = sorted_tracks[i];
            Ref<TimelineClip> clip = track->get_clip_at_time(time);
            if (clip.is_null()) continue;

            double local_time = time - clip->get_timeline_start();
            double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

            Ref<VideoDecoder> decoder = get_decoder(clip->get_source_path());
            if (decoder.is_null()) continue;
            decoder->seek(source_time);

            Ref<Image> raw_frame = decoder->read_video_frame_scaled(p_width, p_height);
            if (raw_frame.is_null()) continue;

            // Apply CPU effects
            if (clip->get_effect_count() > 0) {
                raw_frame = _apply_cpu_effects(raw_frame, clip->get_effects(), p_width, p_height);
            }

            // Apply opacity
            float op = clip->get_opacity();
            if (op < 1.0f && raw_frame.is_valid()) {
                Ref<Image> faded = raw_frame->duplicate();
                int w = faded->get_width();
                int h = faded->get_height();
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        Color c = faded->get_pixel(x, y);
                        c.a *= op;
                        faded->set_pixel(x, y, c);
                    }
                }
                raw_frame = faded;
            }

            frames.push_back(raw_frame);
        }

        Ref<Image> img;
        if (frames.is_empty()) {
            img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
            img->fill(Color(0, 0, 0, 1));
        } else if (frames.size() == 1) {
            img = frames[0];
        } else {
            img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
            img->fill(Color(0, 0, 0, 1));
            for (int i = 0; i < frames.size(); i++) {
                if (frames[i].is_valid()) {
                    img->blit_rect(frames[i], Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
                }
            }
        }

        // ---- Text overlays (CPU path: render to image and blit) ----
        TypedArray<TextOverlay> overlays = timeline->get_text_overlays_at_time(time);
        for (int i = 0; i < overlays.size(); i++) {
            Ref<TextOverlay> ov = overlays[i];
            if (ov.is_null()) continue;

            Ref<Image> text_img = ov->render_to_image();
            if (text_img.is_null()) continue;

            Vector2 pos = ov->get_position();
            Vector2 anchor = ov->get_anchor_point();
            int tw = text_img->get_width();
            int th = text_img->get_height();

            Vector2 blit_pos = pos - Vector2(anchor.x * tw, anchor.y * th);
            int bx = int(blit_pos.x);
            int by = int(blit_pos.y);

            // Simple alpha blit
            for (int y = 0; y < th; y++) {
                int py = by + y;
                if (py < 0 || py >= p_height) continue;
                for (int x = 0; x < tw; x++) {
                    int px = bx + x;
                    if (px < 0 || px >= p_width) continue;
                    Color src = text_img->get_pixel(x, y);
                    if (src.a <= 0.001f) continue;
                    Color dst = img->get_pixel(px, py);
                    float out_a = src.a + dst.a * (1.0f - src.a);
                    if (out_a > 0.001f) {
                        Color out;
                        out.r = (src.r * src.a + dst.r * dst.a * (1.0f - src.a)) / out_a;
                        out.g = (src.g * src.a + dst.g * dst.a * (1.0f - src.a)) / out_a;
                        out.b = (src.b * src.a + dst.b * dst.a * (1.0f - src.a)) / out_a;
                        out.a = out_a;
                        img->set_pixel(px, py, out);
                    }
                }
            }
        }

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

    if (preview_texture_rid.is_valid()) {
        RenderingServer::get_singleton()->free_rid(preview_texture_rid);
        preview_texture_rid = RID();
    }
    preview_texture.unref();
    preview_tex_w = 0;
    preview_tex_h = 0;

    _free_gpu_compositor();

    mat_normal.unref();
    mat_add.unref();
    mat_multiply.unref();
    mat_subtract.unref();

    composite_buffer.unref();
    black_frame.unref();

    last_render_time = -1.0;
}
