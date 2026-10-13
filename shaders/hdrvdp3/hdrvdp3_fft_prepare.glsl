
#version 450
#pragma shader_stage(compute)

// HDR-VDP-3 FFT Local Adaptation
// Log-clamp + replicate padding + real-to-complex preparation

layout(local_size_x = 16, local_size_y = 16) in;

layout(set = 0, binding = 0, r32f)
uniform readonly image2D input_img;

layout(set = 0, binding = 1, std430)
writeonly buffer FFTInput {
    vec2 data[];
} fft_input;

layout(push_constant) uniform PushConstants {
    uint fft_width;
    uint fft_height;
} params;

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;

    if (x >= params.fft_width || y >= params.fft_height) {
        return;
    }

    ivec2 input_size = imageSize(input_img);

    // Replicate the last valid pixel along each dimension.
    ivec2 source_coord = ivec2(
        min(x, uint(input_size.x - 1)),
        min(y, uint(input_size.y - 1))
    );

    float value = imageLoad(input_img, source_coord).r;

    // Match the standalone FFT reference.
    float log_value = log(max(value, 1e-6));

    uint index = y * params.fft_width + x;

    // VkFFT complex float32 layout: real, imaginary.
    fft_input.data[index] = vec2(log_value, 0.0);
}
