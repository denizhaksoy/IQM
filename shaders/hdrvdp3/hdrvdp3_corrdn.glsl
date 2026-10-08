/*
 * Image Quality Metrics - HDR-VDP-3 Spatial Correlation
 * Implementation based on matlabPyrTools/corrDn.m and rconv2.m
 */

#version 450
#pragma shader_stage(compute)

#include "hdrvdp3_shared.glsl"

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f) uniform readonly image2D input_img;
layout(set = 0, binding = 1, r32f) uniform writeonly image2D output_img;
layout(set = 0, binding = 2, std430) readonly buffer FilterBuffer {
    float filter_coeffs[MAX_FILTER_SIZE];
};

layout(push_constant) uniform PushConstants {
    int filter_size;     // Filter dimension (7, 9, or 13)
    int step_x;          // Downsampling step in X (1 or 2)
    int step_y;          // Downsampling step in Y (1 or 2)
    int start_x;         // Start offset in X (0-indexed)
    int start_y;         // Start offset in Y (0-indexed)
} push_consts;

void main() {
    uint out_x = gl_WorkGroupID.x * gl_WorkGroupSize.x + gl_LocalInvocationID.x;
    uint out_y = gl_WorkGroupID.y * gl_WorkGroupSize.y + gl_LocalInvocationID.y;
    ivec2 out_size = imageSize(output_img);
    
    if (out_x >= out_size.x || out_y >= out_size.y) {
        return;
    }

    ivec2 in_size = imageSize(input_img);
    int filter_radius = push_consts.filter_size / 2;
    
    // Compute input coordinates for this output pixel
    // Output pixel (out_x, out_y) corresponds to input at:
    // in_x = start_x + out_x * step_x
    // in_y = start_y + out_y * step_y
    int center_x = push_consts.start_x + int(out_x) * push_consts.step_x;
    int center_y = push_consts.start_y + int(out_y) * push_consts.step_y;
    
    float acc = 0.0;
    
    // Correlation: filter is already flipped (rotated 180) compared to convolution
    // In MATLAB: filt = filt(size(filt,1):-1:1, size(filt,2):-1:1)
    // The filter coefficients in filter_coeffs are stored in row-major order
    // (fy * filter_size + fx) for correlation kernel at (fy, fx)
    
    for (int fy = 0; fy < push_consts.filter_size; ++fy) {
        for (int fx = 0; fx < push_consts.filter_size; ++fx) {
            int sample_x = center_x - filter_radius + fx;
            int sample_y = center_y - filter_radius + fy;
            
            // Reflect1 boundary handling
            float max_x = float(in_size.x - 1);
            float max_y = float(in_size.y - 1);
            
            float rx = float(sample_x);
            float ry = float(sample_y);
            
            if (rx < 0.0) rx = -rx;
            if (ry < 0.0) ry = -ry;
            if (rx > max_x) rx = 2.0 * max_x - rx;
            if (ry > max_y) ry = 2.0 * max_y - ry;
            
            // Handle multiple reflections (for large filters near boundaries)
            while (rx < 0.0 || rx > max_x || ry < 0.0 || ry > max_y) {
                if (rx < 0.0) rx = -rx;
                if (rx > max_x) rx = 2.0 * max_x - rx;
                if (ry < 0.0) ry = -ry;
                if (ry > max_y) ry = 2.0 * max_y - ry;
            }
            
            int idx_x = int(rx);
            int idx_y = int(ry);
            
            // Clamp to valid range (safety)
            idx_x = clamp(idx_x, 0, in_size.x - 1);
            idx_y = clamp(idx_y, 0, in_size.y - 1);
            
            float val = imageLoad(input_img, ivec2(idx_x, idx_y)).r;
            float coeff = filter_coeffs[fy * push_consts.filter_size + fx];
            acc += val * coeff;
        }
    }
    
    imageStore(output_img, ivec2(int(out_x), int(out_y)), vec4(acc));
}