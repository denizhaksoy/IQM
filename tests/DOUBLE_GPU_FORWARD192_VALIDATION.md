# Complete Double192x192 forward GPU FFT

Validated2026-10-29. Completed root:
`validation/double192_full_completed_path.txt`.

The real192 first axis and complex192 second axis now run consecutively on
NVIDIA RTX A500 using Float64. No CPU intermediate is supplied between the
two GPU dispatches. The input boundary is the native192x192 Double padded
optical/adaptation array; this is an FFT stage test, not original-image to
GPU quality prediction. RTE64 and SignedZeroInfNanPreserve64 modes are
explicit; validation was enabled throughout.

The second-axis graph is8x12x2, matching the observed FFTW3.3.10 native
buffered192 transform: t3fv_8_avx2_128,t1fv_12_avx2 and n1_2. Both generated
SIMD codelets require their FMA-branch graphs (variant0). The previous
Float32 t1fv_6 choice cannot simply be reused for Double t1fv_12. Complex
products preserve the explicit fused/separate operation order.

The additional t1fv_12 source was extracted from the already-present official
archive after checking SHA256
`56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467`.
Its LF hash is pinned in
`tests/fftw_reference/double192/SOURCES.json`; all old sources are preserved.
Generated derivatives retain GPL-2.0-or-later/COPYING. Independent Double
octant-reduced twiddles use24-point powers1..11 and192-point powers1,3,7.
The prior154-word first-axis twiddle hash remains unchanged.

Fresh reference packets contain the partial first-axis transform for isolated
second-axis tests, the padded real input for coupled tests and the full native
fft2 result. Audit additionally verifies that fresh full results are byte-equal
to the actual FFT outputs previously captured inside official hdrvdp3 visual
processing. Double FFTW uses three threads,estimate planning and cleared
wisdom. Layout is column-major with interleaved real/imaginary components;
the half spectrum has97 rows per column, the full spectrum192. Forward
normalization and DC remain unchanged. Source profiles are original encoded
Single96x96/30ppd/age24/defaultsp0, with symmetric optical post-padding and
replicate adaptation post-padding.

Complete coupled GPU forward tests passed24 matrices per case and72 total:
stress pair, ramp pair and identity control, each under three spectra/two
sides/three optical channels plus adaptation. Identity reuses stress reference
content. **5,308,416 Double component words matched all bits**, including
signed zero. The isolated second-axis tests also match5,308,416 words, but
these are the same final values and overlapping coverage; do not add them
as independent image coverage. One12x12 base FFT and real192 first-axis
regression passed864 and893,952 words respectively on the updated harness.
All accepted Vulkan logs were checked without reported validation errors.

Initial graph choices produced346,634(t1=1,t3=0),289,065(t1=0,t3=1) and
556,260(t1=1,t3=1) bit mismatches in the first24-matrix packet. Variant0/0
eliminated all differences. Failed candidates remain in separate directories
and are not counted as passes. Accepted directories are `accepted_forward_*`
and `accepted_second_*`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/compile_forward192_double.ps1 -OutputDirectory validation/new_forward192_double
$r = Get-Content validation/double192_full_completed_path.txt
& "$r/fft-double-gpu-test-used.exe" "$r/native" "$r/forward_pack/first/real192_double.spv" "validation/forward192_recheck_$([DateTime]::UtcNow.Ticks)" --forward192 "$r/forward_pack/second/complex192_double.spv"
node tests/hdrvdp3_evaluation/audit_double192_forward.cjs $r
```

Production application routing and shaders_out were not changed this session.
This is not yet a strict production runtime package. Next implement the
Double inverse192 axes and normalization, validate full optical/adaptation
filtering, then integrate the remaining visual/quality GPU stages and CLI.
Intel Arc still lacks shaderFloat64; the Float64 tests explicitly select
NVIDIA. Full GPU scores and arbitrary Float64/subnormal coverage remain open.
