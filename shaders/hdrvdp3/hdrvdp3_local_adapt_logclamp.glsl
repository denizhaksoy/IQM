// HDR-VDP-3 Local Adaptation - Log Clamp Pass
// log(max(input, 1e-6))

#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform readonly image2D input_img;
layout(set = 0, binding = 1, r32f) uniform writeonly image2D temp_img;

layout(push_constant) uniform PushConstants {
    float ppd;
} push_consts;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(input_img);
    
    if (coord.x >= size.x || coord.y >= size.y) {
        return;
    }
    
    float val = imageLoad(input_img, coord).r;
    float clamped = max(val, 1e-6);
    float result = log(clamped);
    
    imageStore(temp_img, coord, vec4(result));
}