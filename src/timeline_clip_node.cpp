#include "timeline_clip_node.h"
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/image.hpp>

using namespace godot;

void TimelineClipNode::_bind_methods() {
    // Core clip property — THIS WAS MISSING
    ClassDB::bind_method(D_METHOD("set_clip", "clip"), &TimelineClipNode::set_clip);
    ClassDB::bind_method(D_METHOD("get_clip"), &TimelineClipNode::get_clip);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::OBJECT, "clip", PROPERTY_HINT_RESOURCE_TYPE, "TimelineClip"), "set_clip", "get_clip");

    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &TimelineClipNode::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &TimelineClipNode::is_selected);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::BOOL, "selected"), "set_selected", "is_selected");

    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineClipNode::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineClipNode::get_pixels_per_second);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::FLOAT, "pixels_per_second"), "set_pixels_per_second", "get_pixels_per_second");

    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineClipNode::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineClipNode::get_zoom);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::FLOAT, "zoom"), "set_zoom", "get_zoom");

    ClassDB::bind_method(D_METHOD("set_is_video", "is_video"), &TimelineClipNode::set_is_video);
    ClassDB::bind_method(D_METHOD("get_is_video"), &TimelineClipNode::get_is_video);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::BOOL, "is_video"), "set_is_video", "get_is_video");

    ClassDB::bind_method(D_METHOD("update_layout"), &TimelineClipNode::update_layout);

    // Customisation
    ClassDB::bind_method(D_METHOD("set_custom_color", "color"), &TimelineClipNode::set_custom_color);
    ClassDB::bind_method(D_METHOD("get_custom_color"), &TimelineClipNode::get_custom_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "custom_color"), "set_custom_color", "get_custom_color");

    ClassDB::bind_method(D_METHOD("set_use_custom_color", "use"), &TimelineClipNode::set_use_custom_color);
    ClassDB::bind_method(D_METHOD("get_use_custom_color"), &TimelineClipNode::get_use_custom_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::BOOL, "use_custom_color"), "set_use_custom_color", "get_use_custom_color");

    ClassDB::bind_method(D_METHOD("set_font", "font"), &TimelineClipNode::set_font);
    ClassDB::bind_method(D_METHOD("get_font"), &TimelineClipNode::get_font);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::OBJECT, "font", PROPERTY_HINT_RESOURCE_TYPE, "Font"), "set_font", "get_font");

    ClassDB::bind_method(D_METHOD("set_font_size", "size"), &TimelineClipNode::set_font_size);
    ClassDB::bind_method(D_METHOD("get_font_size"), &TimelineClipNode::get_font_size);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::INT, "font_size"), "set_font_size", "get_font_size");

    ClassDB::bind_method(D_METHOD("set_label_color", "color"), &TimelineClipNode::set_label_color);
    ClassDB::bind_method(D_METHOD("get_label_color"), &TimelineClipNode::get_label_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "label_color"), "set_label_color", "get_label_color");

    // Selection customisation
    ClassDB::bind_method(D_METHOD("set_selection_border_color", "color"), &TimelineClipNode::set_selection_border_color);
    ClassDB::bind_method(D_METHOD("get_selection_border_color"), &TimelineClipNode::get_selection_border_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "selection_border_color"), "set_selection_border_color", "get_selection_border_color");

    ClassDB::bind_method(D_METHOD("set_selection_border_width", "width"), &TimelineClipNode::set_selection_border_width);
    ClassDB::bind_method(D_METHOD("get_selection_border_width"), &TimelineClipNode::get_selection_border_width);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::FLOAT, "selection_border_width"), "set_selection_border_width", "get_selection_border_width");

    ClassDB::bind_method(D_METHOD("set_selection_handle_color", "color"), &TimelineClipNode::set_selection_handle_color);
    ClassDB::bind_method(D_METHOD("get_selection_handle_color"), &TimelineClipNode::get_selection_handle_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "selection_handle_color"), "set_selection_handle_color", "get_selection_handle_color");

    ClassDB::bind_method(D_METHOD("set_handle_width", "width"), &TimelineClipNode::set_handle_width);
    ClassDB::bind_method(D_METHOD("get_handle_width"), &TimelineClipNode::get_handle_width);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::FLOAT, "handle_width"), "set_handle_width", "get_handle_width");

    ClassDB::bind_method(D_METHOD("set_selection_grip_color", "color"), &TimelineClipNode::set_selection_grip_color);
    ClassDB::bind_method(D_METHOD("get_selection_grip_color"), &TimelineClipNode::get_selection_grip_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "selection_grip_color"), "set_selection_grip_color", "get_selection_grip_color");

    ClassDB::bind_method(D_METHOD("set_split_handle_color", "color"), &TimelineClipNode::set_split_handle_color);
    ClassDB::bind_method(D_METHOD("get_split_handle_color"), &TimelineClipNode::get_split_handle_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "split_handle_color"), "set_split_handle_color", "get_split_handle_color");

    ClassDB::bind_method(D_METHOD("set_split_line_color", "color"), &TimelineClipNode::set_split_line_color);
    ClassDB::bind_method(D_METHOD("get_split_line_color"), &TimelineClipNode::get_split_line_color);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::COLOR, "split_line_color"), "set_split_line_color", "get_split_line_color");

    // Display mode
    ClassDB::bind_method(D_METHOD("set_display_mode", "mode"), &TimelineClipNode::set_display_mode);
    ClassDB::bind_method(D_METHOD("get_display_mode"), &TimelineClipNode::get_display_mode);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::INT, "display_mode"), "set_display_mode", "get_display_mode");

    ClassDB::bind_method(D_METHOD("set_thumb_size", "size"), &TimelineClipNode::set_thumb_size);
    ClassDB::bind_method(D_METHOD("get_thumb_size"), &TimelineClipNode::get_thumb_size);
    ClassDB::add_property("TimelineClipNode", PropertyInfo(Variant::FLOAT, "thumb_size"), "set_thumb_size", "get_thumb_size");

    ClassDB::bind_method(D_METHOD("refresh_thumbnails"), &TimelineClipNode::refresh_thumbnails);

    ADD_SIGNAL(MethodInfo("selected"));
    ADD_SIGNAL(MethodInfo("moved", PropertyInfo(Variant::FLOAT, "new_start")));
    ADD_SIGNAL(MethodInfo("trimmed", PropertyInfo(Variant::FLOAT, "new_start"), PropertyInfo(Variant::FLOAT, "new_duration"), PropertyInfo(Variant::FLOAT, "new_source_in")));
    ADD_SIGNAL(MethodInfo("split_requested", PropertyInfo(Variant::FLOAT, "timeline_time")));

    BIND_ENUM_CONSTANT(DRAG_NONE);
    BIND_ENUM_CONSTANT(DRAG_MOVE);
    BIND_ENUM_CONSTANT(DRAG_TRIM_LEFT);
    BIND_ENUM_CONSTANT(DRAG_TRIM_RIGHT);

    BIND_ENUM_CONSTANT(DISPLAY_SOLID);
    BIND_ENUM_CONSTANT(DISPLAY_THUMBNAILS);
}

