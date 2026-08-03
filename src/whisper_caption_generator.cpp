#include "whisper_caption_generator.h"
#include "video_decoder.h"
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <vector>

extern "C" {
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}

using namespace godot;

void WhisperCaptionGenerator::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_model_path", "path"), &WhisperCaptionGenerator::set_model_path);
    ClassDB::bind_method(D_METHOD("get_model_path"), &WhisperCaptionGenerator::get_model_path);
    ClassDB::add_property("WhisperCaptionGenerator",
        PropertyInfo(Variant::STRING, "model_path", PROPERTY_HINT_FILE, "*.bin"),
        "set_model_path", "get_model_path");
}

WhisperCaptionGenerator::WhisperCaptionGenerator() {}
WhisperCaptionGenerator::~WhisperCaptionGenerator() {
    if (ctx) {
        whisper_free(ctx);
        ctx = nullptr;
    }
}

void WhisperCaptionGenerator::set_model_path(const String &p_path) { model_path = p_path; }
String WhisperCaptionGenerator::get_model_path() const { return model_path; }

TypedArray<CaptionSegment> WhisperCaptionGenerator::generate_captions(const String &p_video_path) {
    TypedArray<CaptionSegment> result;

    if (!ctx && !model_path.is_empty()) {
        String resolved = model_path;
        if (resolved.begins_with("res://") || resolved.begins_with("user://")) {
            if (ProjectSettings *ps = ProjectSettings::get_singleton())
                resolved = ps->globalize_path(resolved);
        }
        whisper_context_params cparams = whisper_context_default_params();
        ctx = whisper_init_from_file_with_params(resolved.utf8().get_data(), cparams);
    }

    if (!ctx) {
        UtilityFunctions::push_error("[WhisperCaptionGenerator] Model not loaded. Set model_path.");
        return result;
    }

    Ref<VideoDecoder> dec;
    dec.instantiate();
    if (!dec->open(p_video_path) || !dec->has_audio()) {
        UtilityFunctions::push_error("[WhisperCaptionGenerator] Failed to open video or no audio: ", p_video_path);
        return result;
    }

    int src_rate = dec->get_audio_sample_rate();
    int src_ch = dec->get_audio_channels();

    SwrContext *swr = nullptr;
    AVChannelLayout src_layout, dst_layout;
    av_channel_layout_default(&src_layout, src_ch);
    av_channel_layout_default(&dst_layout, 1);

    int ret = swr_alloc_set_opts2(&swr,
        &dst_layout, AV_SAMPLE_FMT_FLT, 16000,
        &src_layout, AV_SAMPLE_FMT_FLT, src_rate,
        0, nullptr);

    if (ret < 0 || swr_init(swr) < 0) {
        UtilityFunctions::push_error("[WhisperCaptionGenerator] Resampler init failed");
        swr_free(&swr);
        dec->close();
        return result;
    }

    std::vector<float> pcm16k;
    const int chunk_frames = 4096;

    while (true) {
        PackedFloat32Array chunk = dec->read_audio_samples(chunk_frames * src_ch);
        if (chunk.size() == 0) break;

        int in_frames = chunk.size() / src_ch;
        int max_out = swr_get_out_samples(swr, in_frames);
        if (max_out <= 0) continue;

        size_t prev = pcm16k.size();
        pcm16k.resize(prev + max_out);

        uint8_t *out_ptr = (uint8_t *)(pcm16k.data() + prev);
        const uint8_t *in_ptr = (const uint8_t *)chunk.ptr();

        int got = swr_convert(swr, &out_ptr, max_out, &in_ptr, in_frames);
        if (got > 0) {
            pcm16k.resize(prev + got);
        } else {
            pcm16k.resize(prev);
        }
    }

    swr_free(&swr);
    dec->close();

    if (pcm16k.empty()) {
        UtilityFunctions::push_error("[WhisperCaptionGenerator] Extracted 0 audio samples");
        return result;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.translate = false;
    wparams.language = "en";
    wparams.n_threads = CLAMP(OS::get_singleton()->get_processor_count(), 1, 8);
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;

    if (whisper_full(ctx, wparams, pcm16k.data(), (int)pcm16k.size()) != 0) {
        UtilityFunctions::push_error("[WhisperCaptionGenerator] whisper_full failed");
        return result;
    }

    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; i++) {
        int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        int64_t t1 = whisper_full_get_segment_t1(ctx, i);
        const char *txt = whisper_full_get_segment_text(ctx, i);

        Ref<CaptionSegment> seg;
        seg.instantiate();
        seg->set_start_time(t0 / 1000.0);
        seg->set_end_time(t1 / 1000.0);
        seg->set_text(String(txt).strip_edges());
        result.push_back(seg);
    }

    UtilityFunctions::print("[WhisperCaptionGenerator] Generated ", n_segments, " segments");
    return result;
}
