# Expanded linear BT.709 content validation

2026-10-28 checkpoint: `validation/rgb709_content_active_path.txt`.

This run broadens content coverage without changing production code, shaders
or runtime pins. It uses the frozen main IQM executable from the preceding709
CLI checkpoint and its source-generated23-payload709 runtime.

## Inputs and reference

Five additional96×96 pairs: identity, constant HDR colors, pure-primary regions,
HDR impulses and the previously prepared Memorial HDR-derived three-channel
color crop. Exact reference/test Float32 inputs are reused as linear709 values;
all30 input buffers across five pairs/three displays match their source bytes.
Their older native-RGB scores are not reused. Fresh official3.0.7/Octave11.3
709 runs compute new matrices, gamut-clamped pixels, P/adaptation, band errors
and final scores. Stock/instrumented score bits and accumulated errors agree.

The original default709 fixture remains unchanged:42 numerical files regenerated
in a fresh directory match the preceding ramp cohort byte for byte.

Each pair is tested with led-lcd-srgb,led-lcd-wcg,oled:15 spectral/content
configurations, not fifteen independent image pairs. Profile stays Float32
linear709,96×96,30ppd,age24,default quality/sp0; FFTW3 threads3/estimate/cleared
single wisdom, symmetric MTF/replicate adaptation padding/native DC handling.
The HDR crop retains RGB channels and the earlier explicit virtual-display
scaling; interpreting it as709 is a numerical stimulus assignment, not recovered
camera primaries or absolute capture luminance. Source credit/hash and mapping
remain in `RGB_CONTENT_VALIDATION.md` and the copied provenance.

## CLI comparisons

For every configuration, the normal main application reads actual color PF files
and independently loads spectral tables/calculates the conversion. The validator
compares original709 values, raw conversion, clamped native RGB, P/adaptation,
all five band errors, total error,Q and Q_JOD bit for bit with fresh709 goldens.
All five cohorts pass: **3,041,280 Float32 export words +120 quality values
bitwise exact**. Each cohort has608,256 words and24 scalars. All15 Vulkan
validation logs have no reported error markers. These are five new content
pairs; cohorts overlap earlier API stages and are not fifteen independent
content pairs. `results.json` records each exact Q/Q_JOD and configuration.

## Next encoding characterization

While the CLI batches run, optional `rgb-bt.2020` and `xyz` targets were added
to the native XYZ-matrix oracle and diagnostic solve inspector. The existing
flags77 solve order matches27 Double coefficients for2020 and27 forXYZ, across
the same three display spectra. These are **diagnostic matrix results only**;
production/CLI support for2020 andXYZ is not yet present.

Repeated native oracle exports have identical bytes and explicit Octave exit0.
An initial shell aggregate reported nonzero despite completed oracle PASS files;
the fresh repeat captured each child exit code. Six default709 XYZ sample/matrix
files are unchanged after adding optional targets. This does not broaden the
validated BLAS environment or support arbitrary ill-conditioned primary sets.

## Reproduction

`generate_rgb709_reference.m <fresh> --rgb-input <dir>` reads exact column-major
planar `reference_rgb.f32` / `test_rgb.f32`, preserving its old no-option default.
Run `validate_rgb709_cli.cjs <main-exe> <native709> <runtime709> <spectral-data>
<fresh-output>` for each pair; every actual CLI command is saved as command.json.

Next generalize the independently validated matrix helper for2020/XYZ, verify
GPU conversion and full scores, and expose explicit runtime/CLI contracts only
after passing. sRGB-display needs its own Double precision work. Other sizes,
viewing configurations, prediction modes, devices and residency remain open.
