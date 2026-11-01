# Fixed-profile sRGB Double quality API

Validated2026-10-29. Artifact pointer:
`validation/srgb_quality96_api_completed_path.txt`.

`evaluate_srgb_quality96_double` is now part of the IQM-HDRVDP3 library.
It accepts original planar column-major encoded Single RGB reference/test,
raw XYZ/display spectral samples, and explicit convolution/baseFFT callbacks.
The implementation has no filesystem or native-capture dependency. The
profile is explicitly96x96,30ppd,age24,quality/defaultsp0. It returns Double
error,Q,Q_JOD and five band errors. It does not silently select an FFT backend.

Calculation preserves the previously accepted mixed-precision display
preparation,Double symmetric/replicate padding and filters,optical clamp,
spectral projection,log adaptation,Gaussian adaptation,JND,normalized/DC
pyramid,mutual/neighbor masking,CSF,comparison masks and quality pooling.
Backend results are checked for shape/finiteness and base-spectrum DC zero.
Images must have27648 finite encoded values in[0,1];both images and missing
callbacks are rejected before any backend execution. XYZ and spectral samples
are required to describe the same display, as documented in the API contract.

The test supplies the accepted NVIDIA Double GPU FFT backend. Native records
are read only inside test comparisons; the library receives no native path.
Nine configurations(two distorted pairs plus identity,three display spectra)
pass with13,937,256 compared Double words bitwise exact. These comparisons
cover independently generated padding/filter,actual GPU forward/inverse/crop,
baseFFT,quality scores and per-band errors. This scope overlaps previous tests;
it is not additional image coverage. All18Q/Q_JOD values match the official
v3.0.7 native records. Nine negative cases reject invalid images,missing
callbacks,wrong convolution shape and nonfinite convolution results.

`srgb-quality96-run` is a minimal raw-f32 API usage example. A separate run
uses copied original images,raw model data and seven shader binaries,with no
native comparison directory argument. Its error,Q,Q_JOD equal the official
OLED stress result. This example is not the public IQM CLI and does not read
PFM. The main srgb-display runtime/CLI route remains unfinished.

Builds passed:
- tests/hdrvdp3_double_gpu target srgb-quality96-api-test and srgb-quality96-run.
- Actual production target IQM-HDRVDP3 in build-vs2026,Release;new source uses
  /fp:strict (or -ffp-contract=off on non-MSVC).

All accepted Vulkan logs are clean. Backend uses NVIDIA RTX A500 Float64,
RTE64/SignedZeroInfNanPreserve64 and previously accepted corrected Double
normalization;denorm64 preservation is unavailable. Remaining non-FFT visual
and quality computations run on CPU. This is not fullGPU,all-size/viewing or
cross-device parity. Existing production routes and shaders_out are preserved.

```text
srgb-quality96-api-test <cpu-reference-root> <encoded-root> <chain-root> <base-root> <raw-model-data> <fresh-output>
srgb-quality96-run <raw-model-data> <display> <reference.f32> <test.f32> <seven-shader-directory> <fresh-output>
node tests/hdrvdp3_evaluation/audit_srgb_quality96_api.cjs <validation-root>
```

The example requires files forward-first.spv,forward-second.spv,multiply.spv,
inverse-first.spv,inverse-second.spv,crop.spv,base12.spv. Frozen working example
inputs/model/shaders and exact arguments are recorded in standalone_command.json.
No native FFTW DLL is used. The GPU adapter remains a diagnostic helper;
next extract a reusable production GPU backend and authenticated/fixed-profile
runtime package,then wire public CLI with PFM input and parity/negative tests.
