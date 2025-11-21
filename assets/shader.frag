#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec4 fragColor;

void main() {
    outColor = vec4(fragColor.r, fragColor.y, fragColor.b, fragColor.a);
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules