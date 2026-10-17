# Luminance, adaptation, pupil and JND stage diagnostics

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


## Single lookup boundary correction (2026-10-17)

`generate_jnd_knot_reference.m` builds the official JND tables and18432
positive luminance inputs around every LUT node for all three D65 pathway
coefficients, including the immediately adjacent float inputs. The isolated
`IQM-jnd-knot-test` loads those same double tables and compares native single
log queries, channel interpolations and final responses. This isolates
sampling from differences in LUT construction or adaptation.

Before correction: zero log-query mismatches,1534 channel mismatches and550
response mismatches. After correction all three counts are zero across18432
cases (55296 channel queries). Octave lookup converts the double coordinate
table to single when querying with single values. C++ previously compared
against unrounded double nodes, selecting the wrong interval at rounded
knots. The interpolation slopes still come from the original double table;
only the interval comparison was corrected.

Reproduce into a fresh reference directory:

```powershell
octave-cli --quiet --no-gui tests/generate_jnd_knot_reference.m '<new directory>'
cmake --build validation/fft_publish_build --config Release --target IQM-jnd-knot-test
& './validation/fft_publish_build/Release/IQM-jnd-knot-test.exe' '<directory>'
```

Evidence: validation/jnd_knots_reference_path.txt, jnd_knots_before.log,
jnd_knots_corrected.log and matching build logs. The broader quality suite
now also invokes IQM-jnd-single-test against all13 native trace values after
each strict preprocessing check. Build both IQM and IQM-jnd-single-test before
running that suite. Older results below retain their original scope.

The same18432 inputs also pass with `--production-lut`, which calls
create_jnd_lut instead of sampling the loaded reference tables. All2048
rounded grid, cone and rod entries match; log queries, channels and final
responses again have zero mismatches. The LUT source is compiled separately
with the production default FP options; the single sampler/test use strict
FP. Evidence: validation/jnd_knots_production.log and matching build log.

Integrated post-fix regressions pass four HDR pairs plus all six prior pairs:
validation/quality_hdr_suite_639278605867642854/PASS.txt and
validation/quality_suite_639278607136633457/PASS.txt. Each case checks all13
native intermediates for reference and test images, strict full preprocessing,
CSF/masking/D maps and direct official Q/Q_JOD. Reference luminances now span
0.001–10000 cd/m² (15000 maximum in the stronger test), still64x64/30ppd,
age24,D65 and opt-in reference FFT/math. Other configurations remain unproved.

## Current native-single result (2026-10-16)

The harness now uses `jnd_single.cpp`, compiled with contraction disabled
(`/fp:strict` on MSVC), for native single pupil and LMSR/JND arithmetic.
The double API remains available. The single response transform is scoped to
the existing age-24 single-channel D65 harness, not arbitrary display spectra
or observer ages. Adaptation is still computed on the GPU; pupil and JND are CPU.

For diagonal, asymmetric, and impulse patterns (reference and test each),
all 24576 final JND values and all 13 per-pixel trace values matched Octave
exactly. Pupil coefficients matched exactly on identical GPU adaptation maps.
`IQM-jnd-single-test` compares the library implementation directly with native
Octave traces. All six new SPYR pyramids also passed zero-error comparisons.

`trace_jnd_single.m` additionally computes official adaptation from the native
L+M input and uses that map for the pupil calculation. This is a separate
comparison from the shared-map test. See `validation/native_*_trace.log`.
For all six images, the official-adaptation comparison also had zero final
JND mismatches and zero pupil-coefficient difference. The adaptation maps
themselves still differ; these cases round to the same downstream results.
The test isolates the MTF-off, age-24 D65 path; it is not full metric validation.

Reproduce after running the harness and noting its printed export directory:

```powershell
cmake --build build-vs2026 --config Release --target IQM IQM-jnd-single-test
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/trace_jnd_single.m '<export directory>'
& './build-vs2026/bin/Release/IQM-jnd-single-test.exe' '<export directory>'
```

## Earlier double-path investigation (historical)

The following measurements predate native-single integration. They explain
the change; they do not describe the current harness's arithmetic.

`diagnose_jnd_path.m <export directory>` reads the harness's luminance,
adaptation, pupil and JND exports. It uses the local official 3.07 data and
sensitivity functions. Spectral D65 integration includes the default age-24
optical density correction (`do_aod=true`). Its LMSR coefficients agree with
the current C++ constants. The script reproduces the equations of the private
JND LUT builder; it does not run the full visual pathway or its MTF stage.

The harness now derives pupil adaptation from each GPU adaptation map's
geometric mean instead of hardcoded 10/10.5. Logarithms and reduction currently
use double. The exported JND arrays remain float32.

Modes in the diagnostic output:

- 1: Identical exported pupil coefficient; independently reconstruct JND.
- 2: Pupil from official adaptation with double geometric-mean reduction.
- 3: Pupil from native Octave single `geomean` and pupil arithmetic, then
  the same double JND evaluator. This isolates pupil precision; it does not
  emulate all single rounding in the official LMSR and JND stages.

For the asymmetric pattern after the fix, modes 1 and 2 match all 8192 JND
values exactly. Mode 3 differs by at most 0.0001220703125, with 1627 reference
and 691 test pixels differing. These are remaining precision differences,
not an end-to-end parity pass. Native single geometric-mean accumulation and
the full single LMSR/JND path must be traced next before changing arithmetic.

Adaptation itself remains approximate: asymmetric reference max error
1.9073486328125e-5 cd/m^2 (RMSE 7.410627770822678e-6); test max error
2.288818359375e-5 (RMSE 9.525129485008725e-6), against official single
`hdrvdp_local_adapt` at 30 pixels/degree on identical luminance inputs.

The diagnostic asserts that the exported pupil agrees with the double
geometric mean of the GPU adaptation and that mode 1 has zero mismatches.
Modes 2/3 report differences for investigation, without imposing an arbitrary
tolerance. SPYR should be checked separately on the newly exported JND arrays.

Evidence: `validation/jnd_path_diagnostic.log` (before pupil fix),
`validation/jnd_geomean_diagnostic.log`, and `validation/jnd_geomean_spyr.log`.

## GPU pupil/JND continuation (2026-10-17)

The historical double-path limitations above have been superseded by the
native single CPU path and the opt-in ABI7 MapGPU port. GPU now computes
adaptation preparation, single geometric-mean pupil reduction, all13 D65
photoreceptor/JND stages, and mean-adaptation log10 for CSF. Double calibration
LUT construction remains on the CPU. Math/shader generation uses fixed MIT
AMD sources and mathematical power tables, without reading a Windows DLL or
requiring shaderFloat64.

All20 images from the ten saved validation pairs match all13 GPU intermediates
against native Octave traces. The GPU knot fixture also passes258048 values
using both native and production LUTs. See [current GPU map validation](MAP_GPU_VALIDATION.md)
for numerical rules, integration evidence, source generation and remaining work.
