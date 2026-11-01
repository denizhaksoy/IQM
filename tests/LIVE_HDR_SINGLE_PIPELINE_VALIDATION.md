# Original-size LIVE HDR Single CPU pipeline

2026-11-01. Scope: Daylight1 `4k_3M` versus `4k_ref`, 3840x2160,
local RGB48 PNG decoding. This is a CPU C++ correctness checkpoint, not
full GPU support, full-video evaluation, or published challenge reproduction.

## Implemented

Public C++ stages now cover Single native optical filtering, local adaptation,
RGB JND response, general sp0 pyramid, and quality pooling. New files:

- `fft_single.h`: explicit convolution and forward-FFT provider contract.
- `optical_single.h/.cpp`: hdrvdp MTF, symmetric post-padding to twice the
  dimensions, optical clamp; replicate-padding FFT Gaussian log adaptation.
- `jnd.h`, `jnd_single.cpp`: general RGB Single pupil/JND response, retaining
  Double JND tables and Single lookup/evaluation arithmetic.
- `sp0_single.h/.cpp`: variable-size Single pyramid and base DC removal.
- `general_quality_single.h/.cpp`: validated prepared-stage quality inputs,
  mutual/neighbor masking, CSF, base-frequency selection and Q/Q_JOD.

`tests/live_hdr_preprocessing/frame.cpp` runs the entire PNG-to-score chain in
one independent C++ process using these library stages. It reads no reference
intermediates and invokes no Octave scorer. The diagnostic FFT provider uses
the same installed standard FFTW Float library as the reference, with three
threads and estimate planning. It is an explicit CPU dependency, not Vulkan.
The existing main IQM CLI/runtime profiles have not been replaced by this tool.

Initial validated settings: Single BT2020, OLED spectral model, age24,
ppd62.729130063099447, quality/sp0, ambient200 lux, reflectivity0.005,
peak2000 cd/m² before ambient. Equal-resolution RGB16 PNG pair required by
the complete runner. Lower-resolution PQ bicubic input processing is not yet
connected/validated in that runner. General stage ppd domain26..256 avoids the
unimplemented spatial-Gaussian adaptation branch.

## Numerical checks on frame0

All comparisons use identical input hashes and parameters. Official arrays are
column-major; C++ arrays are row-major with explicit index translation. MTF and
Gaussian coefficient generation remain Double, while FFTs, products and output
images are Single. Both dimensions are padded by the same amount and cropped
at the same origin. Base DC is removed only after normalized pyramid creation.

| Independent stage | Values across both images | Bit mismatches |
|---|---:|---:|
| Optical RGB | 49,766,400 | 0 |
| LMSR from independently computed optical RGB | 66,355,200 | 0 |
| Local adaptation | 16,588,800 | 0 |
| Achromatic JND response | 16,588,800 | 0 |
| Six normalized pyramid bands, including base DC removal | 38,685,600 | 0 |

Total: 187,984,800 compared stage values with zero bit differences.
This count excludes previously validated PQ/native color stages. Full masking
and CSF arrays were not exhaustively compared; final Q/Q_JOD equality is a
separate measured check and must not be described as proof that every remaining
intermediate is identical.

Frame0 independent PNG-to-score run:

| Output | Official / C++ Single text | Official / C++ hex |
|---|---|---|
| Q | 4.075747013092041 | 40826c85 |
| Q_JOD | 4.9195575714111328 | 409d6d04 |

The first complete C++ run took approximately99.48 seconds on this machine.
Reference metric execution took approximately59 seconds; diagnostic captures
retain the same stock output bits. Timings are observations, not performance
claims or a GPU benchmark.

Frozen-package complete runs also passed on both zero-based indices:

| Frame | Q (official = C++) | Q_JOD (official = C++) | Hex Q / Q_JOD |
|---|---|---|---|
| 0, frozen repeat | 4.075747013092041 | 4.9195575714111328 | 40826c85 / 409d6d04 |
| 30 | 7.4749040603637695 | 8.2962665557861328 | 40ef326a / 4104bd82 |

