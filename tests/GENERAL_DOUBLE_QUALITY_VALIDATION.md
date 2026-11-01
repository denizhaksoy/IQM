# Original-size generic Double quality path

Checkpoint 2026-10-31:
`validation/general_quality_639290762115433659`.

An independent C++ Double CPU orchestration path now removes the fixed 96x96
geometry assumption for its explicitly supported profile. It computes image
preparation, optical/adaptation processing, JND response, variable sp0 pyramid,
masking, CSF, base-frequency selection and Q/Q_JOD. FFTs are supplied by an
explicit callback backend; the diagnostic uses FFTW, not an Octave scorer.

The public header is `general_quality_double.h`; the implementation is compiled
into IQM-HDRVDP3. The existing main IQM CLI still uses its established runtime
profiles. The generic path is currently exercised with the separate C++ runner
and manifest adapter, not silently substituted for the GPU CLI.

## Clean reference provenance

Official SourceForge `hdrvdp-3.0.7.zip` was downloaded to this fresh checkpoint
and extracted without changing `C:/HDR-VDP-3_Ref`. Clean `hdrvdp3.m` SHA256:
`0F9F267F68B042C167EF03F41DAAF07388AE192AB30F81B243766863F9A5402A`.

The older local installation differs in two `.m` files through additional
diagnostic printing/inspection. Diffs are preserved. Historical references
remain unchanged; this new checkpoint uses the clean distribution for stock
score calculations. A separate instrumented copy exports intermediate values;
every captured case's Q/Q_JOD is checked bitwise against clean stock execution.
Do not describe the older local source directory as byte-identical to the clean
distribution. This source check does not establish every historical result anew.

## Exact measurements

| Size | ppd | Age | Encoding | Compared Double words |
|---|---:|---:|---|---:|
| 80x64 | 30 | 24 | native RGB | 80,167 |
| 97x65 | 30 | 24 | native RGB | 98,900 |
| 96x96 | 30 | 24 | sRGB display | 144,295 |
| 256x192 | 60 | 45 | native RGB | 769,928 |
| 1024x683 | 30 | 24 | sRGB display | 10,950,663 |
| 96x80 | 45 | 35 | BT.709 linear | 120,247 |
| 96x80 | 45 | 35 | BT.2020 linear | 120,247 |
| 65x97 | 30 | 60 | XYZ | 98,900 |

**12,383,347 comparisons, zero bit differences**, including transformed RGB,
adaptation, P response, every normalized/DC-corrected pyramid band, mean log
adaptation, band errors and final Q/Q_JOD. These are engineering configurations,
not eight independent photographs or UPIQ samples. The original 1024x683 official
`wavy_facade.png` example is preserved at full resolution; a local green-channel
distortion provides a nonidentity score. Other patterns are synthetic.

An additional original-file PNG pair passes independently through the C++ PNG
decoder, full C++ computation and the unmodified stock reference runner: Q/Q_JOD
match exactly, with matching file hashes, precision and viewing metadata. C++
normalization of all 2,098,176 samples in the original 16-bit reference PNG also
matches Octave imread/normalization bytewise. The file-pair test overlaps the same
photograph and must not be added as a new source photograph.

Thirteen invalid API/input/viewing configurations reject with checked error
categories. The rebuilt main IQM application retains three byte-identical v10
numeric/input exports for the OLED stress regression. No old shader was rebuilt
or replaced. Existing v10/demo checkpoints remain preserved.

## Numerical issue resolved

The initial rectangle implementation matched optical, adaptation and JND values
but differed in almost every pyramid value. Octave's convolution sorts axes by
decreasing length; on a wider-than-tall array this changes tap accumulation order
and the contiguous dimension used by the BLAS FMA blocks. The C++ path now
preserves this order for reflected pyramid correlation and mutual masking.
No tolerance, input-specific correction or reference intermediate is injected.
Initial failing artifacts are retained.

## Reproduction

```powershell
cmake -S tests/hdrvdp3_general_double -B validation/my_general_build -G "Visual Studio 18 2026" -A x64
cmake --build validation/my_general_build --config Release --target general-double-quality prepare-general-double-image general-double-contract
```

`export_general_double_reference.m` accepts `<clean-reference> <fresh-output>
quick|extended|linear`. The C++ runner accepts `<case-directory> <model-directory>
<fftw-dll> <fresh-output>` and computes results before loading any expected
intermediates for comparison. `--scores-only` does not read reference scores or
intermediates at all. The FFT provider uses three threads, estimate planning,
explicit unaligned/preserve flags and cleared wisdom, matching the tested setup.

`run_general_implementation.cjs` accepts the explicit reference-manifest schema
and invokes only independent C++ executables. PNG requires a declared unit-encoded
sRGB policy; linear EXR requires a verified decoded cache and absolute-linear
policy. Unsupported Single precision is rejected, not silently promoted. Missing
cases or mismatched protocols prevent acceptance in `compare_predictions.cjs`.

```powershell
node tests/upiq/audit_general_quality.cjs <checkpoint-root>
```

## Limits and next acceptance target

Initial API domain: axes >=16, ppd 26..256 (FFT local-adaptation branch), age
0..90, Double input, default quality/sp0, symmetric optical surround. Native RGB,
sRGB-display, linear BT.709/BT.2020 and XYZ are implemented, but the table above
defines measured coverage; it is not an exhaustive all-argument guarantee.
The general path is CPU-based. FFTW shares a numerical library with the reference
environment; C++ orchestration does not invoke or proxy the Octave metric.

**UPIQ archive access still returns HTTP 500. Zero UPIQ images or predictions
have been validated.** The actual corrected dataset metadata adapter, possible
Single-input pathway, smaller ppd/spatial adaptation branch, general main-CLI
runtime and full UPIQ acceptance remain open. Generic engineering parity does
not replace the supervisor's dataset requirement.
