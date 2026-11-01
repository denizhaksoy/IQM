# Expanded RGB96 content validation

2026-10-27 checkpoint: `validation/rgb_content_active_path.txt`.

Four new synthetic pairs and one HDR-derived color pair extend the previous
single RGB ramp pair. Production code/shaders/binaries were unchanged; frozen
accepted API/CLI binaries and the source-generated RGB runtime were used.
Native HDR-VDP-3 v3.0.7, Octave11.3, FFTW3 threads3/estimate/cleared single
wisdom, Float32 rgb-native96×96,30ppd,age24,default quality/sp0 remain fixed.
Symmetric MTF and replicate adaptation padding, native DC handling and
column/row boundary layout conversions are preserved.

## New cases

| Pair | Description |
| --- | --- |
| identity | Existing spatially varying RGB reference compared to itself |
| constant | Constant [.125,10,1000] RGB; test green multiplied by1.01 |
| primaries | Pure R/G/B regions, local green reduction and blue zero-to-positive patch |
| impulse | Low background, isolated HDR impulses up to10000 and color-local changes |
| real | Actual RGBE photograph crop, three channels retained, local red/blue changes |

The official reference returns Q/Q_JOD10 for identity and the uniform constant
change; these behaviors are reproduced rather than replaced with expected
perceptual judgments. Primaries and impulse scores differ across display
spectra; all exact native values are retained in per-case metadata.json.

Production API: **465 checks /4,745,670 Float32 words bitwise exact** across
five pairs × led-lcd-srgb/led-lcd-wcg/oled =15 configurations. This includes
original-to-P intermediate comparisons, masked differences in five bands,
band/accumulated errors and Q/Q_JOD. Counts repeat image content across
spectra; they are not fifteen independent content pairs. Previous cohorts
are not included in this number.

CLI synthetic cases: four actual PF pairs with10 exports each,
884,736 Float32 export words +32 quality scalars bitwise exact. The additional
real PF CLI case passes221,184 export words +8 quality scalars bitwise in
`cli_real/`. Total: **1,105,920 export words +40 quality scalars**. CLI cohorts
overlap the API. Vulkan validation enabled; no errors were reported in the
five API logs or five numerical CLI runs.

Nine additional public CLI contracts reject unsupported geometry, encoding,
display,ppd,age60/60.001,luminance base runtime,missing spectral inputs and
display-on-luminance. Expected errors and absence of export directories are
checked, preventing silent fallback. Production code was not changed to pass
this cohort; all cases use the frozen previously accepted binaries.

## HDR-derived color provenance

Local cached `memorial.hdr`, Paul Debevec/Jitendra Malik Memorial Church HDR
radiance map, source metadata copied from the earlier local grayscale study.
SHA256:18376B9219927B363127817900B1C170A0F752F52A63F5949655B4150E35061D.
The source photograph remains excluded from Git and is not copied into this
checkpoint. Original file and older grayscale references are unchanged.

`prepare_real_rgb.cpp` uses stbi_loadf RGBE decoding and keeps three channels.
It assigns the global maximum source channel1000 virtual display units,
performs one Float32 cast and extracts a96×96 center crop at x208/y336 from
512×768. Scale3.6496350364963503; crop min.00089102418860420585,
max1.5967153310775757. No averaging, floor, gamma, resizing or tone mapping.
Reference/test binary inputs are hashed. Test red is scaled by.85 locally;
test blue has a separate local.1 addition.

Source RGB is interpreted as native display primaries for this numerical
stimulus. This does not recover absolute capture luminance, source primaries
or a colorimetric conversion. In particular it does not validate an sRGB EOTF.

Real native Q/Q_JOD:

| Display | Q | Q_JOD |
| --- | ---: | ---: |
| led-lcd-srgb | 9.874041557312012 | 9.963422775268555 |
| led-lcd-wcg | 9.856755256652832 | 9.956871032714844 |
| oled | 9.773855209350586 | 9.922581672668457 |

## Reproduction

`generate_rgb_projection_reference.m <fresh> --jnd --pattern
identity|constant|primaries|impulse` exports fresh native traces and quality
goldens. Default behavior is preserved. `--pattern external --rgb-input <dir>`
reads exact column-major planar `reference_rgb.f32` and `test_rgb.f32` without
transforms. `prepare-real-rgb` is a separate target in `tests/hdrvdp3_hdr_inputs`;
its arguments are source HDR and fresh output directory.

Run the existing `rgb-quality-api-test` with native, RGB runtime, original
spectral data and fresh output arguments for each case. The additional
`validate_rgb_content_cli.cjs` uses four synthetic pairs; `--real-only` selects
the real pair. Each CLI command is saved as command.json. All new exports are
fresh; no old reference is overwritten.

The updated generator's default `--jnd` run was also exported to a fresh
directory:102 original numerical fixtures are byte-identical to the preceding
accepted ramp cohort. Production source and shaders were not changed in this
content-expansion checkpoint. Existing dirty vendor/shader changes remain.

This broadens content coverage at one geometry/viewing profile. Other sizes,
viewing conditions, encodings, prediction modes and GPU residency remain open.
