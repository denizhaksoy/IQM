# Double96 GPU comparison masks

Validated2026-10-29. Artifacts:
`validation/double96_comparison_completed_path.txt`.

Runtimev6 adds GPU relative-RGB comparison decisions and bicubic resizing.
For every96sq pixel,it evaluates any channel abs(test-reference)/reference
strictly greater than Double0.001. The accepted FMA remainder correction
preserves tested threshold decisions. Zero denominators are handled explicitly:
positive/positivezero is true,zero/zero is false,positive/negativezero is false.
Overflowing positive quotients returntrue without feedingInf into the FMA
correction. Doublezero sign is inspected using32bit unpacking;no shaderInt64
feature is introduced.

The96sq binary mask stays onGPU. Existing Doublebicubic shader and CPU
source-generated axis plans resize it to96/96/48/24/12. Ratios1/.5/.25/.125
have exactly1normalization in the supported profile;the backend checks this.
No CPU mask intermediate and no clamp after resizing:negative/ringing values
are preserved exactly as the native comparison map requires.

The production API has an optional comparison callback. It checks finite
output andshape but intentionallydoes not require nonnegative values.
The backend owns an optional12th pipeline;publicCLI records GPU comparison
selected. V6requires18payloads(12SPV+6model files);older v1-v5 continue to use
their prior CPU comparison path. Defaultpackagebuild remainsv1.

Results:
-193,104 official native Double comparison-map values across9configurations/
 five bands matchbitwise;all45saved maps byte-match afterlayoutconversion.
-85,824 CPU-oracle control comparisons match for threshold-adjacent inputs,
 +/-zero denominators,overflow quotients,all-channel decisions,sparse/smooth
 patterns andadditional.25/.125 downsampling. These are decision/map tests,
 notproof of general correctlyrounded GPUdivision or arbitrary subnormals.
-5stage-invalid cases reject(band,shape,negative,NaN,Inf).
-9PFM CLIv6 configurations matchall72quality Double scalars anddecoded inputs;
 the13existingCLI invalid-case checks pass.
-10malformedv6package cases reject beforeGPU creation.
-V1-v5OLEDstress regressions eachretain byte-identical quality andtwoinput
 exports. Allaccepted Vulkanlogs areclean;mainRelease/stagebuilds pass.

The same two distorted pairs+identity andthree spectra are reused. Scope
96sq/30ppd/age24/qualitydefaultsp0,NVIDIA RTX A500 Float64/RTE64/signedzero64;
DenormPreserve64 is unavailable. Counts overlap previous profile coverage.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUComparison
```

The flag includes allpriorGPU stages. Accepted package is final_pack/
runtime_repro/runtime;initial accepted_pack build predates signedzero handling
and was superseded before numerical acceptance. Oldartifactspreserved.
PROFILEmetadata clarified after runs;payloads unchanged,inventoryrefreshed.
CMake targetdouble96-comparison-test accepts<v6-runtime> <native-cpu-root>
<fresh-output>. Audit:node tests/hdrvdp3_evaluation/audit_double96_comparison.cjs
<root>. No oldreferences orshaders_out edits.

RemainingCPU work:display preparation/calibration/padding,log/exp,JND,CSF,
nonlinearquality/pooling. Next:JND/pupil correction and LUTinterpolation,
keeping Doublelog/exp queries onCPU until an exactGPU math path is verified.
Thefixed-profile hybridCLI is usable;fullGPU/generalHDR-VDP-3 remains open.