TimelineClipNode::TimelineClipNode() {
    set_clip_contents(true);
}

void TimelineClipNode::set_clip(const Ref<TimelineClip> &p_clip) {
    clip = p_clip;
    thumbnails_dirty = true;
    thumbnail_textures.clear();
    if (thumb_decoder.is_valid()) {
        thumb_decoder->close();
    }
    update_layout();
    queue_redraw();
}

Ref<TimelineClip> TimelineClipNode::get_clip() const {
    return clip;
}

void TimelineClipNode::set_selected(bool p_selected) {
    selected = p_selected;
    queue_redraw();
}

bool TimelineClipNode::is_selected() const {
    return selected;
}

void TimelineClipNode::set_pixels_per_second(float p_pps) {
    pixels_per_second = p_pps;
    update_layout();
}

float TimelineClipNode::get_pixels_per_second() const {
    return pixels_per_second;
}

void TimelineClipNode::set_zoom(float p_zoom) {
    zoom = p_zoom;
    update_layout();
}

float TimelineClipNode::get_zoom() const {
    return zoom;
}

void TimelineClipNode::set_is_video(bool p_video) {
    is_video = p_video;
    if (!use_custom_color) {
        base_color = is_video ? Color(0.2f, 0.5f, 0.95f) : Color(0.25f, 0.8f, 0.35f);
    }
    queue_redraw();
}

