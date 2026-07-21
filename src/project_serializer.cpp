// project_serializer.cpp
#include "project_serializer.h"
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "timeline_track.h"
#include "timeline_clip.h"
#include "text_overlay.h"
#include "image_overlay.h"
#include "color_correction_effect.h"

using namespace godot;

void ProjectSerializer::_bind_methods() {
    ClassDB::bind_method(D_METHOD("serialize_timeline", "timeline"), &ProjectSerializer::serialize_timeline);
    ClassDB::bind_method(D_METHOD("deserialize_timeline", "json"), &ProjectSerializer::deserialize_timeline);
    ClassDB::bind_method(D_METHOD("save_timeline_to_file", "timeline", "path"), &ProjectSerializer::save_timeline_to_file);
    ClassDB::bind_method(D_METHOD("load_timeline_from_file", "path"), &ProjectSerializer::load_timeline_from_file);
}

ProjectSerializer::ProjectSerializer() {}

String ProjectSerializer::serialize_timeline(const Ref<Timeline> &p_timeline) const {
    if (p_timeline.is_null()) return "";

    Dictionary root;
    root["version"] = 1;
    root["frame_rate"] = p_timeline->get_frame_rate();
    root["playhead_time"] = p_timeline->get_playhead_time();

    Array tracks_arr;
    for (int i = 0; i < p_timeline->get_track_count(); i++) {
        Ref<TimelineTrack> track = p_timeline->get_track(i);
        if (track.is_null()) continue;

        Dictionary track_dict;
        track_dict["type"] = track->get_track_type();
        track_dict["layer_index"] = track->get_layer_index();
        track_dict["blend_mode"] = track->get_blend_mode();

        Array clips_arr;
        TypedArray<TimelineClip> clips = track->get_clips();
        for (int j = 0; j < clips.size(); j++) {
            Ref<TimelineClip> clip = clips[j];
            if (clip.is_null()) continue;

            Dictionary clip_dict;
            clip_dict["source_path"] = clip->get_source_path();
            clip_dict["timeline_start"] = clip->get_timeline_start();
            clip_dict["source_in_point"] = clip->get_source_in_point();
            clip_dict["source_out_point"] = clip->get_source_out_point();
            clip_dict["playback_speed"] = clip->get_playback_speed();
            clip_dict["position_x"] = clip->get_position().x;
            clip_dict["position_y"] = clip->get_position().y;
            clip_dict["scale_x"] = clip->get_scale().x;
            clip_dict["scale_y"] = clip->get_scale().y;
            clip_dict["rotation"] = clip->get_rotation();
            clip_dict["opacity"] = clip->get_opacity();
            clip_dict["anchor_x"] = clip->get_anchor_point().x;
            clip_dict["anchor_y"] = clip->get_anchor_point().y;

            Array effects_arr;
            TypedArray<VideoEffect> effects = clip->get_effects();
            for (int k = 0; k < effects.size(); k++) {
                Ref<ColorCorrectionEffect> cc = effects[k];
                if (cc.is_valid()) {
                    Dictionary fx;
                    fx["type"] = "color_correction";
                    fx["brightness"] = cc->get_brightness();
                    fx["contrast"] = cc->get_contrast();
                    fx["saturation"] = cc->get_saturation();
                    effects_arr.push_back(fx);
                }
            }
            clip_dict["effects"] = effects_arr;
            clips_arr.push_back(clip_dict);
        }
        track_dict["clips"] = clips_arr;
        tracks_arr.push_back(track_dict);
    }
    root["tracks"] = tracks_arr;

    Array text_arr;
    TypedArray<TextOverlay> texts = p_timeline->get_all_text_overlays();
    for (int i = 0; i < texts.size(); i++) {
        Ref<TextOverlay> ov = texts[i];
        if (ov.is_null()) continue;
        Dictionary d;
        d["text"] = ov->get_text();
        d["start_time"] = ov->get_start_time();
        d["end_time"] = ov->get_end_time();
        d["position_x"] = ov->get_position().x;
        d["position_y"] = ov->get_position().y;
        d["font_size"] = ov->get_font_size();
        d["color_r"] = ov->get_color().r;
        d["color_g"] = ov->get_color().g;
        d["color_b"] = ov->get_color().b;
        d["color_a"] = ov->get_color().a;
        d["animation"] = ov->get_animation();
        d["animation_duration"] = ov->get_animation_duration();
        text_arr.push_back(d);
    }
    root["text_overlays"] = text_arr;

    Array img_arr;
    TypedArray<ImageOverlay> imgs = p_timeline->get_all_image_overlays();
    for (int i = 0; i < imgs.size(); i++) {
        Ref<ImageOverlay> ov = imgs[i];
        if (ov.is_null()) continue;
        Dictionary d;
        d["texture_path"] = ov->get_texture_path();
        d["start_time"] = ov->get_start_time();
        d["end_time"] = ov->get_end_time();
        d["position_x"] = ov->get_position().x;
        d["position_y"] = ov->get_position().y;
        d["scale_x"] = ov->get_scale().x;
        d["scale_y"] = ov->get_scale().y;
        d["rotation"] = ov->get_rotation();
        d["opacity"] = ov->get_opacity();
        img_arr.push_back(d);
    }
    root["image_overlays"] = img_arr;

    return JSON::stringify(root);
}

