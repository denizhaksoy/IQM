// HDR-VDP-3 Local Adaptation - Crop and Exp Pass
// Crops 128x128 to 64x64 and applies exp()

#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform readonly image2D input_img;   // 128x128
layout(set = 0, binding = 1, r32f) uniform writeonly image2D output_img; // 64x64

layout(push_constant) uniform PushConstants {
    float ppd;
} push_consts;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 out_size = imageSize(output_img);  // 64x64
    
    if (coord.x >= out_size.x || coord.y >= out_size.y) {
        return;
    }
    
    // Crop from 128x128 (take top-left 64x64)
    float val = imageLoad(input_img, coord).r;
    float result = exp(val);
    
    imageStore(output_img, coord, vec4(result));
}