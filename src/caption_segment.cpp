#include "caption_segment.h"

using namespace godot;

void CaptionSegment::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_start_time", "time"), &CaptionSegment::set_start_time);
    ClassDB::bind_method(D_METHOD("get_start_time"), &CaptionSegment::get_start_time);
    ClassDB::add_property("CaptionSegment", PropertyInfo(Variant::FLOAT, "start_time"), "set_start_time", "get_start_time");

    ClassDB::bind_method(D_METHOD("set_end_time", "time"), &CaptionSegment::set_end_time);
    ClassDB::bind_method(D_METHOD("get_end_time"), &CaptionSegment::get_end_time);
    ClassDB::add_property("CaptionSegment", PropertyInfo(Variant::FLOAT, "end_time"), "set_end_time", "get_end_time");

    ClassDB::bind_method(D_METHOD("set_text", "text"), &CaptionSegment::set_text);
    ClassDB::bind_method(D_METHOD("get_text"), &CaptionSegment::get_text);
    ClassDB::add_property("CaptionSegment", PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
}

CaptionSegment::CaptionSegment() {}

void CaptionSegment::set_start_time(double p_time) { start_time = p_time; }
double CaptionSegment::get_start_time() const { return start_time; }

void CaptionSegment::set_end_time(double p_time) { end_time = p_time; }
double CaptionSegment::get_end_time() const { return end_time; }

void CaptionSegment::set_text(const String &p_text) { text = p_text; }
String CaptionSegment::get_text() const { return text; }
