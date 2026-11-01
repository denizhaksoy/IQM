# Experimental 96x96 reference seed

This is preparation for a new geometry. The public pathway runtime still
accepts only 80x96 and 96x80; no 96x96 image-to-score GPU parity is claimed.

`generate_general_pathway_reference.m` accepts `--shape 96x96`, or either
existing orientation. Omitting the flag retains the original two-case export.
The installed official HDR-VDP-3 v3.0.7 sources remain untouched. The isolated
instrumented copy must match stock Q and Q_JOD exactly before exporting.
`export_native_band_errors.m` now discovers shapes from native metadata and
retains the five-band official Minkowski expression and accumulated-error checks.

Checkpoint: `validation/geometry96_639286130307808713` (2026-10-26).
Native configuration: positive Float32 grayscale luminance, 30 ppd, age24,
D65/sp0, CPU FFTW3 threads, estimate planning with fresh single wisdom,
192x192 padding, native DC removal. Nonidentity cases use the original
localized gain1.4 and offset0.25. Native complex matrices are exported as
interleaved row-major real/imaginary values; native selection indices remain
one-based column-major indices.

| Input | Q | Q_JOD | Selected base index |
|---|---:|---:|---:|
| Asymmetric | 3.991631507873535 | 4.826954841613770 | 13 |
| Impulse | 2.771334171295166 | 3.444155216217041 | 13 |
| HDR ramp | 7.395176887512207 | 8.227042198181153 | 13 |
| Constant | 2.771337747573853 | 3.444158554077149 | 13 |
| Identity | 10 | 10 | 1 |

All five stock/traced scores agree. The base is 12x12/144 elements, with
five pyramid bands. `validate_native_geometry96.m` independently replays the
base FFT, comparing 1,440 real/imaginary Float32 words including signed-zero
bits, then checks DC removal, actual index, frequency and adaptation. All pass.
Nonidentity maxima have two equal-magnitude bins; the first native column-major
bin has frequency0.3125000000000001. Identity has144 tied zero bins,
frequency0 and positive-infinite NCSF noise, as in earlier identity controls.
Replay inputs, raw spectra, DC-zero spectra and single-precision wisdom are
preserved under `base_replay`; these are diagnostic/reference artifacts.

Default regressions: all206 original numerical text exports remain byte-identical
(207 files including the PASS marker). The metadata-driven band-error exporter
matches the previous helper across222 text files, including additional errors
and psychometric maps. A first regression command referred to a missing old
snapshot; the successful retry used the retained real-HDR source snapshot.

The isolated `generate_residual144_shader.ps1` reuses the FFTW-derived scalar
12-point real/complex codelets. `compile_residual144.ps1` preserves Float32
denormals and RTE and validates SPIR-V. `tests/hdrvdp3_residual144` builds the
diagnostic caller; `--square144` requests strict bit checks and converts each
row-major input to the kernel's column-major input layout. Existing residual120
binary artifacts and runtime packages are preserved. Compilation alone is not
GPU numerical acceptance.

## Isolated GPU base FFT result

The initial graph passed four nonidentity base inputs exactly but failed
identity by60 signed-zero bits. Octave's
[real-matrix narrowing](https://docs.octave.org/doxygen/11/de/d87/ov-flt-cx-mat_8cc_source.html)
converts a complex matrix whose imaginary components are all zero to a real
matrix. The isolated shader now applies that output representation rule,
preserving real components and retaining complex imaginary zero signs when
any imaginary component is nonzero. No tolerance was introduced.

Additional probes exposed an export issue in the new complex-text helpers:
accessing a complex scalar before taking its imaginary part can itself narrow
the scalar and lose a negative-zero sign. Both new exporters now extract whole
real/imaginary arrays before indexing. Earlier exports remain in place for
audit; `base_replay_v2` and `fft_probes_v2` are the corrected goldens. Existing
production/native stage exporters already extract whole imaginary arrays and
were unaffected. The first Windows-path manifest also failed because C++
`std::quoted` consumes backslash escapes; portable manifests use forward slashes.

Against corrected goldens, all five real-pathway base inputs and five separate
FFT probes pass: 2,880 Float32 words bitwise, including signed zeros, without
Vulkan validation errors. Probes cover zero, constant, origin impulse, Nyquist
checkerboard (entirely real spectra) and shifted impulse (complex spectrum).
This proves the tested isolated 12x12 FFT stage; it does not prove a complete
96x96 GPU visual pathway or universal FFT-input coverage.

The caller's original residual120 route retains its historical result:
18,240 components numerically equal with490 signed-zero differences. That is
not a bitwise acceptance and remains explicitly excluded from exact parity.
No existing runtime package, production dimension guard or shader binary changed.

The next isolated mean/DC, difference and FFT/magnitude/selection chain is
documented in [BASE144_PIPELINE_VALIDATION.md](BASE144_PIPELINE_VALIDATION.md).
