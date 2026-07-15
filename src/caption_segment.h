#ifndef CAPTION_SEGMENT_H
#define CAPTION_SEGMENT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class CaptionSegment : public RefCounted {
    GDCLASS(CaptionSegment, RefCounted)

private:
    double start_time = 0.0;
    double end_time = 0.0;
    String text;

protected:
    static void _bind_methods();

public:
    void set_start_time(double p_time);
    double get_start_time() const;

    void set_end_time(double p_time);
    double get_end_time() const;

    void set_text(const String &p_text);
    String get_text() const;

    CaptionSegment();
};

}

#endif