bool TimelineClipNode::get_is_video() const {
    return is_video;
}

void TimelineClipNode::update_layout() {
    if (clip.is_null()) return;
    double dur = clip->get_duration();
    float w = Math::max(24.0f, (float)(dur * pixels_per_second * zoom));
    set_custom_minimum_size(Vector2(w, 0));
    set_size(Vector2(w, get_size().y));
}

void TimelineClipNode::set_custom_color(const Color &p_color) {
    custom_color = p_color;
    if (use_custom_color) {
        base_color = custom_color;
    }
    queue_redraw();
}

Color TimelineClipNode::get_custom_color() const {
    return custom_color;
}

void TimelineClipNode::set_use_custom_color(bool p_use) {
    use_custom_color = p_use;
    if (use_custom_color) {
        base_color = custom_color;
    } else {
        base_color = is_video ? Color(0.2f, 0.5f, 0.95f) : Color(0.25f, 0.8f, 0.35f);
    }
    queue_redraw();
}

bool TimelineClipNode::get_use_custom_color() const {
    return use_custom_color;
}

void TimelineClipNode::set_font(const Ref<Font> &p_font) {
    font = p_font;
    queue_redraw();
}

Ref<Font> TimelineClipNode::get_font() const {
    return font;
}

void TimelineClipNode::set_font_size(int p_size) {
    font_size = p_size;
    queue_redraw();
}

int TimelineClipNode::get_font_size() const {
    return font_size;
}

void TimelineClipNode::set_label_color(const Color &p_color) {
    label_color = p_color;
    queue_redraw();
}

Color TimelineClipNode::get_label_color() const {
    return label_color;
}

void TimelineClipNode::set_selection_border_color(const Color &p_color) {
    selection_border_color = p_color;
    queue_redraw();
}
Color TimelineClipNode::get_selection_border_color() const {
    return selection_border_color;
}

void TimelineClipNode::set_selection_border_width(float p_width) {
    selection_border_width = Math::max(0.0f, p_width);
    queue_redraw();
}
float TimelineClipNode::get_selection_border_width() const {
    return selection_border_width;
}

void TimelineClipNode::set_selection_handle_color(const Color &p_color) {
    selection_handle_color = p_color;
    queue_redraw();
}
Color TimelineClipNode::get_selection_handle_color() const {
    return selection_handle_color;
}

void TimelineClipNode::set_handle_width(float p_width) {
    handle_width = Math::max(4.0f, p_width);
    queue_redraw();
}
float TimelineClipNode::get_handle_width() const {
    return handle_width;
}

void TimelineClipNode::set_selection_grip_color(const Color &p_color) {
    selection_grip_color = p_color;
    queue_redraw();
}
Color TimelineClipNode::get_selection_grip_color() const {
    return selection_grip_color;
}

void TimelineClipNode::set_split_handle_color(const Color &p_color) {
    split_handle_color = p_color;
    queue_redraw();
}
Color TimelineClipNode::get_split_handle_color() const {
    return split_handle_color;
}

void TimelineClipNode::set_split_line_color(const Color &p_color) {
    split_line_color = p_color;
    queue_redraw();
}
Color TimelineClipNode::get_split_line_color() const {
    return split_line_color;
}

void TimelineClipNode::set_display_mode(int p_mode) {
    DisplayMode new_mode = (DisplayMode)p_mode;
    if (new_mode != display_mode) {
        display_mode = new_mode;
        thumbnails_dirty = true;
        queue_redraw();
    }
}

int TimelineClipNode::get_display_mode() const {
    return (int)display_mode;
}

void TimelineClipNode::set_thumb_size(float p_size) {
    thumb_size = Math::max(16.0f, p_size);
    thumbnails_dirty = true;
    queue_redraw();
}

