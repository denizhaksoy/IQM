# Isolated forward256 validation

Date: 2026-10-21. This is an FFT component checkpoint, not an end-to-end
128x128 HDR-VDP-3 metric pass. Public quality API guards remain32/64.

Eighteen two-axis GPU runs match native single-precision Octave `fft2` in
2,359,296 float components with zero numerical mismatches and no tolerance.
The inputs comprise four actual128x128-image MTF/LA padded matrices and fourteen
synthetic matrices: zero, off-center real impulses, seeded random1e-3/1/1e3,
random subnormal1e-40 and boundary1e-38, with separate ref/test fixtures.
Signed-zero bit identity is not asserted. This is evidence for these graphs,
inputs, compiler and device, not universal bit identity across hardware.

The saved first-axis outputs of those same18 runs also match native FFTW
many-plan references in1,188,864 components. They are additional intermediate
comparisons, not18 extra GPU runs. The independent two-plan CPU reconstruction
matches all GPU final outputs too; for the four actual MTF/LA cases it also
matches the pre-existing Octave exports in524,288 components.

The native plan uses FFTW3.3.10 from local Octave11.3.0, three threads,
FFTW_ESTIMATE|FFTW_UNALIGNED and fresh wisdom. Its first axis is
hc2cf_32/r2cf_32/r2cfII_32 with r2cf_8 leaves, divided into86/86/84 transforms.
The second axis is a buffered256 transform with129 transforms divided43/43/43,
using t3fv_16_avx2_128 and n2fv_16_avx2_128. The isolated many-plan oracles
check those codelet fingerprints. The GPU reproduces the arithmetic graphs;
it does not reproduce CPU threading or buffer-copy scheduling.

Input is256x256 column-major interleaved float32, imaginary components zero.
The first axis writes256 contiguous groups of129 complex frequencies. The
second axis transforms the other dimension, expands the Hermitian half, and
writes a full column-major spectrum. The checker explicitly converts that
storage to row-major interleaved output for the Octave comparison. No DC
removal, mean subtraction, normalization, double shader arithmetic, or CPU
output substitution occurs. Padding is already present in the actual inputs:
symmetric post-padding for MTF, replicate post-padding for LA, as recorded by
the existing native fixture metadata. Synthetic fixtures have no padding.

Original FFTW codelets, GPL notices and archive/member/LF-normalized SHA256
provenance are retained under `tests/fftw_reference/fft256`. The scalar graph
uses separate Float32 operations; native SIMD complex products retain their
FMA graph. Twiddle descriptors are parsed/checked rather than guessed.
SPIR-V explicitly requires DenormPreserve32 and RoundingModeRTE32. The checker
rejects devices lacking these controls. Tests ran on Intel Arc Pro Graphics.

Reproduce shader generation using a fresh output directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_inverse.ps1 -OutputDirectory <fresh-shader> -VulkanBin <SDK-Bin> -Forward
fft256-inverse-test <native-fixture> <fresh-shader>/forward256.spv ref <fresh-export> --forward
fftw-plan-test --forward256-axes <native-fixture>/ref_mtf_stage0.txt <fresh-axis-oracle>
```

The target names remain compatible with the previous inverse checkpoint.
See `tests/hdrvdp3_fft256/README.md` for filenames and first-axis-only mode.
`tests/generate_fft256_forward_reference.m` generates fresh native synthetic
fixtures and records precision, planner, threads, layout and normalization.

Local evidence: `validation/forward256_axes_639281632814118746`, with
`full_records.json`, `independent_axis_checks.json`, `comparison.json`,
`full_shader`, `full_accepted_*`, and native `oracle_*` exports.
`validation/run_forward256_suite.ps1` and `check_forward256_axes.ps1` record
the local invocations. Four initial first-axis-only GPU trials passed; these
are excluded from the18 full-run counts. One PowerShell logging attempt was
interrupted by native diagnostic stderr under Stop; it is retained separately.
The first full MTF probe passed and is likewise excluded from those18 counts.

Inverse regeneration with the extended compiler still produces SHA256
A07F14752C10E713EB85DA51F5AE4379D9DEF03CB74B5830077CDD27EA0A1CED,
identical to the accepted inverse256 shader. Existing32/64 asset pins and
production implementation remain unchanged.

Next: integrate both isolated FFT256 graphs into a diagnostic MTF/LA chain,
validate preparation, multiply and finish outputs at128x128, then extend
residual FFT beyond8x8 and validate larger map/masking/reduction stages before
lifting the public API's dimension guards.
