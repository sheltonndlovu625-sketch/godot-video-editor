#include "register_types.h"
#include "video_encoder.h"
#include "video_decoder.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

extern "C" {
#include <libavformat/avformat.h>
}

using namespace godot;

void initialize_video_encoder_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        avformat_network_init();
        ClassDB::register_class<VideoEncoder>();
        ClassDB::register_class<VideoDecoder>();
    }
}

void uninitialize_video_encoder_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        avformat_network_deinit();
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT video_encoder_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {
        godot::GDExtensionBinding::InitObject init_obj(
            p_get_proc_address, p_library, r_initialization
        );
        init_obj.register_initializer(initialize_video_encoder_module);
        init_obj.register_terminator(uninitialize_video_encoder_module);
        init_obj.set_minimum_library_initialization_level(
            MODULE_INITIALIZATION_LEVEL_SCENE
        );
        return init_obj.init();
    }
}