float TimelineClipNode::get_thumb_size() const {
    return thumb_size;
}

Ref<Image> TimelineClipNode::_extract_thumbnail_frame(double p_time) {
    if (thumb_decoder.is_null()) {
        thumb_decoder.instantiate();
    }
    if (!thumb_decoder->is_open()) {
        if (!thumb_decoder->open(clip->get_source_path())) {
            return Ref<Image>();
        }
    }

    int vw = thumb_decoder->get_video_width();
    int vh = thumb_decoder->get_video_height();
    if (vw <= 0 || vh <= 0) {
        return Ref<Image>();
    }

    float scale = Math::min(thumb_size / (float)vw, thumb_size / (float)vh);
    int dw = Math::round(vw * scale);
    int dh = Math::round(vh * scale);
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;

    thumb_decoder->seek(p_time);
    Ref<Image> frame = thumb_decoder->read_video_frame_scaled(dw, dh);
    if (frame.is_null()) {
        return Ref<Image>();
    }

    Ref<Image> square = Image::create(thumb_size, thumb_size, false, Image::FORMAT_RGBA8);
    if (square.is_null()) {
        return frame;
    }
    square->fill(Color(0.05f, 0.05f, 0.07f, 1.0f));

    int ox = (thumb_size - dw) / 2;
    int oy = (thumb_size - dh) / 2;
    square->blit_rect(frame, Rect2i(0, 0, dw, dh), Vector2i(ox, oy));

    return square;
}

void TimelineClipNode::_ensure_thumbnails() {
    if (!thumbnails_dirty || display_mode != DISPLAY_THUMBNAILS || clip.is_null()) {
        return;
    }

    thumbnail_textures.clear();

    double duration = clip->get_duration();
    double source_in = clip->get_source_in_point();
    if (duration <= 0.0) {
        thumbnails_dirty = false;
        return;
    }

    float w = get_size().x;
    int count = Math::max(1, (int)(w / thumb_size));

    for (int i = 0; i < count; i++) {
        double t = source_in + (duration * i / Math::max(1, count - 1));
        Ref<Image> img = _extract_thumbnail_frame(t);
        if (img.is_valid()) {
            Ref<ImageTexture> tex;
            tex.instantiate();
            tex->set_image(img);
            thumbnail_textures.push_back(tex);
        } else {
            thumbnail_textures.push_back(Ref<ImageTexture>());
        }
    }

    if (thumb_decoder.is_valid()) {
        thumb_decoder->close();
    }
    thumbnails_dirty = false;
}

void TimelineClipNode::refresh_thumbnails() {
    thumbnails_dirty = true;
    _ensure_thumbnails();
    queue_redraw();
}

void TimelineClipNode::_draw_split_handle(const Rect2 &p_rect) {
    float split_w = Math::min(20.0f, p_rect.size.x * 0.3f);
    float split_h = 10.0f;
    if (split_w <= 8.0f) return;

    float hx = p_rect.position.x + (p_rect.size.x - split_w) * 0.5f;
    float hy = p_rect.position.y + 2.0f;

    Vector2 top(hx + split_w * 0.5f, hy);
    Vector2 bl(hx, hy + split_h);
    Vector2 br(hx + split_w, hy + split_h);

    PackedVector2Array poly;
    poly.push_back(top);
    poly.push_back(br);
    poly.push_back(bl);
    draw_colored_polygon(poly, split_handle_color);

    draw_line(Vector2(hx + split_w * 0.5f, hy), Vector2(hx + split_w * 0.5f, hy + split_h), split_line_color, 1.0f);
}

