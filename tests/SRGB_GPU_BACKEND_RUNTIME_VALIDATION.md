# Production Double96 GPU backend and runtime

Validated2026-10-29. Artifacts:
`validation/srgb_gpu_backend_completed_path.txt`.

`SRGBQuality96GPUDouble` is now part of IQM-HDRVDP3. It borrows a caller-owned
physical/logical device and compute queue,owns reusable seven shader modules/
pipelines,descriptor sets,coherent buffers,command pool/buffer and fence,and
provides callbacks for `evaluate_srgb_quality96_double`. There are no Bin or
test dependencies and no static global Vulkan instance in the backend. Normal
convolution returns only the GPU crop;full inverse/spectrum traces are opt-in.
Calls on one context are serialized;other uses of the supplied queue must be
synchronized by the caller. Device/queue must outlive the context and borrowed
callbacks must not outlive it.

The constructor requires an explicit declaration that logical shaderFloat64
was enabled,verifies physical Float64/RTE64/SignedZeroInfNan64 support and a
compute queue family,and rejects missing SPIR-V headers. The caller supplies
trusted compiled shaders;these header checks are not full SPIR-V validation.
The supplied logical device must correspond to the supplied physical device.
Every calculation input is checked for shape and finite values before upload.
Host-visible coherent memory is required;there is no implicit CPU fallback.

Nine original-image configurations(2distorted pairs+identity,3spectra) give
13,937,256 bitwise-exact Double comparisons through the real production
backend,including pad/filter/FFT/inverse/crop/baseFFT/per-band/scalar checks.
All18Q/Q_JOD scores match officialv3.0.7. These reuse prior inputs and overlap
prior validation;they are not new image coverage. Eighteen API/backend
rejection checks pass,covering invalid images/callbacks,malformed/nonfinite
backend results,disabled Float64,invalid queue family/shader header,and
malformed/nonfinite convolution/baseFFT inputs. Vulkan logs are clean.

`load_srgb_quality96_runtime_double` loads a separate `double96.runtime`
profile:96x96/30ppd/age24/quality/defaultsp0. It requires exactly7shaders and
6raw model files,confines canonical paths to the package,and verifies all13
SHA256 digests before loading the selected display. Digests provide integrity,
not signed-package authentication. The loader and legacy runtime functions
are separate;legacy runtime/source helpers were not modified.

`build_srgb96_double_runtime.ps1` generates a fresh package from pinned FFTW
source graphs,source-generated Double twiddles and raw spectral files. It
compiles all7shaders individually,injects the accepted Float64 controls,
runs spirv-val and retains COPYING. All7fresh binaries are byte/hash-identical
to the accepted prior shaders. No shaders_out deletion or modification.

All3display loads pass;14malformed-package/display cases reject,including
wrong profile/version,missing/duplicate/unknown entries,digest failures,
traversal/absolute paths,missing payload,bad SPIR-V magic and corrupted model.
The package-based raw-f32 example uses the actual production loader+GPU backend
and gives exact OLEDstress error/Q/Q_JOD without a native capture directory.
Both the standalone targets and actual IQM-HDRVDP3 Release library build pass.
The optional existing glfw3 configure warning does not prevent the build.

Reproduce:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh-package-build> -RawData <raw-data>
# tests/hdrvdp3_double_gpu CMake targets:
# srgb-quality96-gpu-backend-test and srgb-quality96-runtime-run
srgb-quality96-runtime-run <fresh-package-build>/runtime oled <reference.f32> <test.f32> <fresh-output>
node tests/hdrvdp3_evaluation/validate_double96_runtime.cjs <validation-root>
node tests/hdrvdp3_evaluation/audit_srgb_gpu_backend.cjs <validation-root>
```

The rejection generator requires a fresh runtime_rejections directory.
Numerical configuration remains fixed96sq/30ppd/age24/defaultsp0,mixed Single
RGB preparation then Double,192sq symmetric optical/replicate adaptation
postpadding,column-major complex layout,DC unchanged,inverse36864 with tested
corrected division. NVIDIA RTX A500;denormPreserve64 unavailable. Remaining
non-FFT visual/quality operations are CPU. Public main IQM PFM/srgb-display
routing is the next integration step;it is not enabled by this checkpoint.
Broader geometry/viewing/prediction/device and fullGPU coverage remain open.
