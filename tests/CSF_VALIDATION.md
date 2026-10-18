# Direct nCSF reference validation

> Current scope: scalar CSF/resize code and isolated reference checks are committed locally in `51525e0` and `7ce32a0`. GPU integration and full evidence remain local. No push was performed in this session. Earlier measurements below retain their historical scope.

## Exact scalar checkpoint (2026-10-18)

The standalone [scalar validation project](hdrvdp3_scalar/README.md) builds
without Vulkan. An export of the actual final Git commit passes8/8 CTest
checks,including strict double direct CSF,base noise,LUT and resize checks.
Direct equation division order now follows the official reference;all15675
sampled base CSF/noise double comparisons match.

The LUT grid previously produced539 differing double nodes and3194 differing
double queries despite exact single outputs. Symmetric construction of the
fixed256 logarithmic nodes removes these differences:3840 nodes and22530
queries now match in double and single. A separate fixture includes42300
queries at knots,nextafter neighbors,clipped endpoints and random positions;
the committed executable reports zero differences for all query modes.

Evidence is under `validation/scalar_checkpoint_639279508489310995`.
Eighteen fresh full-pipeline pairs after this change also match official
Q/Q_JOD exactly,using one common application/checker binary set. Evidence:
`validation/lut_grid_regression_639279511058157010/COMPLETED.json`.
These results supersede historical double-LUT mismatch counts below only
within the recorded fixtures;they do not establish arbitrary-parameter parity.


The isolated IQM-csf-reference-test target compares the C++ direct nCSF API
against fresh official HDR-VDP-3 v3.0.7 hdrvdp_ncsf queries. Its reference covers
1152 cases:3 ages(24,45,70), AESL on/off,12 spatial frequencies (including the
low-frequency cutoff and the five current pyramid frequencies), and16 luminances
including interpolation knots and clamped endpoints.

The paired-vector query shape is intentional. In the official helper a scalar
rho<=1e-4 with vector luminance makes its logical assignment select only the
first output element. The test instead pairs rho/luminance vectors, so each
sample follows the scalar point-query semantics. The initial scalar/vector
fixture is retained as a diagnostic, not used to justify a C++ cutoff change.

## Changes and measured scope

The AESL exponent previously used `10^(slope*log2(rho+gamma))`. Official AESL
uses `10^slope * log2(rho+gamma)` inside the outer exponent. Correcting that
fixes a substantive age-dependent error masked by the prior age24-only tests.
Parameter interpolation now follows slope*(x-x0)+value, and neural sensitivity
preserves the reference reciprocal/power/multiply order. The main library's
csf.cpp is compiled with strict FP/no contraction, like the native JND source.

| Comparison | Before correction | After correction |
|---|---:|---:|
| Differing float32 outputs | 320 | 0 |
| Differing float64 outputs | 444 | 283 |
| Maximum absolute double error | 1.2169949371813404 | 2.2204460492503131e-16 |

The remaining double differences are reported, not silently accepted as an
exact pass. Default test mode requires exact double equality and still exits1.
Explicit --float-output compares the rounded float outputs exactly and passes
all1152 cases; it is not a tolerance test. This does not yet validate the
256-entry CSF LUT, log-luminance map resizing or the integrated CSF maps.
Those remain the next CSF step, including the reference/test mean adaptation.

```powershell
cmake --build validation/fft_publish_build --config Release --target IQM-csf-reference-test
# Always use a new reference output directory:
octave-cli --quiet --no-gui tests/generate_csf_reference.m '<new directory>'
& './validation/fft_publish_build/Release/IQM-csf-reference-test.exe' '<directory>' --float-output
# Without --float-output: strict double diagnostic, currently expected exit1.
```

Evidence under validation/: csf_paired_reference_639278581311962498/csf.txt,
csf_paired_before_fix.log, csf_corrected_comparison.log,
csf_float_output_comparison.log, csf_corrected_build.log, csf_precision_build.log.
Official reference files were not modified. Full metric scores and Vulkan CSF
shader parity are outside this direct C++ test.

## Isolated resize validation (2026-10-17)

`generate_resize_reference.m` calls the installed Octave 11.3.0 image-2.20.0
`imresize` with its default bicubic, antialiasing-enabled settings. The independent
scalar C++ `IQM-resize-reference-test` reproduces horizontal-then-vertical
filtering, symmetric boundaries and single-precision products/accumulation.
No production resize code has been replaced yet.

All 45 cases / 32643 output pixels match exactly as float32 (zero mismatches,
maximum error zero). Cases cover 64x64, 13x19 and 1x17 inputs, identity,
half/quarter/eighth-size outputs (floored and bounded to one), enlargement,
smooth asymmetric patterns, corner impulses and alternating signs. Data is
serialized explicitly in row-major order with round-trip float32 precision.
Evidence: `validation/resize_reference_path.txt`, `validation/resize_build.log`,
`validation/resize_comparison.log`. This is parity with the installed Octave
image package; it does not establish equivalence with every MATLAB release.

