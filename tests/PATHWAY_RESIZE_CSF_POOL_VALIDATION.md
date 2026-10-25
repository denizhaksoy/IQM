# Actual-pathway resize, CSF and mutual pooling

Checkpoint: `validation/pathway_resize_csf_639284286981781158`,2026-10-24.
Device: Intel Arc Pro Graphics. Public shape guards and production shaders
remain unchanged. This is component validation, not full quality acceptance.

The native generator `generate_pathway_resize_csf_reference.m` loads the two
actual full-pathway traces (80x96/96x80,30ppd,age24). It verifies imresize plus
clamp[-5,5] against each of five traced resized log maps, then exports native
single logs, resized logs and CSF sensitivities. Targets include the original
shape twice, half, quarter and eighth dimensions. Native bicubic/antialias
defaults use symmetric edges. Serialization is row-major decimal17digits;
no FFT padding or DC removal occurs here.

The existing verified `IQM-map-gpu-test.exe` from
`validation/full_age_639280300414897968/verified_bin` and existing bounded
ABI13 shader from `validation/publish_prepare_639283319336040304/runtime_bounded`
already support these resize/CSF operations. No new runtime profile or source
change is needed. `gpu_resize.log`:10cases,35760 values,zero numerical errors.
`gpu_resize_csf.log`:20comparisons,71520 values (resized maps plus CSF),zero
numerical errors. This second run repeats the resized values; do not count
them as new unique coverage. Each resize axis is a separate MapGPU call;
this checkpoint does not claim a joined SPYR/resize/CSF command buffer.
The last band's generic CSF map is traced too, but actual base noise uses
the separately selected dominant frequency, not that generic map.

Native `native_with_pool/` additionally exports mutual mask raw values
`min(abs(test_band),abs(reference_band))` and `conv2(raw,ones(3)/9,'same')`.
Options assert masking_norm0,pool3 and no Gaussian/dilation alternatives.
Bands are already normalized and the base is already DC-centered. Resize and
CSF fixture hashes are identical to the original `native/` files.

The production mutual_masking API correctly rejects these unvalidated shapes
(`gpu_pool.log`,exit2). Rather than widen its guard, new isolated
`pathway-pool-test`/`pathway_pool.comp` operate on native band inputs. Only
normalization1 and these eight band geometries are allowed by the checker.
Both raw masks and pooled outputs are computed on GPU; pooled samples use the
original uploaded bands directly, with no intermediate CPU calculation.
Out-of-image taps are skipped (zero padding), with native longer-axis tap
traversal. Two variants partition writes; per-tap FMA selection uses source
coordinates within32-element blocks. The fused operation reuses the existing
integer positive_fma/shift_jam/round_word helpers from the reference map shader,
injected and source-hashed by `compile_pathway_pool.ps1`. No Float64/Int64.
Float32 DenormPreserve/RTE and SPIR-V validation are explicit.

Accepted `gpu_pool_integer/PASS.txt` andresults.tsv:10bands,71520 outputs,
zero numerical and observed bit differences. Arithmetic dispatch is empirically
matched on these actual inputs; this does not prove the installed native
compiler's schedule for all shapes, inputs or platforms.

Failed exploratory runs remain intact: destination-wide FMA/tail selection
first passed the two80x96 bands but differed11values on40x48, at x31/32.
Per-tap selection with mixed hardware fma/separate arithmetic then differed
2494values on the first band. Replacing the fused operation with the accepted
integer helper gives the exact accepted pass. No tolerance was introduced.

```powershell
octave-cli --quiet --no-gui tests/generate_pathway_resize_csf_reference.m <fresh native> <full pathway checkpoint>
IQM-map-gpu-test <bounded ABI13 map.spv> <native> --resize
IQM-map-gpu-test <bounded ABI13 map.spv> <native> --bounds
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_pathway_pool.ps1 -OutputDirectory <fresh first shader>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_pathway_pool.ps1 -OutputDirectory <fresh second shader> -NonFma
pathway-pool-test <first shader>/pathway_pool.spv <second shader>/pathway_pool.spv <native>/pool.txt <fresh GPU>
```

Complete masking denominator, neighbor resizing, excitation/psychometric maps,
quality pooling and upstream GPU P production still require general-shape
validation/integration. The new pool checker remains separate from the accepted
SPYR/residual chain. Native inputs are reused from the same two image pairs.

An additional full native masking trace is saved in
`validation/general_masking_trace_639284290290539478`. The instrumented copy
now records self/neighbor masks, N_mask, excitation, neural noise, D, the
double comparison mask, D_p and accumulated quality error. Both original and
instrumented calls retain exact Q/Q_JOD identity; the installed reference is
unchanged. Native MAT files preserve all stage classes.

`validate_pathway_pool_trace.m` compares the experimental GPU pooled dumps
directly against this full metric's self masks, independently of the earlier
formula fixtures. `full_trace_pool_comparison/PASS.txt` confirms **35760 words
bitwise exact** over10bands. This is a second comparison of the same GPU
outputs, not new unique GPU values. The remaining masking maps are reference
artifacts for the next stage; they are not yet GPU-validated.
