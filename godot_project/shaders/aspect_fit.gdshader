shader_type canvas_item;

// TikTok-style aspect-fit shader
// Maintains 9:16 ratio without stretching
// cover_mode = true  -> fills screen (crops edges, like TikTok)
// cover_mode = false -> fits inside (letterbox, black bars)

uniform vec2 video_size = vec2(1080.0, 1920.0);
uniform bool cover_mode = true;

void fragment() {
    vec2 uv = UV;
    vec2 screen_size = vec2(1.0 / SCREEN_PIXEL_SIZE.x, 1.0 / SCREEN_PIXEL_SIZE.y);

    float video_ratio = video_size.x / video_size.y;
    float screen_ratio = screen_size.x / screen_size.y;

    vec2 scale;
    if (cover_mode) {
        // Cover: fill entire area, crop if needed
        scale = vec2(
            max(screen_ratio / video_ratio, 1.0),
            max(video_ratio / screen_ratio, 1.0)
        );
    } else {
        // Contain: fit entirely, black bars if needed
        scale = vec2(
            min(screen_ratio / video_ratio, 1.0),
            min(video_ratio / screen_ratio, 1.0)
        );
    }

    vec2 centered_uv = (uv - 0.5) * scale + 0.5;

    // Sample from the viewport texture (assigned in GDScript)
    if (centered_uv.x < 0.0 || centered_uv.x > 1.0 || 
        centered_uv.y < 0.0 || centered_uv.y > 1.0) {
        COLOR = vec4(0.0, 0.0, 0.0, 1.0); // Black bars
    } else {
        COLOR = texture(TEXTURE, centered_uv);
    }
}
