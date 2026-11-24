#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <filesystem>
#include <memory>

namespace IQM::HDRVDP3 {
enum class FFTDirection { Forward, Inverse };
enum class FFTDomain { Complex1D, Real2D, Complex2D };
// FFTW can select different arithmetic graphs for the same length depending on
// strides, real/complex context and threading. Exactness is graph-specific.
enum class FFTNumericalGraph { Standalone720, Official4KAxis7680 };
struct ResidentFFTSpec {
    FFTDomain domain=FFTDomain::Complex1D;
    uint32_t length=0, batches=0;
    FFTDirection direction=FFTDirection::Forward;
    FFTNumericalGraph graph=FFTNumericalGraph::Standalone720;
};
// All sizes/offsets are bytes. Caller guarantees allocation binding and STORAGE_BUFFER
// usage. Distinct VkBuffers must not alias underlying memory. Buffers and this job
// must outlive all recorded/submitted command buffers. Queue ownership is caller's.
struct FFTBufferRegion {
    vk::Buffer buffer{};
    vk::DeviceSize offset=0, bytes=0;
};
// Complex samples are adjacent float32 real/imaginary words (8 bytes).
// Strides/base are in complex samples relative to region.offset, not bytes.
struct FFTBufferView {
    FFTBufferRegion region;
    uint32_t firstComplex=0, sampleStride=1, batchStride=0;
};
struct ResidentFFTBuffers {
    FFTBufferView input, output;
    std::array<FFTBufferRegion,3> workspace;
};
struct ResidentFFTShaders {
    std::filesystem::path copy, fft720;
    std::array<std::filesystem::path,4> forward7680;
};
// Immutable descriptor/pipeline bindings. Construction does not inspect tensor
// values, compute twiddles, map memory, allocate buffers or submit GPU work.
// record() performs no CPU metric arithmetic/staging/readback/submission/wait.
// Unnormalized finite-f32 C2C only: pinned standalone720 forward/inverse and
// official 4K real-plan CT20 axis7680 forward (NOT standalone FFTW7680).
// No implicit fallback. Values are not scanned on CPU; domain checks belong on GPU.
class ResidentFFT {
public:
    static bool supported(const ResidentFFTSpec&) noexcept;
    ResidentFFT(const vk::raii::Device&, const vk::raii::PhysicalDevice&,
                ResidentFFTSpec, const ResidentFFTBuffers&, const ResidentFFTShaders&);
    ~ResidentFFT();
    ResidentFFT(ResidentFFT&&) noexcept;
    ResidentFFT& operator=(ResidentFFT&&) noexcept;
    ResidentFFT(const ResidentFFT&)=delete;
    ResidentFFT& operator=(const ResidentFFT&)=delete;
    // cmd must be recording on the owning device's compute-capable queue family.
    // Adds same-queue memory dependencies for preceding producers, internal
    // scratch reuse and subsequent consumers. No queue-family transfer is done.
    void record(vk::CommandBuffer cmd) const;
    uint32_t dispatchCount() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
