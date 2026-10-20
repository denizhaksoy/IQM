
#version 450
#pragma shader_stage(compute)

// HDR-VDP-3 FFT Local Adaptation
// Complex FFT buffer -> crop -> exp -> output image

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, std430)
readonly buffer FFTOutput {
    vec2 data[];
} fft_output;

layout(set = 0, binding = 1, r32f)
uniform writeonly image2D output_img;

layout(push_constant) uniform PushConstants {
    uint fft_width;
    uint fft_height;
    uint mode;
    uint column_first;
} params;

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    ivec2 output_size = imageSize(output_img);

    if (x >= uint(output_size.x) ||
        y >= uint(output_size.y)) {
        return;
    }

    // Top-left crop of the inverse FFT result.
    uint index = y * params.fft_width + x;
    if (params.column_first != 0) index = x * params.fft_height + y;

    float reconstructed = fft_output.data[index].x;

    float result = params.mode == 0 ? exp(reconstructed) : clamp(reconstructed, 1e-5, 1e10);

    imageStore(
        output_img,
        ivec2(x, y),
        vec4(result, 0.0, 0.0, 1.0)
    );
}
