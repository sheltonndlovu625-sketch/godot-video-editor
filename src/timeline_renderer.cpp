#include "timeline_renderer.h"
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>
#include "audio_fx.h"

using namespace godot;
TimelineRenderer::TimelineRenderer() {}
TimelineRenderer::~TimelineRenderer() {}

void TimelineRenderer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_timeline", "timeline"), &TimelineRenderer::set_timeline);
    ClassDB::bind_method(D_METHOD("get_timeline"), &TimelineRenderer::get_timeline);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::OBJECT, "timeline"), "set_timeline", "get_timeline");

    ClassDB::bind_method(D_METHOD("set_aspect_ratio_mode", "mode"), &TimelineRenderer::set_aspect_ratio_mode);
    ClassDB::bind_method(D_METHOD("get_aspect_ratio_mode"), &TimelineRenderer::get_aspect_ratio_mode);
    ClassDB::add_property("TimelineRenderer", PropertyInfo(Variant::INT, "aspect_ratio_mode"), "set_aspect_ratio_mode", "get_aspect_ratio_mode");

    ClassDB::bind_method(D_METHOD("render_video_frame", "time", "width", "height"), &TimelineRenderer::render_video_frame);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_texture", "time", "width", "height"), &TimelineRenderer::render_video_frame_to_texture);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_rid", "time", "width", "height"), &TimelineRenderer::render_video_frame_to_rid);
    ClassDB::bind_method(D_METHOD("render_video_frame_with_edit_nodes", "time", "width", "height", "edit_nodes"), &TimelineRenderer::render_video_frame_with_edit_nodes);
    ClassDB::bind_method(D_METHOD("render_video_frame_to_rid_with_edit_nodes", "time", "width", "height", "edit_nodes"), &TimelineRenderer::render_video_frame_to_rid_with_edit_nodes);
    ClassDB::bind_method(D_METHOD("render_audio", "time", "num_samples", "sample_rate"), &TimelineRenderer::render_audio);
    ClassDB::bind_method(D_METHOD("export_to_file", "path", "width", "height", "fps", "video_bitrate", "sample_rate", "audio_bitrate"), &TimelineRenderer::export_to_file);
    ClassDB::bind_method(D_METHOD("clear_cache"), &TimelineRenderer::clear_cache);

    BIND_ENUM_CONSTANT(ASPECT_FILL);
    BIND_ENUM_CONSTANT(ASPECT_FIT);
    BIND_ENUM_CONSTANT(ASPECT_STRETCH);
}


void TimelineRenderer::set_timeline(const Ref<Timeline> &p_timeline) {
    timeline = p_timeline;
    clear_cache();
}

Ref<Timeline> TimelineRenderer::get_timeline() const {
    return timeline;
}

void TimelineRenderer::set_aspect_ratio_mode(int p_mode) {
    aspect_ratio_mode = (AspectRatioMode)p_mode;
}

