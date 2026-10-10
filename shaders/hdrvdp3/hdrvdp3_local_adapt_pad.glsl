// HDR-VDP-3 Local Adaptation - Pad Pass
// Pads 64x64 to 128x128 with replicate on bottom/right

#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform readonly image2D input_img;  // 64x64
layout(set = 0, binding = 1, r32f) uniform writeonly image2D output_img; // 128x128

layout(push_constant) uniform PushConstants {
    float ppd;
} push_consts;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 out_size = imageSize(output_img);  // 128x128
    ivec2 in_size = imageSize(input_img);    // 64x64
    
    if (coord.x >= out_size.x || coord.y >= out_size.y) {
        return;
    }
    
    // Map output coordinate to input with replicate on bottom/right
    int src_x = min(coord.x, in_size.x - 1);
    int src_y = min(coord.y, in_size.y - 1);
    
    float val = imageLoad(input_img, ivec2(src_x, src_y)).r;
    imageStore(output_img, coord, vec4(val));
}