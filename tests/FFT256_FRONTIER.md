#128 image /256 FFT native frontier

Follow-up:the inverse graph now passes isolated GPU/full-inverse andindependent
first-axis tests with explicit float controls;see
[inverse256 checkpoint](FFT256_INVERSE_VALIDATION.md). Forward256 andfull128
metric support remain open. Native frontier results below retain their scope.

The native planning fixture and read-only FFTW DLL inspector now support
128x128 input /256x256 FFT as diagnostics. The public metric API and GPU
backend guards remain at the previously verified square32/64 domain.

Official local HDR-VDP-3 v3.0.7/Octave single MTF and local-adaptation fixtures
were generated for asymmetric reference/test inputs at30ppd. Padding is
symmetric post for MTF and replicate post for adaptation. Stage0/2 exports
are column-major interleaved;native spectra are row-major interleaved. Filter
coefficients are double,FFT outputs single,inverse normalization1/65536.
The staged MTF reconstruction matches official fast_conv_fft with zero differences.

The actual Octave planner was estimate with3 threads. The independent native
DLL inspector tested1/3 threads,aligned/unaligned plans,forward/inverse and
reference/test. All8 unaligned cases match actual Octave FFT outputs exactly:
**1,048,576 float components,zero mismatches**. All8 aligned cases differ and
are retained as rejected alternatives. This is CPU/native plan validation,
not GPU FFT256 or full metric128 validation.

The observed unaligned forward plan requires composite real graphs
hc2cf_32,r2cf_32,r2cfII_32 andr2cf_8. Its complex-axis plan includes
t3fv_16_avx2_128/n2fv_16_avx2_128;inverse plans include t3bv_32_avx2_128/n1_8.
These are not the existing64/128 FFT shader graphs with a larger size constant.

Evidence:validation/fft256_frontier_639281615622631080/COMPLETED.json;
native128/metadata.json,native_single_wisdom.txt,plans256/plans.txt,
plans256/comparison.json andcomparison.log. The inspector reads the local
Octave FFTW DLL and writes fresh diagnostics;it does not replace GPU results.

Next:translate the observed256 graphs and independently compare each stage.
Then extend residual-frequency FFT beyond8x8 andvalidate128-sized resize,
masking and reductions before widening the public API. Preserve the32/64
reference regressions throughout. General dimensions/RGB/full frames remain open.
