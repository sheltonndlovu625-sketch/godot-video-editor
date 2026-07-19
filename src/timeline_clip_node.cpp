#include "timeline_clip_node.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void TimelineClipNode::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_clip", "clip"), &TimelineClipNode::set_clip);
    ClassDB::bind_method(D_METHOD("get_clip"), &TimelineClipNode::get_clip);
    
    ClassDB::bind_method(D_METHOD("set_selected", "selected"), &TimelineClipNode::set_selected);
    ClassDB::bind_method(D_METHOD("is_selected"), &TimelineClipNode::is_selected);
    
    ClassDB::bind_method(D_METHOD("set_pixels_per_second", "pps"), &TimelineClipNode::set_pixels_per_second);
    ClassDB::bind_method(D_METHOD("get_pixels_per_second"), &TimelineClipNode::get_pixels_per_second);
    
    ClassDB::bind_method(D_METHOD("set_zoom", "zoom"), &TimelineClipNode::set_zoom);
    ClassDB::bind_method(D_METHOD("get_zoom"), &TimelineClipNode::get_zoom);
    
    ClassDB::bind_method(D_METHOD("set_is_video", "is_video"), &TimelineClipNode::set_is_video);
    ClassDB::bind_method(D_METHOD("get_is_video"), &TimelineClipNode::get_is_video);
    
    ClassDB::bind_method(D_METHOD("update_layout"), &TimelineClipNode::update_layout);
    
    ADD_SIGNAL(MethodInfo("selected"));
    ADD_SIGNAL(MethodInfo("moved", PropertyInfo(Variant::FLOAT, "new_start")));
    ADD_SIGNAL(MethodInfo("trimmed", PropertyInfo(Variant::FLOAT, "new_start"), PropertyInfo(Variant::FLOAT, "new_duration"), PropertyInfo(Variant::FLOAT, "new_source_in")));
    
    BIND_ENUM_CONSTANT(DRAG_NONE);
    BIND_ENUM_CONSTANT(DRAG_MOVE);
    BIND_ENUM_CONSTANT(DRAG_TRIM_LEFT);
    BIND_ENUM_CONSTANT(DRAG_TRIM_RIGHT);
}

TimelineClipNode::TimelineClipNode() {
    set_clip_contents(true);
}

void TimelineClipNode::set_clip(const Ref<TimelineClip> &p_clip) {
    clip = p_clip;
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
    base_color = is_video ? Color(0.2f, 0.5f, 0.95f) : Color(0.25f, 0.8f, 0.35f);
    queue_redraw();
}

bool TimelineClipNode::get_is_video() const {
    return is_video;
}

void TimelineClipNode::update_layout() {
    if (clip.is_null()) return;
    double dur = clip->get_duration();
    float w = maxf(24.0f, (float)(dur * pixels_per_second * zoom));
    set_custom_minimum_size(Vector2(w, 0));
    set_size(Vector2(w, get_size().y));
}

void TimelineClipNode::_draw() {
    Rect2 rect = Rect2(Vector2(), get_size());
    
    // Background
    draw_rect(rect, base_color, true);
    
    // Subtle stripe texture
    for (int x = 0; x < int(rect.size.x); x += 4) {
        float t = float(x) / rect.size.x;
        Color c = base_color.lightened(0.05f * Math::sin(t * Math_PI));
        draw_line(Vector2(x, rect.position.y + 2), Vector2(x, rect.position.y + rect.size.y - 2), c, 2.0f);
    }
    
    // Border
    if (selected) {
        draw_rect(rect, Color(1, 1, 1), false, 2.5f);
        
        float hw = minf(handle_width, rect.size.x * 0.25f);
        if (hw > 4.0f) {
            Rect2 left_handle = Rect2(rect.position + Vector2(2, 4), Vector2(hw - 2, rect.size.y - 8));
            Rect2 right_handle = Rect2(rect.position + Vector2(rect.size.x - hw, 4), Vector2(hw - 2, rect.size.y - 8));
            
            draw_rect(left_handle, Color(1, 1, 1, 0.25f), true);
            draw_rect(right_handle, Color(1, 1, 1, 0.25f), true);
            
            float grip_x1 = rect.position.x + hw * 0.5f;
            float grip_x2 = rect.position.x + rect.size.x - hw * 0.5f;
            for (int i = -1; i <= 1; i++) {
                float gy = rect.position.y + rect.size.y * 0.5f + i * 4.0f;
                draw_line(Vector2(grip_x1 - 2, gy), Vector2(grip_x1 + 2, gy), Color(1, 1, 1), 1.5f);
                draw_line(Vector2(grip_x2 - 2, gy), Vector2(grip_x2 + 2, gy), Color(1, 1, 1), 1.5f);
            }
        }
    } else {
        draw_rect(rect, Color(0.3f, 0.3f, 0.4f), false, 1.0f);
    }
    
    // Label
    if (rect.size.x > 60.0f && clip.is_valid()) {
        String fname = clip->get_source_path().get_file();
        Vector2 text_pos = Vector2(5.0f, rect.size.y * 0.5f + 5.0f);
        draw_string(get_theme_default_font(), text_pos + Vector2(1, 1), fname, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(0, 0, 0, 0.8f));
        draw_string(get_theme_default_font(), text_pos, fname, HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(1, 1, 1));
    }
}

void TimelineClipNode::_gui_input(const Ref<InputEvent> &p_event) {
    if (clip.is_null()) return;
    
    Ref<InputEventScreenTouch> touch = p_event;
    if (touch.is_valid() && touch->get_index() == 0) {
        if (touch->is_pressed()) {
            Vector2 pos = touch->get_position();
            float w = get_size().x;
            
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