void TimelineClipNode::_draw_solid(const Rect2 &p_rect) {
    draw_rect(p_rect, base_color, true);

    for (int x = 0; x < int(p_rect.size.x); x += 4) {
        float t = float(x) / p_rect.size.x;
        Color c = base_color.lightened(0.05f * Math::sin(t * Math_PI));
        draw_line(Vector2(x, p_rect.position.y + 2), Vector2(x, p_rect.position.y + p_rect.size.y - 2), c, 2.0f);
    }

    _draw_split_handle(p_rect);

    if (selected) {
        draw_rect(p_rect, selection_border_color, false, selection_border_width);
        float hw = Math::min(handle_width, p_rect.size.x * 0.25f);
        if (hw > 4.0f) {
            Rect2 left_handle = Rect2(p_rect.position + Vector2(2, 4), Vector2(hw - 2, p_rect.size.y - 8));
            Rect2 right_handle = Rect2(p_rect.position + Vector2(p_rect.size.x - hw, 4), Vector2(hw - 2, p_rect.size.y - 8));
            draw_rect(left_handle, selection_handle_color, true);
            draw_rect(right_handle, selection_handle_color, true);

            float grip_x1 = p_rect.position.x + hw * 0.5f;
            float grip_x2 = p_rect.position.x + p_rect.size.x - hw * 0.5f;
            for (int i = -1; i <= 1; i++) {
                float gy = p_rect.position.y + p_rect.size.y * 0.5f + i * 4.0f;
                draw_line(Vector2(grip_x1 - 2, gy), Vector2(grip_x1 + 2, gy), selection_grip_color, 1.5f);
                draw_line(Vector2(grip_x2 - 2, gy), Vector2(grip_x2 + 2, gy), selection_grip_color, 1.5f);
            }
        }
    } else {
        draw_rect(p_rect, Color(0.3f, 0.3f, 0.4f), false, 1.0f);
    }
}

void TimelineClipNode::_draw_thumbnails(const Rect2 &p_rect) {
    _ensure_thumbnails();

    draw_rect(p_rect, Color(0.05f, 0.05f, 0.07f, 1.0f), true);

    int count = thumbnail_textures.size();
    if (count == 0) {
        _draw_solid(p_rect);
        return;
    }

    float x = p_rect.position.x;
    float y = p_rect.position.y;
    float h = p_rect.size.y;
    float avail_w = p_rect.size.x;
    float thumb_w = avail_w / (float)count;

    for (int i = 0; i < count; i++) {
        Ref<ImageTexture> tex = thumbnail_textures[i];
        Rect2 slice = Rect2(Vector2(x + i * thumb_w, y), Vector2(thumb_w, h));

        if (tex.is_valid()) {
            float draw_size = h;
            float draw_x = slice.position.x + (thumb_w - draw_size) * 0.5f;
            draw_texture_rect(tex, Rect2(Vector2(draw_x, y), Vector2(draw_size, draw_size)), false);
        } else {
            draw_rect(slice, Color(0.08f, 0.08f, 0.10f, 1.0f), true);
        }
    }

    _draw_split_handle(p_rect);

    if (selected) {
        draw_rect(p_rect, selection_border_color, false, selection_border_width);
        float hw = Math::min(handle_width, p_rect.size.x * 0.25f);
        if (hw > 4.0f) {
            Rect2 left_handle = Rect2(p_rect.position + Vector2(2, 4), Vector2(hw - 2, p_rect.size.y - 8));
            Rect2 right_handle = Rect2(p_rect.position + Vector2(p_rect.size.x - hw, 4), Vector2(hw - 2, p_rect.size.y - 8));
            draw_rect(left_handle, selection_handle_color, true);
            draw_rect(right_handle, selection_handle_color, true);

            float grip_x1 = p_rect.position.x + hw * 0.5f;
            float grip_x2 = p_rect.position.x + p_rect.size.x - hw * 0.5f;
            for (int i = -1; i <= 1; i++) {
                float gy = p_rect.position.y + p_rect.size.y * 0.5f + i * 4.0f;
                draw_line(Vector2(grip_x1 - 2, gy), Vector2(grip_x1 + 2, gy), selection_grip_color, 1.5f);
                draw_line(Vector2(grip_x2 - 2, gy), Vector2(grip_x2 + 2, gy), selection_grip_color, 1.5f);
            }
        }
    } else {
        draw_rect(p_rect, Color(0.3f, 0.3f, 0.4f), false, 1.0f);
    }
}

