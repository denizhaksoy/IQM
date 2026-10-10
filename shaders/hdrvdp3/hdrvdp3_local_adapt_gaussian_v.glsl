// HDR-VDP-3 Local Adaptation - Vertical Gaussian Pass
// Separable 1D Gaussian convolution on 128x128

#version 450
#pragma shader_stage(compute)

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform readonly image2D input_img;   // 128x128
layout(set = 0, binding = 1, r32f) uniform writeonly image2D output_img; // 128x128
layout(set = 0, binding = 6, std430) readonly buffer KernelBuffer {
    float kernel[];
};

layout(push_constant) uniform PushConstants {
    float ppd;
    int kernel_size;
} push_consts;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = imageSize(input_img);  // 128x128
    
    if (coord.x >= size.x || coord.y >= size.y) {
        return;
    }
    
    const int radius = push_consts.kernel_size / 2;
    float acc = 0.0;
    
    // Vertical pass - use 1D kernel
    for (int ky = 0; ky < push_consts.kernel_size; ++ky) {
        int sample_y = coord.y - radius + ky;
        
        // Circular boundary on 128x128 padded image (matching FFT circular convolution)
        sample_y = (sample_y % size.y + size.y) % size.y;
        
        float val = imageLoad(input_img, ivec2(coord.x, sample_y)).r;
        float coeff = kernel[ky];
        acc += val * coeff;
    }
    
    imageStore(output_img, coord, vec4(acc));
}