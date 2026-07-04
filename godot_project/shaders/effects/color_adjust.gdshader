shader_type canvas_item;

// Post-processing effects for video editing
// Apply this to an overlay ColorRect for real-time preview

uniform float brightness : hint_range(0.0, 3.0) = 1.0;
uniform float contrast : hint_range(0.0, 3.0) = 1.0;
uniform float saturation : hint_range(0.0, 3.0) = 1.0;
uniform float fade_amount : hint_range(0.0, 1.0) = 0.0;

void fragment() {
    vec4 color = texture(TEXTURE, UV);

    // Brightness
    color.rgb *= brightness;

    // Contrast
    color.rgb = (color.rgb - 0.5) * contrast + 0.5;

    // Saturation
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = mix(vec3(gray), color.rgb, saturation);

    // Fade to black (transition effect)
    color.rgb *= (1.0 - fade_amount);

    COLOR = color;
}