void TimelineClipNode::_draw() {
    Rect2 rect = Rect2(Vector2(), get_size());

    if (display_mode == DISPLAY_THUMBNAILS) {
        _draw_thumbnails(rect);
    } else {
        _draw_solid(rect);
    }

    if (rect.size.x > 60.0f && clip.is_valid()) {
        Ref<Font> draw_font = font.is_valid() ? font : get_theme_default_font();
        int draw_size = font_size > 0 ? font_size : 10;
        String fname = clip->get_source_path().get_file();
        Vector2 text_pos = Vector2(5.0f, rect.size.y * 0.5f + 5.0f);

        draw_string(draw_font, text_pos + Vector2(1, 1), fname, HORIZONTAL_ALIGNMENT_LEFT, -1, draw_size, Color(0, 0, 0, 0.8f));
        draw_string(draw_font, text_pos, fname, HORIZONTAL_ALIGNMENT_LEFT, -1, draw_size, label_color);
    }
}

void TimelineClipNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (clip.is_null()) return;

    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            Vector2 pos = touch->get_position();
            float w = get_size().x;

            // Check split handle first (top center)
            float split_w = Math::min(20.0f, w * 0.3f);
            float split_h = 10.0f;
            if (split_w > 8.0f) {
                Rect2 split_rect(Vector2((w - split_w) * 0.5f, 2.0f), Vector2(split_w, split_h));
                if (split_rect.has_point(pos)) {
                    double local_time = pos.x / (pixels_per_second * zoom);
                    double timeline_time = clip->get_timeline_start() + local_time;
                    emit_signal("split_requested", timeline_time);
                    return;
                }
            }

            if (pos.x < handle_width && w > handle_width * 2.5f) {
                drag_mode = DRAG_TRIM_LEFT;
            } else if (pos.x > w - handle_width && w > handle_width * 2.5f) {
                drag_mode = DRAG_TRIM_RIGHT;
            } else {
                drag_mode = DRAG_MOVE;
                set_selected(true);
                emit_signal("selected");
            }

            drag_start_pos = pos;
            drag_start_timeline_start = clip->get_timeline_start();
            drag_start_duration = clip->get_duration();
            drag_start_source_in = clip->get_source_in_point();
            drag_start_source_out = clip->get_source_out_point();
        } else {
            drag_mode = DRAG_NONE;
        }
        return;
    }

    Ref<InputEventScreenDrag> drag = p_event;
    if (drag.is_valid() && drag_mode != DRAG_NONE && drag->get_index() == 0) {
        Vector2 delta = drag->get_relative();
        double dt = delta.x / (pixels_per_second * zoom);

        switch (drag_mode) {
            case DRAG_MOVE: {
                double new_start = drag_start_timeline_start + dt;
                if (new_start < 0.0) new_start = 0.0;
                clip->set_timeline_start(new_start);
                emit_signal("moved", new_start);
                break;
            }
            case DRAG_TRIM_LEFT: {
                double new_start = drag_start_timeline_start + dt;
                double new_source_in = drag_start_source_in + dt;

                if (new_source_in < 0.0) {
                    new_start -= new_source_in;
                    new_source_in = 0.0;
                }

                double min_dur = 0.1;
                double max_source_in = drag_start_source_out - min_dur;
                if (new_source_in > max_source_in) {
                    new_source_in = max_source_in;
                    new_start = drag_start_timeline_start + (new_source_in - drag_start_source_in);
                }

                clip->set_timeline_start(new_start);
                clip->set_source_in_point(new_source_in);
                emit_signal("trimmed", new_start, clip->get_duration(), new_source_in);
                break;
            }
            case DRAG_TRIM_RIGHT: {
                double new_source_out = drag_start_source_out + dt;
                double min_dur = 0.1;
                if (new_source_out < drag_start_source_in + min_dur) {
                    new_source_out = drag_start_source_in + min_dur;
                }
                clip->set_source_out_point(new_source_out);
                emit_signal("trimmed", clip->get_timeline_start(), clip->get_duration(), clip->get_source_in_point());
                break;
            }
            default:
                break;
        }
        update_layout();
    }
}