int TimelineRenderer::get_aspect_ratio_mode() const {
    return (int)aspect_ratio_mode;
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

Ref<VideoDecoder> TimelineRenderer::get_audio_decoder(const String &p_path) {
    if (audio_decoders.has(p_path)) {
        return audio_decoders[p_path];
    }

    UtilityFunctions::print("[TimelineRenderer] CREATING audio decoder for: ", p_path);
    Ref<VideoDecoder> decoder;
    decoder.instantiate();
    if (!decoder->open(p_path)) {
        UtilityFunctions::push_error("[TimelineRenderer] Failed to open audio decoder: ", p_path);
        return Ref<VideoDecoder>();
    }

    audio_decoders[p_path] = decoder;
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

    // ---- Composite video clips ----
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

    // Check for text overlays so we know whether we can use the fast path
    TypedArray<TextOverlay> overlays = timeline->get_text_overlays_at_time(p_time);
    bool has_overlays = overlays.size() > 0;

    Ref<Image> img;
    if (frames.is_empty()) {
        img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        img->fill(Color(0, 0, 0, 1));
    } else if (frames.size() == 1 && !has_overlays) {
        // Fast path: no overlays, return decoder frame directly
        return frames[0];
    } else if (frames.size() == 1) {
        // Duplicate so we don't corrupt the decoder's double-buffer
        img = frames[0]->duplicate();
    } else {
        img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        img->fill(Color(0, 0, 0, 1));
        for (int i = 0; i < frames.size(); i++) {
            if (frames[i].is_valid()) {
                img->blit_rect(frames[i], Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
            }
        }
    }

    // ---- Composite text overlays on top (same logic as export) ----
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

    return img;
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
    const Vector<Vector2> &p_texture_sizes,
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

        // Use native texture size if provided, otherwise full canvas
        Vector2 size = (i < p_texture_sizes.size() && p_texture_sizes[i].x > 0.0f && p_texture_sizes[i].y > 0.0f)
            ? p_texture_sizes[i]
            : Vector2(p_width, p_height);

        p_rs->canvas_item_add_texture_rect(
            item,
            Rect2(Vector2(), size),
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
    Vector<Vector2> clip_texture_sizes;

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
        clip_texture_sizes.push_back(Vector2(p_width, p_height));

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

        Vector2 text_size = ov->get_render_size();
        clip_texture_sizes.push_back(text_size);

        Vector2 pos = ov->get_position();
        Vector2 anchor = ov->get_anchor_point();
        float opacity = ov->get_opacity();

        // FIX: Apply anchor offset so position is center-based, not top-left
        Vector2 anchor_offset = Vector2(anchor.x * text_size.x, anchor.y * text_size.y);

        Transform2D t;
        t[0] = Vector2(1, 0);
        t[1] = Vector2(0, 1);
        t[2] = pos - anchor_offset;

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
        clip_texture_sizes.push_back(Vector2(p_width, p_height));
        clip_transforms.push_back(Transform2D());
        clip_blend_modes.push_back(TimelineTrack::BLEND_MODE_NORMAL);
        clip_opacities.push_back(1.0f);
    }

    RID final_tex = _composite_gpu_with_transforms(rs,
        clip_textures, clip_transforms, clip_blend_modes, clip_opacities, clip_texture_sizes,
        p_width, p_height);

    // Free temp textures we created (frame uploads)
    for (int i = 0; i < temp_textures.size(); i++) {
        rs->free_rid(temp_textures[i]);
    }

    return final_tex;

}

// ------------------------------------------------------------------
// TextOverlayEditNode support helpers
// ------------------------------------------------------------------

Ref<Image> TimelineRenderer::_render_text_overlay_from_node(const Ref<TextOverlay> &p_overlay, TextOverlayEditNode *p_edit_node, double p_time) {
    if (p_overlay.is_null()) return Ref<Image>();

    // If edit node is selected, mark dirty to pick up live transform changes
    if (p_edit_node && p_edit_node->is_selected()) {
        p_edit_node->mark_dirty();
    }

    return p_overlay->render_to_image();
}

// ------------------------------------------------------------------
// CPU path with edit node awareness
// ------------------------------------------------------------------

Ref<Image> TimelineRenderer::render_video_frame_with_edit_nodes(double p_time, int p_width, int p_height, const TypedArray<TextOverlayEditNode> &p_edit_nodes) {
    if (timeline.is_null()) {
        return Ref<Image>();
    }

    bool seek = _needs_seek(p_time);

    // ---- Composite video clips ----
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

    TypedArray<TextOverlay> overlays = timeline->get_text_overlays_at_time(p_time);
    bool has_overlays = overlays.size() > 0;

    Ref<Image> img;
    if (frames.is_empty()) {
        img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        img->fill(Color(0, 0, 0, 1));
    } else if (frames.size() == 1 && !has_overlays) {
        return frames[0];
    } else if (frames.size() == 1) {
        img = frames[0]->duplicate();
    } else {
        img = Image::create(p_width, p_height, false, Image::FORMAT_RGBA8);
        img->fill(Color(0, 0, 0, 1));
        for (int i = 0; i < frames.size(); i++) {
            if (frames[i].is_valid()) {
                img->blit_rect(frames[i], Rect2i(0, 0, p_width, p_height), Vector2i(0, 0));
            }
        }
    }

    // Build lookup from TextOverlay resource to edit node
    HashMap<TextOverlay*, TextOverlayEditNode*> edit_node_map;
    for (int i = 0; i < p_edit_nodes.size(); i++) {
        TextOverlayEditNode *node = Object::cast_to<TextOverlayEditNode>(p_edit_nodes[i]);
        if (!node) continue;
        Ref<TextOverlay> ov = node->get_text_overlay();
        if (ov.is_valid()) {
            edit_node_map[ov.ptr()] = node;
        }
    }

    // ---- Composite text overlays with edit node awareness ----
    for (int i = 0; i < overlays.size(); i++) {
        Ref<TextOverlay> ov = overlays[i];
        if (ov.is_null()) continue;

        TextOverlayEditNode **edit_node_ptr = edit_node_map.getptr(ov.ptr());
        TextOverlayEditNode *edit_node = edit_node_ptr ? *edit_node_ptr : nullptr;

        Ref<Image> text_img = _render_text_overlay_from_node(ov, edit_node, p_time);
        if (text_img.is_null()) continue;

        Vector2 pos = ov->get_position();
        Vector2 anchor = ov->get_anchor_point();
        int tw = text_img->get_width();
        int th = text_img->get_height();

        // Handle rotation if present (from edit node)
        float rot = ov->get_rotation();
        if (Math::abs(rot) > 0.001f) {
            Ref<Image> rotated = Image::create(tw, th, false, Image::FORMAT_RGBA8);
            rotated->fill(Color(0, 0, 0, 0));

            float cos_r = Math::cos(rot);
            float sin_r = Math::sin(rot);
            Vector2 center(tw * 0.5f, th * 0.5f);

            for (int y = 0; y < th; y++) {
                for (int x = 0; x < tw; x++) {
                    float dx = x - center.x;
                    float dy = y - center.y;
                    float src_x = center.x + dx * cos_r - dy * sin_r;
                    float src_y = center.y + dx * sin_r + dy * cos_r;

                    int sx = int(Math::round(src_x));
                    int sy = int(Math::round(src_y));
                    if (sx >= 0 && sx < tw && sy >= 0 && sy < th) {
                        rotated->set_pixel(x, y, text_img->get_pixel(sx, sy));
                    }
                }
            }
            text_img = rotated;
        }

        Vector2 blit_pos = pos - Vector2(anchor.x * tw, anchor.y * th);
        int bx = int(blit_pos.x);
        int by = int(blit_pos.y);

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

    return img;
}

// ------------------------------------------------------------------
// GPU path with edit node awareness
// ------------------------------------------------------------------

RID TimelineRenderer::render_video_frame_to_rid_with_edit_nodes(double p_time, int p_width, int p_height, const TypedArray<TextOverlayEditNode> &p_edit_nodes) {
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
    Vector<RID> temp_textures;
    Vector<Vector2> clip_texture_sizes;

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
        clip_texture_sizes.push_back(Vector2(p_width, p_height));

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

    // Build lookup from TextOverlay to edit node
    HashMap<TextOverlay*, TextOverlayEditNode*> edit_node_map;
    for (int i = 0; i < p_edit_nodes.size(); i++) {
        TextOverlayEditNode *node = Object::cast_to<TextOverlayEditNode>(p_edit_nodes[i]);
        if (!node) continue;
        Ref<TextOverlay> ov = node->get_text_overlay();
        if (ov.is_valid()) {
            edit_node_map[ov.ptr()] = node;
        }
    }

    // ---- Collect text overlay layers (on top of video) with edit node awareness ----
    TypedArray<TextOverlay> text_overlays = timeline->get_text_overlays_at_time(p_time);
    for (int i = 0; i < text_overlays.size(); i++) {
        Ref<TextOverlay> ov = text_overlays[i];
        if (ov.is_null()) continue;

        TextOverlayEditNode **edit_node_ptr = edit_node_map.getptr(ov.ptr());
        TextOverlayEditNode *edit_node = edit_node_ptr ? *edit_node_ptr : nullptr;

        // If edit node is selected, mark dirty to pick up live transform changes
        if (edit_node && edit_node->is_selected()) {
            edit_node->mark_dirty();
        }

        RID text_tex = ov->render_to_rid(rs, p_width, p_height, p_time);
        if (!text_tex.is_valid()) continue;

        clip_textures.push_back(text_tex);

        Vector2 text_size = ov->get_render_size();
        clip_texture_sizes.push_back(text_size);

        Vector2 pos = ov->get_position();
        Vector2 anchor = ov->get_anchor_point();
        float opacity = ov->get_opacity();
        float rot = ov->get_rotation();
        float scl = ov->get_scale();

        // Apply scale and rotation from TextOverlay (supported by edit node)
        Vector2 anchor_offset = Vector2(anchor.x * text_size.x, anchor.y * text_size.y);

        Transform2D t;
        t[0] = Vector2(Math::cos(rot), Math::sin(rot)) * scl;
        t[1] = Vector2(-Math::sin(rot), Math::cos(rot)) * scl;
        t[2] = pos - (t[0] * anchor_offset.x + t[1] * anchor_offset.y);

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
        clip_texture_sizes.push_back(Vector2(p_width, p_height));
        clip_transforms.push_back(Transform2D());
        clip_blend_modes.push_back(TimelineTrack::BLEND_MODE_NORMAL);
        clip_opacities.push_back(1.0f);
    }

    RID final_tex = _composite_gpu_with_transforms(rs,
        clip_textures, clip_transforms, clip_blend_modes, clip_opacities, clip_texture_sizes,
        p_width, p_height);

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

    // Independent seek logic for audio (do not share last_render_time with video)
    bool seek = false;
    if (last_audio_time < 0.0) {
        seek = true;
    } else {
        double frame_duration = 1.0 / timeline->get_frame_rate();
        double delta = p_time - last_audio_time;
        if (!(delta >= 0.0 && delta < frame_duration * 10.0)) {
            seek = true;
        }
    }

    TypedArray<TimelineTrack> audio_tracks = timeline->get_audio_tracks();
    TypedArray<PackedFloat32Array> buffers;

    for (int i = 0; i < audio_tracks.size(); i++) {
        Ref<TimelineTrack> track = audio_tracks[i];
        if (track.is_null()) continue;
        Ref<TimelineClip> clip = track->get_clip_at_time(p_time);
        if (clip.is_null()) continue;

        double local_time = p_time - clip->get_timeline_start();
        double source_time = clip->get_source_in_point() + (local_time * clip->get_playback_speed());

        Ref<VideoDecoder> decoder = get_audio_decoder(clip->get_source_path());
        if (decoder.is_null() || !decoder->has_audio()) continue;

        if (seek) {
            if (!decoder->seek(source_time)) {
                continue;
            }
        }

        PackedFloat32Array samples = decoder->read_audio_samples(p_num_samples);
        if (samples.size() > 0) {
            // ---- Apply per-clip AudioFX ----
            Ref<AudioFX> fx = clip->get_audio_fx();
            if (fx.is_valid()) {
                samples = fx->process_audio(samples, decoder->get_audio_sample_rate(), decoder->get_audio_channels());
            }
            buffers.push_back(samples);
        }
    }

    last_audio_time = p_time;
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
// Export with CPU effects + text overlay support + edit node support
// ------------------------------------------------------------------

bool TimelineRenderer::export_to_file(const String &p_path, int p_width, int p_height, int p_fps, int p_video_bitrate, int p_sample_rate, int p_audio_bitrate, const TypedArray<TextOverlayEditNode> &p_edit_nodes) {
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

    // Build edit node lookup once
    HashMap<TextOverlay*, TextOverlayEditNode*> edit_node_map;
    for (int i = 0; i < p_edit_nodes.size(); i++) {
        TextOverlayEditNode *node = Object::cast_to<TextOverlayEditNode>(p_edit_nodes[i]);
        if (!node) continue;
        Ref<TextOverlay> ov = node->get_text_overlay();
        if (ov.is_valid()) {
            edit_node_map[ov.ptr()] = node;
        }
    }

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

            // Only seek if we jumped discontinuously; sequential reads are smooth
            double decoder_time = decoder->get_current_time();
            double diff = source_time - decoder_time;
            if (diff < -0.05 || diff > 0.5) {
                decoder->seek(source_time);
            }

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

            TextOverlayEditNode **edit_node_ptr = edit_node_map.getptr(ov.ptr());
            TextOverlayEditNode *edit_node = edit_node_ptr ? *edit_node_ptr : nullptr;

            Ref<Image> text_img = _render_text_overlay_from_node(ov, edit_node, time);
            if (text_img.is_null()) continue;

            Vector2 pos = ov->get_position();
            Vector2 anchor = ov->get_anchor_point();
            int tw = text_img->get_width();
            int th = text_img->get_height();

            // Handle rotation if present
            float rot = ov->get_rotation();
            if (Math::abs(rot) > 0.001f) {
                Ref<Image> rotated = Image::create(tw, th, false, Image::FORMAT_RGBA8);
                rotated->fill(Color(0, 0, 0, 0));

                float cos_r = Math::cos(rot);
                float sin_r = Math::sin(rot);
                Vector2 center(tw * 0.5f, th * 0.5f);

                for (int y = 0; y < th; y++) {
                    for (int x = 0; x < tw; x++) {
                        float dx = x - center.x;
                        float dy = y - center.y;
                        float src_x = center.x + dx * cos_r - dy * sin_r;
                        float src_y = center.y + dx * sin_r + dy * cos_r;

                        int sx = int(Math::round(src_x));
                        int sy = int(Math::round(src_y));
                        if (sx >= 0 && sx < tw && sy >= 0 && sy < th) {
                            rotated->set_pixel(x, y, text_img->get_pixel(sx, sy));
                        }
                    }
                }
                text_img = rotated;
            }

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

        last_render_time = time;
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

    Array audio_keys = audio_decoders.keys();
    for (int i = 0; i < audio_keys.size(); i++) {
        Ref<VideoDecoder> decoder = audio_decoders[audio_keys[i]];
        if (decoder.is_valid()) {
            decoder->close();
        }
    }
    audio_decoders.clear();

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
    last_audio_time = -1.0;
}

// ------------------------------------------------------------------
// Stubs for declared-but-not-yet-implemented private helpers
// ------------------------------------------------------------------

const Vector<Ref<TimelineTrack>> &TimelineRenderer::_get_sorted_video_tracks() {
    static Vector<Ref<TimelineTrack>> empty;
    if (timeline.is_null()) return empty;
    if (!video_tracks_dirty) return cached_video_tracks;
    cached_video_tracks.clear();
    TypedArray<TimelineTrack> vt = timeline->get_video_tracks();
    for (int i = 0; i < vt.size(); i++) cached_video_tracks.push_back(vt[i]);
    struct TrackComparator {
        _FORCE_INLINE_ bool operator()(const Ref<TimelineTrack> &a, const Ref<TimelineTrack> &b) const {
            return a->get_layer_index() < b->get_layer_index();
        }
    };
    cached_video_tracks.sort_custom<TrackComparator>();
    video_tracks_dirty = false;
    return cached_video_tracks;
}

const Vector<Ref<TimelineTrack>> &TimelineRenderer::_get_sorted_audio_tracks() {
    static Vector<Ref<TimelineTrack>> empty;
    if (timeline.is_null()) return empty;
    if (!audio_tracks_dirty) return cached_audio_tracks;
    cached_audio_tracks.clear();
    TypedArray<TimelineTrack> at = timeline->get_audio_tracks();
    for (int i = 0; i < at.size(); i++) cached_audio_tracks.push_back(at[i]);
    struct TrackComparator {
        _FORCE_INLINE_ bool operator()(const Ref<TimelineTrack> &a, const Ref<TimelineTrack> &b) const {
            return a->get_layer_index() < b->get_layer_index();
        }
    };
    cached_audio_tracks.sort_custom<TrackComparator>();
    audio_tracks_dirty = false;
    return cached_audio_tracks;
}

Vector2i TimelineRenderer::_get_decode_size(int p_src_w, int p_src_h, int p_dst_w, int p_dst_h) const {
    // Placeholder: return destination size
    return Vector2i(p_dst_w, p_dst_h);
}

void TimelineRenderer::_cpu_blit_normal(Image *p_dst, Image *p_src, int p_dx, int p_dy, float p_opacity) {
    if (!p_dst || !p_src) return;
    p_dst->blit_rect(p_src, Rect2i(0, 0, p_src->get_width(), p_src->get_height()), Vector2i(p_dx, p_dy));
}

void TimelineRenderer::_cpu_blit_add(Image *p_dst, Image *p_src, int p_dx, int p_dy, float p_opacity) {
    // TODO: implement additive CPU blend
    _cpu_blit_normal(p_dst, p_src, p_dx, p_dy, p_opacity);
}

void TimelineRenderer::_cpu_blit_multiply(Image *p_dst, Image *p_src, int p_dx, int p_dy, float p_opacity) {
    // TODO: implement multiply CPU blend
    _cpu_blit_normal(p_dst, p_src, p_dx, p_dy, p_opacity);
}

void TimelineRenderer::_cpu_blit_subtract(Image *p_dst, Image *p_src, int p_dx, int p_dy, float p_opacity) {
    // TODO: implement subtract CPU blend
    _cpu_blit_normal(p_dst, p_src, p_dx, p_dy, p_opacity);
}

void TimelineRenderer::_cpu_blit(Image *p_dst, Image *p_src, int p_dx, int p_dy, float p_opacity, int p_blend_mode) {
    switch (p_blend_mode) {
        case TimelineTrack::BLEND_MODE_ADD:       _cpu_blit_add(p_dst, p_src, p_dx, p_dy, p_opacity); break;
        case TimelineTrack::BLEND_MODE_MULTIPLY:  _cpu_blit_multiply(p_dst, p_src, p_dx, p_dy, p_opacity); break;
        case TimelineTrack::BLEND_MODE_SUBTRACT:  _cpu_blit_subtract(p_dst, p_src, p_dx, p_dy, p_opacity); break;
        case TimelineTrack::BLEND_MODE_NORMAL:
        default:                                  _cpu_blit_normal(p_dst, p_src, p_dx, p_dy, p_opacity); break;
    }
}

Ref<Image> TimelineRenderer::_cpu_render_text_overlay(const Ref<TextOverlay> &p_overlay, int p_canvas_w, int p_canvas_h, double p_time) {
    if (p_overlay.is_null()) return Ref<Image>();
    return p_overlay->render_to_image();
}

Ref<Image> TimelineRenderer::_cpu_render_image_overlay(const Ref<ImageOverlay> &p_overlay, int p_canvas_w, int p_canvas_h, double p_time) {
    if (p_overlay.is_null()) return Ref<Image>();
    return p_overlay->render_to_image();
}

Ref<Image> TimelineRenderer::_cpu_composite_frame(double p_time, int p_width, int p_height) {
    // TODO: full CPU compositing with aspect ratio handling
    return render_video_frame(p_time, p_width, p_height);
}