Both exited0 with `EXACT Q AND Q_JOD`. Result directories within the run02 demo:
`results_frame0000_1793538185136` and `results_frame0030_1793538174683`.
These are two frames, not the complete 16-frame sample or a video average.

## Differences discovered and corrected

1. A sequential Float base mean produced differences in all32,400 base values.
   Local Octave11.3.0 `mean(Single)` uses a Double accumulator, divides in Double,
   then casts to Single. Correcting that behavior made all six bands exact.
   Pupil `geomean` uses a different accumulation path; it was already exact and
   was not changed to match the base mean.
2. The first quality integration used the psychometric response for pooling.
   Official quality uses raw `D` for `D_p`, while the psychometric result goes
   into reconstructed visibility bands. Correcting this yielded exact Q/Q_JOD.

Failed runs are preserved in `live_hdr_pyramid_20261101_run01` and
`live_hdr_quality_20261101_run01`. They are not acceptance passes.

## Artifacts

- `validation/live_hdr_capture_20261101_run03`: full optical/LMSR/adaptation/JND
  reference capture; instrumented output checked against unmodified stock bits.
- `validation/live_hdr_capture_20261101_run04`: additionally all six reference
  bands per side; same stock-score transparency check.
- `validation/live_hdr_fft_20261101_run01`: independent optical/LMSR/adaptation
  arrays and bit comparisons, both sides.
- `validation/live_hdr_jnd_20261101_run01`: independent JND responses/comparisons.
- `validation/live_hdr_pyramid_20261101_run02`: corrected independent bands,
  zero mismatches on both sides.
- `validation/live_hdr_quality_20261101_run02`: exact score from independently
  persisted C++ stages.
- `validation/live_hdr_frame_cpp_20261101_run01`: first complete C++ PNG-to-score
  run with provenance and exact comparison.
- `validation/live_hdr_frame30_20261101_run01`: explicitly decoded zero-based
  frame30 pair, stream metadata, exact FFmpeg commands/version and hashes.
- `validation/live_hdr_frame30_score_20261101_run01`: second stock baseline;
  Q7.4749040603637695 / Q_JOD8.2962665557861328, hex40ef326a / 4104bd82.

## Reproducible local demo

`validation/live_hdr_single_demo_20261101_run02` contains a frozen executable,
both PNG pairs, stock score files, required model data, dataset notice,
source snapshot and a SHA256 manifest. The Float FFTW DLL remains an explicit
installed dependency; expected SHA256 is recorded and checked. The wrapper
verifies PNG identity, baseline precision/backend/viewing settings and final
score hex; it never falls back to official results.

From the IQM repository in PowerShell:

```powershell
node validation/live_hdr_single_demo_20261101_run02/run_demo.cjs frame0000
node validation/live_hdr_single_demo_20261101_run02/run_demo.cjs frame0030
```

Each call creates a fresh result directory. The Node entry avoids changing
PowerShell execution policy. A `.ps1` entry is included but direct execution was
blocked by the current system policy; no policy was changed.

The source snapshot can be configured with CMake using its
`source/tests/live_hdr_preprocessing` directory. It includes the local stb header
and provider, but still requires a suitable compiler and installed FFTW runtime.
The original clean official distribution and all older numerical artifacts are
preserved. No shader compilation, reset, cleanup, commit or push was performed.

Main `IQM-HDRVDP3` integration build passed. The source snapshot was separately
configured and its `live-frame-single` target successfully built in
`validation/live_hdr_demo_build_20261101_run01`. Existing main-build glfw3 and
unrelated compiler warnings remain. The protected corrdn shader hash is
unchanged and VkFFT remains dirty.

## Remaining completion requirements

- Extend to all selected frames with explicit timing/alignment validation and
  mean frame Q_JOD aggregation; two frames cannot stand for a full video.
- Validate lower-resolution distorted inputs and broaden contents/encodings.
- Establish challenge cohort/reference-inclusion and correlation policy; report
  unresolved original decoder precision/frame phase explicitly.
- Full dataset comparison and subjective correlations remain unperformed.
- General Vulkan execution and main CLI integration remain separate work.
