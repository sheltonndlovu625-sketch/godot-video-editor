#ifndef TIMELINE_CLIP_NODE_H
#define TIMELINE_CLIP_NODE_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/color.hpp>
#include "timeline_clip.h"
#include "video_decoder.h"

namespace godot {

class TimelineClipNode : public Control {
    GDCLASS(TimelineClipNode, Control)

public:
    enum DragMode {
        DRAG_NONE,
        DRAG_MOVE,
        DRAG_TRIM_LEFT,
        DRAG_TRIM_RIGHT
    };

    enum DisplayMode {
        DISPLAY_SOLID,
        DISPLAY_THUMBNAILS
    };

private:
    Ref<TimelineClip> clip;
    Color base_color = Color(0.2f, 0.5f, 0.95f);
    bool selected = false;
    DragMode drag_mode = DRAG_NONE;
    Vector2 drag_start_pos;
    double drag_start_timeline_start = 0.0;
    double drag_start_duration = 0.0;
    double drag_start_source_in = 0.0;
    double drag_start_source_out = 0.0;
    float pixels_per_second = 60.0f;
    float zoom = 1.0f;
    float handle_width = 12.0f;
    bool is_video = true;

    // Inspector customisation
    bool use_custom_color = false;
    Color custom_color = Color(1, 1, 1);
    Ref<Font> font;
    int font_size = 10;
    Color label_color = Color(1, 1, 1);

    // ---- NEW: Selection customisation ----
    Color selection_border_color = Color(1.0f, 0.85f, 0.2f);  // KineMaster-style yellow
    float selection_border_width = 2.5f;
    Color selection_handle_color = Color(1, 1, 1, 0.25f);
    Color selection_grip_color = Color(1, 1, 1);
    Color split_handle_color = Color(1, 1, 1, 0.9f);
    Color split_line_color = Color(0.2f, 0.2f, 0.2f);
    // --------------------------------------

    // Thumbnails
    DisplayMode display_mode = DISPLAY_SOLID;
    float thumb_size = 48.0f;
    Ref<VideoDecoder> thumb_decoder;
    Vector<Ref<ImageTexture>> thumbnail_textures;
    bool thumbnails_dirty = true;

    void _draw_solid(const Rect2 &p_rect);
    void _draw_thumbnails(const Rect2 &p_rect);
    void _draw_split_handle(const Rect2 &p_rect);
    void _ensure_thumbnails();
    Ref<Image> _extract_thumbnail_frame(double p_time);

protected:
    static void _bind_methods();

public:
    void _draw() override;
    void _gui_input(const Ref<InputEvent> &p_event) override;

    void set_clip(const Ref<TimelineClip> &p_clip);
    Ref<TimelineClip> get_clip() const;
    void set_selected(bool p_selected);
    bool is_selected() const;
    void set_pixels_per_second(float p_pps);
    float get_pixels_per_second() const;
    void set_zoom(float p_zoom);
    float get_zoom() const;
    void set_is_video(bool p_video);
    bool get_is_video() const;
    void update_layout();

    // Customisation
    void set_custom_color(const Color &p_color);
    Color get_custom_color() const;
    void set_use_custom_color(bool p_use);
    bool get_use_custom_color() const;

    void set_font(const Ref<Font> &p_font);
    Ref<Font> get_font() const;
    void set_font_size(int p_size);
    int get_font_size() const;
    void set_label_color(const Color &p_color);
    Color get_label_color() const;

    // ---- NEW: Selection inspector properties ----
    void set_selection_border_color(const Color &p_color);
    Color get_selection_border_color() const;
    void set_selection_border_width(float p_width);
    float get_selection_border_width() const;
    void set_selection_handle_color(const Color &p_color);
    Color get_selection_handle_color() const;
    void set_handle_width(float p_width);
    float get_handle_width() const;
    void set_selection_grip_color(const Color &p_color);
    Color get_selection_grip_color() const;
    void set_split_handle_color(const Color &p_color);
    Color get_split_handle_color() const;
    void set_split_line_color(const Color &p_color);
    Color get_split_line_color() const;
    // -------------------------------------------

    // Display mode
    void set_display_mode(int p_mode);
    int get_display_mode() const;
    void set_thumb_size(float p_size);
    float get_thumb_size() const;
    void refresh_thumbnails();

    TimelineClipNode();
};

}

VARIANT_ENUM_CAST(TimelineClipNode::DragMode);
VARIANT_ENUM_CAST(TimelineClipNode::DisplayMode);

#endif
