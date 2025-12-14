#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) out vec4 outColor;

void main() {
    outColor = vec4(fragColor.r, fragColor.y, fragColor.b, fragColor.a);
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules
