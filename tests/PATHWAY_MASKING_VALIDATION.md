# Actual-pathway masking and quality component checks

Checkpoint `validation/pathway_masking_639284292364400623`,2026-10-24,
Intel Arc Pro Graphics. No public shape guard or production algorithm changed.
These are synchronous component checks using independent native intermediate
inputs; they are not complete image-to-score GPU quality acceptance.

`generate_pathway_masking_reference.m` uses the full metric traces in
`general_masking_trace_639284290290539478`. It exports per-band self/neighbor
masks,N_mask,excitation,neural noise,D,double comparison mask,D_p,parameters,
psychometric maps and band errors. The native formula for each band error is
the reference's `sum(abs(D_p(:)).^.8/numel(D_p)).^(1/.8)`; reconstruction of
accumulated errors must exactly match the traced native accumulator. Initial
generation referenced the private minkowski_sum function and failed; the
corrected explicit formula writes a fresh `native_verified/` directory.
Old partial files are retained. MAT trace precision remains authoritative.

`pathway-masking-test` links the unchanged MapGPU,resize,CSF and quality
implementations in an isolated strict MSVC Release target. It reuses bounded
ABI13 `publish_prepare_639283319336040304/runtime_bounded/map.spv`. It validates
five bands for each actual80x96/96x80 pair at30ppd/age24:

- Neighbor masks: GPU resize from independent adjacent native self masks,
  then clamp/add with the native neighbor eligibility rules.
- N_mask from native self and GPU neighbor, using traced q and scales.
- Test-minus-reference from the already normalized/DC-centered native bands.
- D from GPU band difference and N_mask, native CSF sensitivity, and the
  base band's native neural-noise scalar transformed on CPU as in the existing
  implementation. Nonbase neural noise is evaluated by the map shader.
- Psychometric maps, band quality errors and final Q/Q_JOD.

The first run `gpu/PASS.txt` covers62 checks/178814 values with zero numerical
and observed bit differences. Initial build was attempted before configure
finished and failed; after configure completed the Release build passed.
There was no numerical masking failure.

Optional fourth checker argument adds an isolated direct D_p export kernel,
`pathway_mask_product.comp`. Its input is GPU-computed D downloaded by the
synchronous map API, plus the native comparison mask converted to Float32.
The kernel uses a precise Float32 product, explicit DenormPreserve/RTE and
Vulkan1.2 SPIR-V validation. Its output is compared directly to native D_p.
It is a separate dispatch/upload, not a fully resident masking pipeline.

Final `gpu_with_Dp/PASS.txt`/`results.tsv`: **72 checks,214574 Float32 values
numerically exact**, with observed bit differences also zero. This comprises
35760 values each for neighbor,N_mask,band difference,D,psychometric,D_p,
plus10 band errors and4 scores. Signed-zero bits are reported separately,
not required by this checker's numerical acceptance. Native self,N_mask,D,D_p
are single; comparison masks are double; original parameter precision is
retained until the same conversions used by MapGPU.

The original62-check executable is preserved as `masking_first_pass.exe`.
All generated inputs, outputs, original and final binaries, shaders and
source dependencies are hashed in the checkpoint. No broad unchanged suite
was repeated: the72-check run includes every prior62 comparison plus D_p.

```powershell
octave-cli --quiet --no-gui tests/generate_pathway_masking_reference.m <fresh native> <full masking trace>
cmake -S tests/hdrvdp3_dimensions -B <fresh build> -G "Visual Studio 18 2026" -A x64
cmake --build <build> --config Release --target pathway-masking-test -j 4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_pathway_mask_product.ps1 -OutputDirectory <fresh product shader>
pathway-masking-test <bounded ABI13 map.spv> <native> <fresh GPU> <product shader>/pathway_mask_product.spv
```

Missing end-to-end work: GPU generation of comparison masks and their double
resize for these dimensions; joining accepted GPU SPYR/pooling/resize/CSF
components; upstream GPU visual-pathway production of P and adaptation; and
independent full image-to-score comparisons before widening public support.
Matching scores here still depends on native comparison masks,CSF,self masks
and the native base-noise scalar, so does not remove those integration steps.
