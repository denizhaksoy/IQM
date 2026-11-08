# Full Vulkan completion target - 2026-11-07

User explicitly requires the metric processing pipeline on GPU. The current hybrid
quality implementation is not a completed full-GPU implementation. Numerical and
algorithmic reference parity continues to take priority over performance.

| Stage | Current independent quality path | Remaining work |
|---|---|---|
| RGB16 normalization, PQ-domain resize, inversePQ/display | CPU | Exact GPU arithmetic/transcendentals and resize |
| Native RGB and LMSR projection | GPU backend available | Retain buffers on device |
| L+M adaptation input | Optional GPU backend,16/16 integration cases exact | Device-resident buffers |
| MTF filter construction and optical convolution | CPU/FFTW; optionalGPU padding16/16integration exact | GPU filter, exact FFT and scaling |
| Local adaptation log/Gaussian/exp | CPU/FFTW | GPU math/filter/FFT and crop |
| Spectral/model/JND LUT preparation | CPU | GPU construction with preserved operation order |
| JND response/interpolation | CPU | GPU implementation and stage parity |
| Steerable pyramid | CPU in current quality path | GPU band construction/filtering/layout |
| Comparison/mutual/mask resize | GPU experimental stages | Retain buffers, remove synchronous stage roundtrips |
| Remaining masking/CSF/band errors | CPU | GPU implementation and matching reductions |
| Q/Q_JOD and video mean | CPU | Ordered GPU reductions and mapping |

Frozen reference-evaluation files are not modified during the full archive run.
Develop each port in separate build/shader/export roots, compare intermediates by
exact bits, integrate only after independent PNG-to-score parity. No CPU numerical
fallback may silently appear in a future full-GPU profile. Record dispatches and
CPU numerical stages explicitly; full_gpu remains false until the complete path
has been measured. File I/O and Vulkan command submission still require host
orchestration; video hardware decoding is a separate stage and is not implemented
by the numerical kernels described here.
