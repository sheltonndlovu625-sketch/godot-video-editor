// project_serializer.h
#ifndef PROJECT_SERIALIZER_H
#define PROJECT_SERIALIZER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include "timeline.h"

namespace godot {

class ProjectSerializer : public RefCounted {
    GDCLASS(ProjectSerializer, RefCounted)

protected:
    static void _bind_methods();

public:
    String serialize_timeline(const Ref<Timeline> &p_timeline) const;
    Ref<Timeline> deserialize_timeline(const String &p_json) const;

    bool save_timeline_to_file(const Ref<Timeline> &p_timeline, const String &p_path) const;
    Ref<Timeline> load_timeline_from_file(const String &p_path) const;

    ProjectSerializer();
};

}

#endif
