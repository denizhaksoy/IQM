# LIVE HDR Single preprocessing checkpoint

2026-11-01. Scope: one Daylight1 frame pair at 3840x2160 and exhaustive
16-bit PQ code checks. This is not video-level or full-dataset parity.

## Implemented

`include/IQM/hdrvdp3/pq_single.h` and `src/hdrvdp3/pq_single.cpp` add independent
CPU Single ST2084/display preprocessing to `IQM-HDRVDP3`. Callers supply ambient
lux, reflectivity and peak luminance explicitly. The API preserves array layout,
rejects nonfinite/out-of-range normalized samples and invalid display parameters,
and performs no resize, YCbCr decoding, luma conversion or implicit precision promotion.
MSVC builds use `/fp:strict`.

`tests/live_hdr_protocol.json` separates paper facts, source-example settings,
implementation defaults, local diagnostic choices and unresolved original settings.
It is a protocol record, not an implemented video sampling runner.

## Measured results

Clean v3.0.7 `pq2lin.m` and Octave image package `im2single` generated the baseline.
The reference script retains the example's distorted bicubic resize and [0,1]
clamp. This pair has equal dimensions; lower-resolution bicubic parity is untested.

| Comparison | Values | Bit mismatches | Maximum ULP |
|---|---:|---:|---:|
| Reference 4K frame, C++ API versus official preprocessing | 24,883,200 | 0 | 0 |
| Distorted 4K frame, C++ API versus official preprocessing | 24,883,200 | 0 | 0 |
| All uint16 codes: normalization | 65,536 | 0 | 0 |
| All uint16 codes: first PQ power | 65,536 | 0 | 0 |
| All uint16 codes: PQ ratio | 65,536 | 0 | 0 |
| All uint16 codes: linear RGB | 65,536 | 0 | 0 |
| All uint16 codes: final display result | 65,536 | 0 | 0 |

Final exhaustive results exercise the public API. Stage diagnostics independently
isolate arithmetic operations. Four invalid normalized inputs are rejected.
Equality is observed on MSVC 19.51 / Windows and Octave 11.3.0; this does not prove
identical libm behavior on every compiler/platform or arbitrary floating inputs.

Original PNG files were decoded earlier using explicitly requested `rgb48be`.
The authors' PNG depth/decoder settings remain unresolved. No new tone mapping,
padding, FFT layout or DC processing is introduced by this elementwise API.

## First full-resolution official score

Unmodified clean official v3.0.7, CPU Octave 11.3.0, FFTW threads=3,
planner=`estimate`; input Single, `rgb-bt.2020`, ppd=62.729130063099447,
options `{'quiet',true,'use_gpu',false}`. Other defaults retained, including age24,
OLED spectral model, hdrvdp MTF and sp0Filters. CPU override is recorded explicitly.

- Pair: `4k_3M_Daylight1` against `4k_ref_Daylight1`, zero-based frame 0.
- Q: `4.075747013092041`, Single hex `40826c85`.
- Q_JOD: `4.9195575714111328`, Single hex `409d6d04`.
- Metric execution: approximately 58.95 seconds on this machine.

These are official-reference scores, **not C++ quality predictions**. The generic
C++ quality implementation remains Double-only. A single frame has no video-level
aggregation or subjective correlation result.

Warnings retained in the reference log: pyrTools factorial shadows a core
function; invalid UTF-8 replacement while loading hdrvdp_ncsf data; usual Octave
exit warning. Process exited 0 and finite Single Q/Q_JOD were exported. The data
warning is not silently suppressed or interpreted as a numerical mismatch.

## Artifacts and commands

- `validation/live_hdr_single_20261101_run01`: reference/test `.f32`, input and
  output hashes in `preprocessing.json`, independent/native and API comparisons,
  standalone build.
- `validation/live_hdr_single_stages_20261101_run01`: exhaustive stages and
  `native_cpp.json` / `api_cpp.json`.
- `validation/live_hdr_frame_score_20261101_run01`: official `score.json` and log.

```powershell
# Each exporter requires a fresh output directory; preserve the existing runs.
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/prepare_live_hdr_reference.m validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7 validation/live_hdr_639290787607997134/frame_0001/reference.png validation/live_hdr_639290787607997134/frame_0001/test.png <fresh-preprocessing-root>
cmake -S tests/live_hdr_preprocessing -B <fresh-build-root> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build-root> --config Release
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/score_live_hdr_reference_frame.m validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7 <preprocessing-root> <fresh-score-root>
```

Main integration check: `cmake --build build-vs2026 --config Release --target IQM-HDRVDP3`
passed. Existing glfw3 configuration warning remains. No unrelated GPU validation
was rerun, no shaders rebuilt, and no commit/push performed.

Next: capture the official Single quality pipeline's intermediate classes/layouts
and implement a general Single execution path without promoting it to Double.
Then compare this original-size frame's Q and Q_JOD exactly before expanding to
all sampled frames, lower-resolution inputs, cohort scoring and correlations.
