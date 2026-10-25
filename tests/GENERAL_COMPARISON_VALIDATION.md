# General-shape binary comparison and double resize

Checkpoint `validation/general_comparison_639284533493306890`,2026-10-24.
Experimental only: production MapGPU shape guards and runtime assets unchanged.

`generate_general_comparison_reference.m` exports native Float32 reference/test
luminance, binary `abs(test-reference)./reference > .001` and double imresize
goldens. Shapes80x96/96x80 each have nine input pairs: the actual metric pair,
zeros/ones masks,corners,checkerboard,seeded random,stripes,and threshold-adjacent
single values. Five band targets per pair include identity twice,half,quarter
and eighth sizes. Actual-pair resize goldens are asserted bitwise against
the saved full-metric trace.masking comparison maps. `native_verified/` uses
the original .001 comparison expression; earlier native files are preserved.

`general_comparison.comp` runs three GPU dispatches in one command buffer:
binary comparison,horizontal Q20 resize,vertical Q40 resize with direct IEEE
binary64 bit packing. There is no Float64/Int64 capability or host intermediate
read/compute/upload. CPU uploads only original luminance and axis plans; plans
occupy buffer suffixes while GPU writes intermediate masks into prefixes.
Input/output layouts are row-major uint32 words; double output stores low/high
words. No FFT padding or DC processing occurs; resize uses native bicubic
antialias/default symmetric extension through the existing resize axis planner.

Host validation requires exact Q20 weights,normalization1,absolute row weight
sum <=2 and scale ratios1,2,4,8. First signed accumulator magnitude <=2*2^20;
second <=4*2^40. These integer results fit the uint32 pair representation and
the exact53-bit double significand, so the double conversion needs no rounding.
The finite Float32 division helper is reused from the reference map shader;
explicit DenormPreserve/RTE controls and Vulkan1.2 validation apply.

`gpu/PASS.txt`,Intel Arc Pro Graphics: **90 cases,691200 binary word checks,
321840 double values,zero bit differences**. The binary calculation repeats
for all five targets; this is18 unique original pairs/138240 unique input mask
positions, including the same two actual image pairs used before. Identity
targets also repeat, so test counts are not claims of unique coverage.

Fresh strict MSVC Release target `general-comparison-test` leaves all earlier
binaries intact. The shader and its helper source,axis planner,native fixtures
and checker are pinned by the checkpoint hash manifest. No numerical trial
failed. Public resize_comparison_mask remains restricted to its previous shapes.

```powershell
octave-cli --quiet --no-gui tests/generate_general_comparison_reference.m <fresh native> <full masking trace>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_general_comparison.ps1 -OutputDirectory <fresh shader>
cmake --build <fresh configured dimension build> --config Release --target general-comparison-test -j 4
general-comparison-test <shader>/general_comparison.spv <native> <fresh GPU output>
```

The masking checker additionally accepts these generated double-mask dumps as
its final argument. It requires exact native double bit identity before using
the GPU-derived mask in D_p and band-error calculations. This is file-based
component composition with host transfers, not a fully resident image-to-score
GPU evaluation. Native self masks,CSF and base neural noise remain inputs.

`gpu_masking_with_comparison/PASS.txt` confirms the composed test:82 checks,
250334 values with zero numerical and observed bit differences. This includes
35760 exact double mask bit comparisons plus the prior masking/D/D_p/band-error
and Q/Q_JOD checks. Two actual pairs retain their native scores. Earlier
native-input masking executables are preserved; the newer checker lives in
this checkpoint's build/Release directory. The extra comparison repeats actual
mask outputs already validated above and is not additional unique GPU data.
