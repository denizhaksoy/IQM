# Experimental image-to-quality GPU composition

2026-10-24: the80x96/96x80 localized-distortion fixtures now pass an assembled
image-to-Q/Q_JOD path. Fresh complete replay:
`validation/actual_gpu_replay_639284715589047637/PASS.txt`.
Initial staged assembly is retained in
`validation/actual_gpu_composition_639284565879130482`.

This is an experimental,file/host-composed validation path, not production
API integration or a new public geometry guarantee. It uses single positive
luminance,30ppd,age24,D65 andsp0Filters. It does not assert detection-map parity.

The replay starts with original reference/test luminance and fixed model data:

1. GPU symmetric2H x2W padding ->forward mixed FFT ->MTF multiply ->normalized
   inverse ->crop/clamp produces retinal luminance.
2. GPU L+M preparation feeds GPU replicate padding/log ->forward FFT ->Gaussian
   multiply ->normalized inverse ->crop/exp, producing local adaptation.
3. GPU pupil/geomean,JND interpolation and P response,plus mean/log adaptation.
4. GPU all SPYR bands,DC removal,residual difference/FFT/magnitude/index.
5. GPU mutual mask/pool,comparison binary/double resize,neighbor/CSF/masking,
   D/D_p,band errors and final quality scores.

Image-dependent native retinal,adaptation,P,log,bands,self masks,comparison
masks,CSF and base-noise maps are comparison goldens only in the final replay.
Native lookup tables/matrix are fixed model data, with hashes verified identical
across all four images. CPU work includes model/axis plans,double NCSF using
GPU-derived frequency/adaptation,serialization and validation. Components
exchange files and synchronous host buffers; this is not one resident command
buffer. Every independent native reference and earlier binary is preserved.

## Checkpoints

- `pathway_jnd_639284560688608036`: initial48checks/368640 values; GPU pupil
  version52/368656; final GPU retinal/adaptation version60/430096. All pass
  numerically with observed bit differences zero. Native pupil geomean uses
  sequential Float32 log accumulation, not the base120 double-sum mean contract.
  Existing map ABI13 already executes this operation on7680 pixels; no new
  mean shader or public guard change was needed.
- `actual_pathway_filters_639284562227581717`: both orientations,reference/test,
  MTF/LA: eight253440-value stage checks pass (2027520 comparisons). Four LA
  reruns also pass with GPU-generated adaptation input. IO/log/exp/exact-product
  shader is new and targeted; accepted mixed FFT shaders remain unchanged.
  Each filter chain is seven GPU dispatches, no intermediate host computation.
- Final replay reruns all required stages in fresh directories. SPYR113526,
  pool71520,comparison90cases (691200 binary word checks and321840 double
  outputs),quality126checks/393384 comparisons all pass. These counts repeat
  intermediate values; they are not unique input coverage counts.

Native stock/traced scores and all intermediate goldens remain pinned. Final
independent validation confirms original filter/comparison inputs bit-identical,
fixed model tables invariant,and Q/Q_JOD **bitwise** identical:

| Shape | Q | Q_JOD |
|---|---:|---:|
|80x96|4.030489921569824|4.869779586791992|
|96x80|3.958810567855835|4.790723323822022|

Device:Intel Arc Pro Graphics. Shader Float32 DenormPreserve/RTE applies where
required; no Float64/Int64 shader capability introduced. FFT/convolution stage
acceptance remains numerical; observed zero bit errors do not prove arbitrary
signed-zero parity. Double masks/frequency grids and final scores have explicit
bit checks. Padding/layout/normalization are recorded in each component's
metadata; inverse normalization is integer30720 division. No power-of-two pad.

## Replay

