# 128-frontier reduction, masking and resize components

Date:2026-10-21. These are isolated component tests,not an end-to-end128x128
HDR-VDP-3 score pass. Production MapGPU andquality API guards remain unchanged.
Numerical equality below uses no tolerance;signed-zero bits are not asserted
for Float32 tests. Binary64 mask-resize tests compare the actual64-bit words.

|Component|Measured cases|Compared values|Differences|
|---|---:|---:|---:|
|64-sample DC/base adaptation|512|34,304 Float32|0|
|256-sample DC/base adaptation|512|132,608 Float32|0|
|GPU DC→FFT16→magnitude/first-max|512|655,872 scalar values|0|
|128-square raw mutual masking/pool|16|524,288 Float32|0|
|128-square pupil traces|30|120 Float32|0|
|128-square quality-band pooling|16|16 Float32|0|
|Float32 bicubic resize|80 transitions|479,744 Float32|0|
|Binary64 comparison-mask resize|40 outputs|174,592 binary64 words|0 bits|

Counts describe separate scopes andoverlapping data;do not add them asunique
full-metric cases. Final reduction-shader replays at64/256 samples are excluded
fromunique-case counts. No CPU output substitution is used. All tests ran on
Intel Arc Pro Graphics withVulkan validation enabled andnoERROR/VUID diagnostics.

## DC and base adaptation

`generate_base_reductions_reference.m` independently generates native Octave
single inputs/outputs. Each case contains aresidual vector andan independent
log10-luminance vector. Expected outputs are native mean-subtracted residual,
residual mean,log mean and10.^log_mean. Cases include zero,constants,adjacent
Float32 values,signed cancellation,small values,random scales,anddeclared
exponent-boundary controls. Residual exponents are[-90,100],span<=16;log values
are[-5,5] withspan<=16 andminimum exponent>=-90. These retain the bounded
numerical domain used bythe existing64-sample path;arbitrary inputs are not
validated. The checker enforces the domain before dispatch.

The isolated shader reuses source-generated map arithmetic helpers:compensated
Float32 pair addition,exact product andnativePower. With256 samples,the exact
sum needs atmost24+16+8=48 significant bits inthis domain. Explicit Float32
DenormPreserve andRTE modes are required. Native mean rounding andpower-of-two
division are checked rather than inferred fromthe final metric score.

The subsequent GPU-only chain writes interleaved centered data directly to
the previously validated FFT16/magnitude/selection shader. There is no
intermediate CPU readback/upload. Its655,872 comparisons comprise131,072
centered values,131,072 zero imaginary padding values,262,144 FFT components,
131,072 DC-zero magnitude values and512 zero-based column-major first-max
indices. These are controlled residual inputs,not full visual-pathway outputs.

Evidence:`validation/base_reductions_639281814095504643`,includingnative64,
native256,native256_chain,shader,chain_shader,gpu_chain andfinal_gpu64/
final_gpu256. Initialstandalone shader runs andfinal source replays are kept
separately. `compile_base16.ps1 -Reductions` emits the isolated reduction
shader;without that switch it still reproduces the accepted base16 SHA256
64F99AC7EF8242219A13402416549BF94FD7BED991AC02F2E3BF2C1EE264CEDC.

## 128 masking and scalar reductions

The unchanged accepted map ABI13 shader is used directly through the diagnostic
runner. It is not routed through/widening MapGPU's guarded128 API. Raw masking
compares min(abs(ref/norm),abs(test/norm)) and3x3 zero-padded native conv2.
Sixteen cases include zero,constants,edge impulses,structured signed values,
random scales andnormalizations1/2/4/8/16. Pooling matches native128 arithmetic,
including border pixels.

Pupil references call native geomean andofficial3.07 pupil_d_unified forfive
luminance patterns (constants1e-3/10/1e4,HDR ramp,seeded log-random),30/60ppd
andages24/60/60.001. Allfour trace values are checked:geometric mean,two
diameters,andreduction. Quality pooling checks sixteen separate128-square
difference/mask pairs against native sum(abs(D.*mask).^0.8/numel(D)).^1.25.
This does not validate final Q/Q_JOD or128 spectral/JND/SPYR composition.

Evidence:`validation/mask128_639281816218927514` and
`validation/scalar128_639281817346881631`. Native generators are
generate_mask128_reference.m andgenerate_scalar128_reference.m.

## Resize and the Q16 failure

Float32 resize uses existing CPU axis plans andunchanged map modes0/1,checking
native image-package imresize withbicubic,antialiasing andsymmetric edges.
Ten transitions,eight inputs each:128→128/64/32/16/8,64→128,32→64,16→32,
8→16 and16→128. Inputs include zero,constant,twoedge impulses,checkerboard,
ramp andrandom scales. Evidence:`validation/resize_float128_639281818909600345`.

Binary64 comparison-mask resize initially failed at128→8:the existing Q16
packing cannot represent allaxis coefficients exactly. The diagnostic rejected
the input plan before that dispatch;the oldshader was not silently rounded.
`validation/resize_mask128_639281818201889156/gpu.log` andthe partialgpu directory
retain this failed trial. The four completedzero-mask outputs before rejection
are not counted asacomplete Q16 pass.

Aseparate experimental ABI141 shader uses Q20 coefficients andsignedQ40
accumulation/conversion tobinary64 word pairs. CPU checks require exactinteger
coefficients,unit normalization andabsolute weight sums<=2. Horizontal sums
fit signed32 bits;vertical products/sums stay within signed64 representation
andthe53-bit binary64 significand. Shader arithmetic uses32-bit words and
umul/imulExtended,withnoFloat64/Int64 capabilities. This is not the accepted
MapGPU ABI13 binary andmustnotbe substituted underits existing contract.

Eight binary masks (zero,ones,corner impulses,checkerboard,structured andsparse/
dense random) resized from128 to128/64/32/16/8 match all174,592 native binary64
words. Accepted evidence isgpu_q20/gpu_q20.log andq20_shader inthat checkpoint.
Use generate_mask_resize128_shader.ps1 andthechecker argument20 together.

The existing accepted map shader reproduces unchanged SHA256
0B023987B9A2C9267B8979A072574DCC04AD8F56EA56CB1B7AACF164EA041BFF.
No oldshader/source/reference was overwritten.

## Reproduction and next integration

Build fft256-filter-test fromtests/hdrvdp3_fft256. The native generators create
fresh directories;thechecker likewise rejects existing output directories.

```text
fft256-filter-test --base-reductions native reductions.spv fresh-output 256
fft256-filter-test --base-reductions-chain native reductions.spv base16.spv fresh-output
fft256-filter-test --mask128 native accepted-map.spv fresh-output
fft256-filter-test --pupil128 native accepted-map.spv fresh-output
fft256-filter-test --quality128 native accepted-map.spv fresh-output
fft256-filter-test --resize-float128 native accepted-map.spv fresh-output
fft256-filter-test --resize-mask128 native mask_q20.spv fresh-output 20
```

Next:integrate these paths with explicit shader ABI/layout contracts inMapGPU,
including256-sample DC/base adaptation,16x16 FFT/selection andQ20 mask resize.
Combine validated forward/inverseFFT256 forLocalAdaptFFT's8/9 and3/4 dispatch
graphs. Apply official ppd-limited SPYR height tobothcalls andband-count
metadata (128@30ppd:height3,residual16;128@60ppd:height4,residual8).
Then run native full-stage/score regressions throughanisolated128 diagnostic
entrypoint. Keep public API guards32/64 untilthose regressions pass.