```powershell
octave-cli --quiet --no-gui tests/generate_resize_reference.m '<new directory>'
cmake --build validation/fft_publish_build --config Release --target IQM-resize-reference-test
& './validation/fft_publish_build/Release/IQM-resize-reference-test.exe' '<directory>'
```

Next: validate the 256-node LUT and interpolation using the actual log10 nodes;
combine reference/test adaptation in single precision, apply native single
log10, resize without an exp/log roundtrip, then compare each integrated CSF
band map against the official reference. Existing production code still uses
reference-only adaptation and bilinear resizing, so full CSF parity is pending.

## LUT and log-domain sampling validation (2026-10-17)

`generate_csf_lut_reference.m` evaluates official 256-node logspace LUTs for
five pyramid frequencies (15,7.5,3.75,1.875,0.9375) and ages24/45/70 with AESL.
Queries include every node, interval midpoint and 1001 single-rounded log
coordinates covering [-5,5], deduplicated to1502 queries per LUT.
`IQM-csf-lut-reference-test` requires exact equality after float32 rounding.

Results:3840 nodes and22530 interpolated values have zero float32 mismatches.
Double equality is NOT achieved:1432 LUT nodes and8341 queries differ, maximum
query error8.88178e-16. The previous weighted interpolation gave10157 differing
double queries; both variants matched this fixture after float32 rounding.
The new `sample_csf_lut_log` samples log10 of the actual C++ LUT luminances
using slope*(query-x0)+y0 and avoids an exp/log roundtrip. Existing luminance
sampling delegates to it. C++ logspace nodes themselves have not been proved
bit-identical as double to Octave logspace. This float result does not imply
double parity, direct single-query interp1 arithmetic parity, or integrated
band-map parity. The direct nCSF1152-case float regression also still passes.

Evidence: `validation/csf_lut_reference_path.txt`, `csf_lut_baseline.log`,
`csf_lut_corrected.log`, `csf_lut_corrected_build.log` under validation/.
Run the generator into a fresh directory, build `IQM-csf-lut-reference-test`
in validation/fft_publish_build, then pass that directory to the executable.
Next work is reference/test mean adaptation, single log10 and bicubic resize
integration with official per-band map exports. No final metric pass is claimed.

## Integrated single CSF maps (2026-10-17)

The wrapper already formed the reference/test adaptation mean; the previous
reference-only comment and review were inaccurate. The integration now takes
single log10 of that mean, resizes with the validated bicubic implementation
in `src/hdrvdp3/resize.cpp`, and uses `sample_csf_lut_log_single`.
That sampler follows Octave ppval single offsets, products and sums. Octave
lookup also converts the grid to single for single queries; omitting that
conversion failed372 of22530 broad single queries, now zero. Double-query
validation remains as above; no double parity is claimed.

All three asymmetric/diagonal/impulse experiments pass exact comparison of
4096 log-adaptation values,9536 resized values and9536 sensitivity values
per experiment (28608 CSF output pixels total). The reference uses the
exported adaptation inputs, official nCSF and default installed imresize.
No DC removal is applied to adaptation or CSF maps. Exports include dimensions
and row-major round-trip float32 values; tests explicitly transpose on import.
Upstream strict preprocessing and raw/centered SPYR regressions also pass on
the asymmetric final export, establishing the chain for that case.

Run `tests/validate_csf_maps.m <absolute export directory>`. Evidence paths:
`validation/csf_maps_{asymmetric,diagonal,impulse}_final_path.txt` and matching
`_final.log`, `csf_regression_{stages,preprocessing,spyr}.log`,
`csf_lut_single_reference_path.txt`, `csf_lut_single_corrected.log`.
The production resize implementation itself passes the existing45-case,
32643-pixel fixture (`resize_production_comparison.log`). Current integrated
scope remains64x64,30ppd,age24,D65 luminance and opt-in reference FFT/math;
CSF/resize are CPU operations, not yet Vulkan shader implementations.

Subsequent masking, raw/transformed D and direct official quality-mode
comparisons now pass six input-pair cases. See
[masking and quality validation](MASKING_QUALITY_VALIDATION.md) for scope,
reproduction commands and the completed suite artifact. Double CSF differences
and the lack of complete Vulkan shader coverage remain explicit limitations.

## Subsequent Vulkan map backend

CSF LUT sampling and bicubic resize/clamp now have an opt-in Vulkan backend
with exact float32 comparisons and ten end-to-end quality pairs passing.
LUT construction still uses CPU work; mean-adaptation log10 is now evaluated
by the opt-in ABI7 GPU backend.
Both CPU and GPU now clamp the resized log map to[-5,5] before computing
base-band adaptation as required by the reference; previously the sampler
alone clamped its query. Ten separate bounds cases explicitly exercise4136
overshooting pixels. See [Vulkan map validation](MAP_GPU_VALIDATION.md) for
the current ABI7 shader, historical ABI3 results, independent fixtures,
integration evidence and remaining work.