The assets manifest records existing accepted binaries,shaders,model inputs
and independent native goldens. The runner checks every process exit and uses
fresh output directories. It does not compile all shaders or overwrite runtime
assets. Example from the workspace root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_actual_gpu_composition.ps1 -OutputDirectory validation/my_fresh_replay -AssetsManifest validation/actual_gpu_composition_639284565879130482/assets.json
```

Artifacts include per-stage logs/dumps,result tables and `validated/results.json`.
An initial Octave copyfile attempt triggered Windows xcopy's file/directory
prompt. The serializer now copies bytes directly; failed `inputs/` remains,
accepted assembly uses `inputs_verified/`. No numeric failure occurred in the
asymmetric assembly/replay. All native installations remain untouched.

## CPU model filter validation

`validation/cpu_model_filters_639284717818610970` generates MTF/Gaussian
coefficients in C++ instead of uploading native exported filter values. Native
coefficients are strict bitwise goldens only. Four orientation/mode cases each
match30720 coefficient words bitwise and253440 GPU stage values numerically.
This is122880 coefficient checks,not four different filter algorithms. Original
filter executables are retained; new binary/source are independently recorded.
The HDR stress replay uses this CPU-generated-filter executable.

Public80x96/96x80 evaluation is still disabled. Remaining product work includes
integrating the experimental components into the supported runtime/API,broader
content/precision/age/ppd validation,and reducing file/host transfers while
preserving exact arithmetic. Do not treat a tolerance or a component pass as
general HDR-VDP-3 parity.

## Content stress

`validation/actual_content_stress_639284716222632999` adds impulse and HDR-ramp
references via the native generator's explicit `--pattern` option, preserving
the original default. HDR reference luminance spans1e-2..1e3. All three contents
use the same localized test transform and both orientations: six image pairs.
Native original/traced scores are asserted for every new fixture. Model matrix
and JND tables are independently verified identical across all six pairs.

Impulse replay and HDR replay with C++ model filters both pass every stage
and final score bits. Native fixtures/intermediate values are not execution
inputs beyond original images and fixed model data. This expands content
coverage,not viewing parameters or a general quality guarantee.

| Content | Shape | Q | Q_JOD |
|---|---|---:|---:|
|Impulse|80x96|2.664517402648926|3.319782257080078|
|Impulse|96x80|2.575063705444336|3.215233325958252|
|HDR ramp|80x96|7.378299713134766|8.212310791015625|
|HDR ramp|96x80|7.446496009826660|8.271669387817383|

`tests/summarize_actual_gpu_replays.ps1` verifies all stage PASS markers,
original-input/model checks and actual score dump bits against the independent
native validation records before producing the six-pair summary.

## Production JND operation order (2026-10-25)

The previous C++ model produced60 different Float32 rod interpolation slopes.
The production implementation now follows native symmetric logspace,
log10(luminance) coordinates, per-node Jacobian and cumtrapz prefix order.
Strict FP compilation includes jnd.cpp. The production model checker with
--strict-double verifies10238 Float32 node/slope words and6144 double table
words, all bitwise identical for the default model on this platform.
Evidence: validation/cpu_jnd_model_639284722715977911/production_strict_double_verified.
The existing CLI regression preserves11 PFM pairs and1166 numerical exports,
including exact quality files: validation/production_jnd_order_639284726753249859/cli_regression.
This does not expand viewing parameters, hardware coverage or public shapes.

The strict-double negative control changes one binary64 rod node by one ULP
without changing its binary32 representation. Float32 checks pass and strict
double checks reject it (one mismatch). Official reference files are untouched.


The production-linked CPU-model JND checker now additionally passes asymmetric
and impulse on both orientations (64checks/471060 values per content). Together
with the completed HDR replay this covers all three content fixtures for the
production JND model, while only HDR has been rerun end to end in CPU-LUT mode.
Artifacts: production_frequency_639285092485366527/jnd_asymmetric and /jnd_impulse;
production_cpu_jnd_replay_639285090794799166/replay for HDR. Earlier six complete
CPU-filter replays used native fixed JND model tables as recorded in manifests.
The CPU-LUT profile now reports GPU_adaptation=1 and CPU_model=1 explicitly;
old argc==7 metadata incorrectly displayed0 for argc8 despite the correct data
path. Numerical results and old artifacts are preserved.

## Reusable filter runtime extraction (2026-10-25)

`run_mixed_filter` now owns the seven-dispatch MTF/LA implementation; native
fixture loading remains in the independent checker. The24-case component
matrix passes6,082,560 stage values and737,280 runtime-generated model words
bitwise. Same-instance repeated calls with trace disabled also match.
The bounded evaluation library builds with the new component. See
[MIXED_FILTER_RUNTIME_VALIDATION.md](MIXED_FILTER_RUNTIME_VALIDATION.md).
Public geometry support is unchanged; the remaining mixed SPYR/residual/pool/
map runtime integration is still required before full API evaluation can use it.

The SPYR/all-chain stage is now also available as `run_pathway_spyr`, using
in-memory P pairs,explicit fixed sp0 filter words and shader assets. Six paired
component chains match340578 values bitwise; a fresh HDR end-to-end replay with
both extracted filter/SPYR runtimes preserves native final score bits. See
[SPYR_RUNTIME_VALIDATION.md](SPYR_RUNTIME_VALIDATION.md). Runtime validates its
actual GPU base before mean/DC,using two synchronized submissions. This does
not enable general public geometries or remove downstream host/file transfers.

Pooling now also has an in-memory `run_pathway_pool` entry point. Thirty band
cases match214560 words bitwise,and a fresh HDR replay using extracted filters,
SPYR andpooling preserves final native score bits in both orientations. See
[POOL_RUNTIME_VALIDATION.md](POOL_RUNTIME_VALIDATION.md). Resize/CSF remain on
existing production MapGPU. Comparison/masking runtime composition and public
geometry integration remain unfinished.

Comparison and masking/base-noise/quality now also have in-memory entry points:
`run_pathway_comparison` and `run_pathway_masking`. Native files are checker
inputs only; default masking parameters are computed in C++ and independently
verified double-bit exact. Comparison270cases and masking378stage/input checks
across three contents pass strict bit audits. See COMPARISON_RUNTIME_VALIDATION.md
and MASKING_RUNTIME_VALIDATION.md. A fresh HDR replay using all extracted
runtimes preserves independent native Q/Q_JOD bits in both orientations.
A single original-image-to-score in-memory API remains to be wired and tested;
public geometry guards remain unchanged.

The single in-memory `evaluate_pathway_quality` API is now independently
validated from original images on all six pairs:487 stage checks/1966412words
bitwise exact,including one score-only same-instance repeat. Thirteen malformed
input checks reject before GPU access. See PATHWAY_QUALITY_API_VALIDATION.md.
This supersedes the earlier statement that single-call composition was still
pending;historical file-replay results retain their original scopes. Public
CLI runtime packaging and geometry integration remain open.
