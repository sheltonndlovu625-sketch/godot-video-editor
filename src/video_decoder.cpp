#include "video_decoder.h"
#include <godot_cpp/classes/project_settings.hpp>

using namespace godot;

namespace {
    String resolve_path(String p_path) {
        if (p_path.begins_with("user://") || p_path.begins_with("res://")) {
            ProjectSettings *ps = ProjectSettings::get_singleton();
            if (ps) {
                return ps->globalize_path(p_path);
            }
        }
        return p_path;
    }

    void log_av_error(const char *prefix, int errnum) {
        char errbuf[256];
        av_strerror(errnum, errbuf, sizeof(errbuf));
        UtilityFunctions::push_error(String("[VideoDecoder] ") + prefix + ": " + errbuf);
    }
} // anonymous namespace

// ... rest of the file is identical to the original source_code.md
