# Double96 hybrid GPU CSF maps

Finalized2026-10-30. Artifacts:`validation/double96_csf_completed_path.txt`.

Runtimev8 moves bicubic log-adaptation resize,[-5,5] clamp,actual-grid CSF
lookup/interpolation,neural-noise reciprocal,and base-log Neumaier mean to
GPU. CPU generates the256-node model table/grid/slopes and retains the
base-band pow10/compute_ncsf scalar calculation. GPU broadcasts that scalar
noise,including valid+Inf. Initial mean-log preparation remains CPUlog10.
This is GPU map processing with explicit CPU model/nonlinear boundaries,
not a fullGPU CSF/nonlinear metric claim.

csf_log_luminance_grid exposes a copy of the existing actual pow/log-rounded
grid;the original CPU sampler is unchanged. The GPU uses upper_bound on those
Double nodes,not a theoretical uniform grid,and separate multiply/add.
Reciprocal uses tested FMA residual correction. Base-band mean is a serial
column-order Neumaier reduction with corrected divisor144;CPU receives the
mean for pow10/model evaluation before GPU noise fill. API explicitly accepts
positiveInf noise and rejects NaN/nonpositive noise,matching identity behavior.

Production optional14th pipeline andCSF callback are used bypublicCLI v8.
V8requires20payloads(14SPV+6model);v1-v7 retain priorCPU CSF behavior.
-GPUCSF includes earlierGPU stages;default packagebuild remainsv1.

Results:
-579,312 native GPU-returned map values matchbitwise across9configurations/
 five bands;11,520 CPU model-table values and9CPU base-luminance scalars match.
-321,840 GPU-returned control values,6,400 CPU table values and5CPU scalars
 match forlower/upperclamp,all256grid nodes,sparse/border inputs,ramps and
 zero base-frequency with+Infnoise.
-Total919,086 comparisons:901,152 returned-map values,17,920 CPU table values,
 14CPU scalars. Base-band noise entries are GPU copies of a CPU model scalar;
 these counts do not imply all arithmetic ran onGPU.
-8invalid-stage cases,13CLI cases and12runtime-package cases reject.
-9publicCLI configs match72quality Double scalars and all497,664 input words.
-v1-v7OLEDstress regressions each preserve3numeric/input exports exactly.
-The oldFloat32 BT.2020OLED route was rerun becausecsf.h/cpp are shared;
 all16exports remain byte-identical. Allaccepted Vulkan logs are clean.

The corpus is the same2distortedpairs+identity and3spectra,96sq/30ppd/age24/
qualitydefaultsp0. Coverage overlaps priorcheckpoints;NVIDIA RTX A500 with
Float64/RTE64/signedzero64,no denorm64 guarantee. MainRelease/stage builds pass.
Existing unrelated compile warnings are preserved.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUCSF
```

CMake targetdouble96-csf-test accepts<v8-runtime> <native-cpu-root> <fresh-out>.
Audit:node tests/hdrvdp3_evaluation/audit_double96_csf.cjs <root>. RuntimePROFILE
metadata clarified afterruns;payloads unchanged andinventoryrefreshed.
No oldreference/shaders_out changes. CPU display/calibration/padding/log/exp/
pow andnonlinear quality/pooling remain;wholeGPU/generalHDR-VDP-3 unfinished.
