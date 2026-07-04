shader_type canvas_item;

// Simple transition effects
// transition_type: 0=none, 1=fade, 2=slide_left, 3=slide_right
uniform sampler2D next_video_texture;
uniform int transition_type : hint_range(0, 3) = 0;
uniform float progress : hint_range(0.0, 1.0) = 0.0;

void fragment() {
    vec4 current = texture(TEXTURE, UV);
    vec4 next = texture(next_video_texture, UV);

    if (transition_type == 1) {
        // Fade
        COLOR = mix(current, next, progress);
    } else if (transition_type == 2) {
        // Slide from right
        float slide = progress * 2.0 - 1.0;
        if (UV.x > slide) {
            COLOR = next;
        } else {
            COLOR = current;
        }
    } else if (transition_type == 3) {
        // Slide from left
        float slide = 1.0 - progress * 2.0;
        if (UV.x < slide) {
            COLOR = next;
        } else {
            COLOR = current;
        }
    } else {
        COLOR = current;
    }
}