Ref<Timeline> ProjectSerializer::deserialize_timeline(const String &p_json) const {
    Variant result = JSON::parse_string(p_json);
    if (result.get_type() != Variant::DICTIONARY) {
        UtilityFunctions::push_error("[ProjectSerializer] Invalid JSON format");
        return Ref<Timeline>();
    }

    Dictionary root = result;
    Ref<Timeline> timeline;
    timeline.instantiate();

    if (root.has("frame_rate")) timeline->set_frame_rate(root["frame_rate"]);
    if (root.has("playhead_time")) timeline->set_playhead_time(root["playhead_time"]);

    if (root.has("tracks")) {
        Array tracks_arr = root["tracks"];
        for (int i = 0; i < tracks_arr.size(); i++) {
            Dictionary track_dict = tracks_arr[i];
            Ref<TimelineTrack> track;
            track.instantiate();

            if (track_dict.has("type")) track->set_track_type(track_dict["type"]);
            if (track_dict.has("layer_index")) track->set_layer_index(track_dict["layer_index"]);
            if (track_dict.has("blend_mode")) track->set_blend_mode(track_dict["blend_mode"]);

            if (track_dict.has("clips")) {
                Array clips_arr = track_dict["clips"];
                for (int j = 0; j < clips_arr.size(); j++) {
                    Dictionary clip_dict = clips_arr[j];
                    Ref<TimelineClip> clip;
                    clip.instantiate();

                    if (clip_dict.has("source_path")) clip->set_source_path(clip_dict["source_path"]);
                    if (clip_dict.has("timeline_start")) clip->set_timeline_start(clip_dict["timeline_start"]);
                    if (clip_dict.has("source_in_point")) clip->set_source_in_point(clip_dict["source_in_point"]);
                    if (clip_dict.has("source_out_point")) clip->set_source_out_point(clip_dict["source_out_point"]);
                    if (clip_dict.has("playback_speed")) clip->set_playback_speed(clip_dict["playback_speed"]);

                    Vector2 pos;
                    if (clip_dict.has("position_x")) pos.x = clip_dict["position_x"];
                    if (clip_dict.has("position_y")) pos.y = clip_dict["position_y"];
                    clip->set_position(pos);

                    Vector2 scl(1, 1);
                    if (clip_dict.has("scale_x")) scl.x = clip_dict["scale_x"];
                    if (clip_dict.has("scale_y")) scl.y = clip_dict["scale_y"];
                    clip->set_scale(scl);

                    if (clip_dict.has("rotation")) clip->set_rotation(clip_dict["rotation"]);
                    if (clip_dict.has("opacity")) clip->set_opacity(clip_dict["opacity"]);

                    Vector2 anchor(0.5, 0.5);
                    if (clip_dict.has("anchor_x")) anchor.x = clip_dict["anchor_x"];
                    if (clip_dict.has("anchor_y")) anchor.y = clip_dict["anchor_y"];
                    clip->set_anchor_point(anchor);

                    if (clip_dict.has("effects")) {
                        Array effects_arr = clip_dict["effects"];
                        for (int k = 0; k < effects_arr.size(); k++) {
                            Dictionary fx = effects_arr[k];
                            String type = fx.get("type", "");
                            if (type == "color_correction") {
                                Ref<ColorCorrectionEffect> cc;
                                cc.instantiate();
                                if (fx.has("brightness")) cc->set_brightness(fx["brightness"]);
                                if (fx.has("contrast")) cc->set_contrast(fx["contrast"]);
                                if (fx.has("saturation")) cc->set_saturation(fx["saturation"]);
                                clip->add_effect(cc);
                            }
                        }
                    }
                    track->add_clip(clip);
                }
            }
            timeline->add_track(track);
        }
    }

    if (root.has("text_overlays")) {
        Array text_arr = root["text_overlays"];
        for (int i = 0; i < text_arr.size(); i++) {
            Dictionary d = text_arr[i];
            Ref<TextOverlay> ov;
            ov.instantiate();
            if (d.has("text")) ov->set_text(d["text"]);
            if (d.has("start_time")) ov->set_start_time(d["start_time"]);
            if (d.has("end_time")) ov->set_end_time(d["end_time"]);
            Vector2 pos;
            if (d.has("position_x")) pos.x = d["position_x"];
            if (d.has("position_y")) pos.y = d["position_y"];
            ov->set_position(pos);
            if (d.has("font_size")) ov->set_font_size(d["font_size"]);
            if (d.has("color_r") && d.has("color_g") && d.has("color_b") && d.has("color_a")) {
                ov->set_color(Color(d["color_r"], d["color_g"], d["color_b"], d["color_a"]));
            }
            if (d.has("animation")) ov->set_animation(d["animation"]);
            if (d.has("animation_duration")) ov->set_animation_duration(d["animation_duration"]);
            timeline->add_text_overlay(ov);
        }
    }

    if (root.has("image_overlays")) {
        Array img_arr = root["image_overlays"];
        for (int i = 0; i < img_arr.size(); i++) {
            Dictionary d = img_arr[i];
            Ref<ImageOverlay> ov;
            ov.instantiate();
            if (d.has("texture_path")) ov->set_texture_path(d["texture_path"]);
            if (d.has("start_time")) ov->set_start_time(d["start_time"]);
            if (d.has("end_time")) ov->set_end_time(d["end_time"]);
            Vector2 pos;
            if (d.has("position_x")) pos.x = d["position_x"];
            if (d.has("position_y")) pos.y = d["position_y"];
            ov->set_position(pos);
            Vector2 scl(1, 1);
            if (d.has("scale_x")) scl.x = d["scale_x"];
            if (d.has("scale_y")) scl.y = d["scale_y"];
            ov->set_scale(scl);
            if (d.has("rotation")) ov->set_rotation(d["rotation"]);
            if (d.has("opacity")) ov->set_opacity(d["opacity"]);
            timeline->add_image_overlay(ov);
        }
    }

    return timeline;
}

bool ProjectSerializer::save_timeline_to_file(const Ref<Timeline> &p_timeline, const String &p_path) const {
    String json = serialize_timeline(p_timeline);
    if (json.is_empty()) return false;

    Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
    if (f.is_null()) {
        UtilityFunctions::push_error("[ProjectSerializer] Cannot write: ", p_path);
        return false;
    }
    f->store_string(json);
    return true;
}

Ref<Timeline> ProjectSerializer::load_timeline_from_file(const String &p_path) const {
    Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
    if (f.is_null()) {
        UtilityFunctions::push_error("[ProjectSerializer] Cannot read: ", p_path);
        return Ref<Timeline>();
    }
    return deserialize_timeline(f->get_as_text());
}
