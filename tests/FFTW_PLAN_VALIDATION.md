# Native single FFTW reference plan (2026-10-16)

## Finding

The installed Octave 11.3.0 reports `fftw('threads') == 3`, but that is the
double planner. Its separately initialized native single planner uses **22**
threads on this host. Earlier notes treating the getter as the single setting
were incorrect. This was confirmed through the installed public C++ interface,
actual printed native plans, and the exact release source.

Official source inspected:
[Octave 11.3.0 oct-fftw.cc](https://raw.githubusercontent.com/gnu-octave/octave/release-11-3-0/liboctave/numeric/oct-fftw.cc).
Local inspection copy: `validation/oct-fftw-release-11-3-0.cc`.
SHA256: `DB730C34E2C6DDB5FB078D569FA8A6A259903FA14DE278B8374668658B7545B6`.
The double constructor caps its default at 3; the single constructor uses the
available processor count. No reference source was edited.

The R2C wrapper uses a compact output at offset 8064 complex values within a
128x128 complex buffer, then completes the missing Hermitian half. The inverse
uses complex FFT followed by normalization. The diagnostic mirrors these API
calls and layouts, without copying the Octave/FFTW implementations.

## Tools and checks

- `inspect_fftw_native.cc`: temporary Octave extension calling the installed
  public native FFT wrapper and printing its exact plan, operation counts and
  native single thread setting. It is a reference probe, not production code.
- `validate_native_fft_probe.m`: asserts equality with ordinary `fft2`/`ifft2`
  before accepting those plans. On asymmetric ref/test, both MTF and adaptation
  inputs passed all eight transforms (131072 complex values).
- `inspect_fftw_plan.cpp` / CMake target `IQM-fftw-plan-test`: independently
  loads the same local single FFTW DLL and reproduces API calls/layouts. It
  compares actual single thread count, 3 and 1, each aligned/unaligned. Every
  candidate starts with cleared wisdom to avoid contamination from prior cases.
- `validate_fftw_plan.m --strict-native`: requires the configured native-thread,
  unaligned candidate to match both directions for both ref/test images.
  Other candidates are diagnostics, not parity passes.

With the correct 22-thread estimate/unaligned setting, independent forward
and inverse outputs match Octave exactly for the three patterns (six images):
**196608 complex values, max error=0, RMSE=0, mismatches=0** on MTF snapshots.
This is a CPU reference-plan result; the Vulkan FFT still has nonzero errors.

## Native 128x128 plan decomposition

Forward: real 128-point `r2cf_128` on the first axis, followed by buffered
complex transforms using `t3fv_8_avx2_128` and `n2fv_16_avx2_128` on the second.
Counts: 140658 additions, 38106 multiplications, 19584 FMAs.

Inverse: first-axis radix decomposition uses `t2_8` and `n1_16`; second-axis
buffered transforms use `t3bv_16_avx2_128` and `n1_8`.
Counts: 408576 additions, 108544 multiplications, 47104 FMAs.

Thread partitioning changes which routines process tail batches. Using the
wrong single thread count left forward differences on rows 64/66 (one-based).
Changing Hermitian completion was an unsuccessful hypothesis; no such numeric
patch was applied. Correct plan selection eliminated those differences.
GPU parity requires matching these arithmetic graphs/roundings and batch-tail
selection, not literally running 22 CPU threads on a GPU.

## Reproduce

Build the diagnostic target (Windows only):

```powershell
cmake --build build-vs2026 --config Release --target IQM-fftw-plan-test
```

Build the probe with installed Octave's `mkoctfile.exe` into a **new** directory.
The compiler requires this process's PATH to contain Octave's mingw64/bin and
usr/bin. Do not change the machine-wide PATH.

```powershell
# Supply a fresh directory that has already been created:
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/mkoctfile.exe' tests/inspect_fftw_native.cc -lfftw3f -o '<probe directory>/inspect_fftw_native.oct'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_native_fft_probe.m '<probe directory>' '<GPU export directory>'
# Read actual thread count from probe directory/single_threads.txt; don't assume 22 on another host/session.
& './build-vs2026/bin/Release/IQM-fftw-plan-test.exe' '<GPU export directory>' '<new diagnostic directory>' 22
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fftw_plan.m '<diagnostic directory>' --strict-native
```

The optional final argument of the C++ utility imports a native wisdom file
for cache-key diagnostics; it labels whether each plan actually came from that
wisdom. An import success alone is not proof that a particular plan matched.

Artifacts: `validation/fftw_native_probe_threads.log`,
`validation/octave_plan_probe_639277410904713098/*_plan.txt`,
`validation/fftw_true_single_strict.log`,
`validation/fftw_native_diagonal_comparison.log`,
`validation/fftw_native_impulse_comparison.log`. Earlier `fftw_plan_*`,
`fftw_many_plan_*`, and `fftw_native_wisdom_*` directories preserve the wrong
3/1-thread experiments; their forward outputs are not native parity passes.

## Next implementation step

Isolated inverse `n1_8` and `n1_16` GPU tests now match the local single FFTW
DLL exactly on 12288 complex outputs (selected graph=1/fused=0). See
`fftw_reference/README.md` for fixtures, precision controls and reproduction.
This validates unnormalized leaf routines, not the complete inverse transform.
The subsequent composed 128-point inverse test also matches exactly on 512
transforms / 65536 complex outputs, including `t2_8` twiddles and strides with
`n1_16`. The DLL prints the same decomposition as the native first inverse
axis. Buffered `t3bv_16_avx2_128`/`n1_8` arithmetic now also matches exactly:
group sizes 128, 6 and 2, totaling 197120 complex outputs. In-place stride-128
oracle planning is essential to select the correct SIMD graph. See the
second-axis section of `fftw_reference/README.md` for evidence and commands.
The full normalized GPU 128x128 inverse now also passes: 19 matrices / 311296
complex outputs, zero error against the 22-thread DLL oracle, independently
verified against native Octave ifft2. This covers synthetic inputs and ref/test
MTF/adaptation stage2 snapshots for all three patterns. See the complete 2D
section of `fftw_reference/README.md`. Production preprocessing is unchanged.
Forward first-axis r2cf_128 now also matches on 2048 real transforms, with
133120 computed half-spectrum bins (262144 values after conjugate completion).
All fixtures independently match native Octave fft. See the forward R2C
section of `fftw_reference/README.md` for evidence and commands.
Forward SIMD n2fv_16/t3fv_8 now matches groups65/3; the final group2 uses
**n1fv_128_avx2_128** and has a separately validated graph. Across all three
groups, 197760 complex outputs match the DLL oracle exactly. This tail differs
from the composite main path; preserve it when assembling the full transform.
Full128x128 forward composition now also passes:19 matrices, 311296
full-spectrum complex values including conjugates, zero error. CPU expected
matrices independently match native single Octave fft2 (22 threads). This
includes real MTF/adaptation stage0 snapshots for all3 spatial patterns.
Next integrate the proven forward/inverse routines into an explicit experimental
path and rerun strict full-chain preprocessing comparisons. Compare intermediate
stages before claiming whole-metric parity.
Existing GPU log/exp/subnormal differences and full metric validation remain.
