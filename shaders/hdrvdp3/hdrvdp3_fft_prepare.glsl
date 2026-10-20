
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
    uint mode; // 0: log adaptation/replicate; 1: optical MTF/symmetric
    uint column_first;
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
    if (params.mode == 1) {
        // padarray(..., 'symmetric', 'post'), including the edge pixel.
        source_coord.x = x < uint(input_size.x) ? int(x) : 2 * input_size.x - 1 - int(x);
        source_coord.y = y < uint(input_size.y) ? int(y) : 2 * input_size.y - 1 - int(y);
    }

    float value = imageLoad(input_img, source_coord).r;

    // Match the standalone FFT reference.
    float log_value = params.mode == 0 ? log(max(value, 1e-6)) : value;

    uint index = y * params.fft_width + x;
    if (params.column_first != 0) index = x * params.fft_height + y;

    // VkFFT complex float32 layout: real, imaginary.
    fft_input.data[index] = vec2(log_value, 0.0);
}
