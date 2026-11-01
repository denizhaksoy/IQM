# Experimental 144-element base-band GPU chain

This extends the isolated 12x12 FFT work in
`GEOMETRY96_REFERENCE_VALIDATION.md`. It does not enable 96x96 in the public
runtime or establish complete image-to-score GPU parity for that geometry.
Checkpoint: `validation/mean144_639286139772659192` (2026-10-26).

The isolated official v3.0.7 visual trace now captures `BB` before native
mean removal and its native mean. Stock/traced Q/Q_JOD must still agree.
Five native96x96 cases (asymmetric, impulse, HDR ramp, constant and identity)
provide ten original base bands, native means and centered bands. Native input
and precision, 30ppd/age24/D65/sp0, 192x192 padding, FFTW three threads/estimate,
native DC removal and layout retain the earlier reference configuration.
All515 previously exported numerical stage files match the earlier96x96 seeds
byte-for-byte after this instrumentation.

`mean144.comp` sums finite normal Float32 values or zeros in exact signed
fixed-point arithmetic, then divides by144=9×16 with explicit nearest-even
rounding. Supported input exponents are [-90,100], with spread≤16; callers
check that domain before dispatch. At144 terms, at most48 magnitude bits
are needed, so the paired uint32 accumulator remains sufficient and the
corresponding CPU double accumulation is exact. Unlike division by120, the
normalization threshold uses9/8 and the binary exponent offsets7/8. The
integer division remainder supplies the final Float32 tie decision. No Float64
GPU feature or CPU intermediate calculation is required by this shader.

The standalone mean/DC test checks8192 deterministic bounded synthetic groups,
including positive/negative zeros, constants, cancellation and random signed
mantissas across the permitted exponent range: **1,187,840 output words**.
The ten official raw base bands add **1,450 words**. Every word must match,
including signed zeros. Native means must independently match the CPU
double-accumulation/Float32-rounding contract before comparison to the shader.

The second shader subtracts the centered reference band from the centered
test band once, then packs the real difference in column-major complex layout
with +0 imaginary components. The third shader runs the accepted 12x12 FFT,
applies Octave's entirely-real output representation, computes native-compatible
finite hypot with DC magnitude zero, and selects the first maximum by traversing
column-major order with strict `>` updates. Raw FFT export remains before DC
removal; magnitudes and selection use DC exclusion. The selected index exported
by the shader is zero-based; native indices are explicitly converted.

The three stages share GPU buffers with compute barriers and no host readback
between dispatches. Per pair, all290 mean/DC words,288 packed difference words
and433 FFT/magnitude/index words are checked. Five pairs pass **5,055 words
bitwise**, including FFT signed zeros and first-maximum selection. The identity
case is included. Vulkan validation reports no errors in the accepted runs.
Input bands are official native stage inputs; this is deliberately an isolated
stage test, not a full GPU visual-pathway claim.

Build `tests/hdrvdp3_residual144` and targets `mean144-test` and
`base144-pipeline-test`. Compile only the new shaders with
`compile_mean144.ps1`, `compile_mean144_difference_pack.ps1` and
`compile_residual144_extended.ps1` in `tests/hdrvdp3_dimensions`. Generate fresh
fixtures using `export_native_mean144.m` and `export_native_base144_pipeline.m`.
The native root must contain the five `--visual --shape 96x96` trace folders.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/validate_base144.ps1 -ExecutableDirectory <build>/Release -MeanShader <mean>/mean144.spv -DifferenceShader <difference>/mean144_difference_pack.spv -FFTShader <fft>/residual144_extended.spv -NativeDirectory <native-root> -OutputDirectory <fresh-results>
```

The same command with `-ResultsOnly` audits retained results. `acceptance_final`
is the final checked run. Earlier runs and failed setup attempts are preserved:
an initial identity argument was split by a PowerShell single-string splat;
the identity was generated successfully with explicit arguments, then fresh
complete fixtures replaced that attempt for acceptance. No numerical failure
or tolerance adjustment was needed for mean/DC or the connected chain.

The shared diagnostic callers retain their original120 defaults. Separate
legacy regression targets (`IQM_BASE144_LEGACY_REGRESSION=ON`) pass the original
mean/DC probes and two-orientation native GPU chain. Their historical FFT
zero-sign exclusion remains unchanged; those old claims are not promoted to
exact FFT parity. Existing shader binaries, runtime package pins, public
geometry guards and unrelated worktree changes remain untouched.

Next: prove the192x192 forward/inverse optical and adaptation FFT arithmetic,
then integrate the96x96 SPYR/base layouts and validate the complete pathway.
The next isolated FFT checkpoint is documented in
[FFT192_SQUARE_VALIDATION.md](FFT192_SQUARE_VALIDATION.md).
