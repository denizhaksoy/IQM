# IQM HDR-VDP-3 session handoff

Last updated: 2026-10-17 (Europe/Prague). Branch: `feature/hdr-vdp3`.

## Current checkpoint: ABI8 maps and ABI81 native-matching base FFT

This section supersedes the ABI7 checkpoint below. This publication contains
English documentation and a portable summary only. Implementation, test tools,
generated shaders and full logs remain local/uncommitted.

- Ten fresh 64x64/30ppd/age24/D65 synthetic pairs match official v3.0.7
  Q/Q_JOD exactly. Independent GPU checks: 71 comparisons /227984 values per
  pair, 2279840 values total, zero differences; native base index/frequency
  assertions also pass. See tests/MAP_GPU_VALIDATION.md and the summary JSON.
- GPU preparation now includes guarded64-sample residual DC, separately
  normalized band subtraction and original binary comparison mask. Optional
  ABI81 computes the native-matching8x8 base FFT; magnitude/index/frequency
  selection still runs on CPU. No shaderFloat64 is required.
- Local accepted checkpoint: validation/map_gpu_abi8_639278691970167183/.
  Combined suites: quality_gpu_base_fft_suite_639278697831326240 and
  quality_hdr_gpu_base_fft_suite_639278700680928595 under validation/.
- Short publication checks: two fresh localized repeats each match53 numeric
  files from the accepted export by SHA256. Fresh localized and identity scores
  match official Octave exactly; identity band/total errors are0 and Q/Q_JOD10.
  Three incompatible shader configurations reject with exit2. No Vulkan errors
  or VUIDs occur in the fresh pipeline logs. These were sequential runs.
  Evidence: validation/abi8_quick_checks_639278706524535142/.
- DC domain: exactly64 samples; finite nonzero exponents[-90,100], span<=16,
  zeros allowed. Comparison-mask reference must be positive finite. Do not
  extrapolate this validation to black/exceptional inputs or arbitrary sizes.
- Remaining CPU: calibration/plans, base hypot/first-max/physical frequency,
  base geometric adaptation/CSF/noise/scalar double power, double bicubic mask
  resize and diagnostics/readbacks. Next: independently validate remaining
  preparation ports, then dimensions/viewing/color/real HDR generalization,
  a usable public metric API and other required output modes.
- Preserve staged corrdn-test shader and dirty lib/VkFFT. Publish implementation
  separately after reviewing mixed changes. Update this note at publication.

## Historical checkpoint (superseded): ABI7 GPU quality validation

This section supersedes the historical progress notes below. This publication
contains documentation and a compact validation summary only. The implementation,
new test tools, generated shaders and full local logs remain uncommitted/local;
checking out this documentation commit does not reproduce the ABI7 implementation.
See `tests/MAP_GPU_VALIDATION.md` and `tests/HDRVDP3_VALIDATION_SUMMARY.json`.

- All ten fresh synthetic pairs match official HDR-VDP-3 v3.0.7 Q/Q_JOD exactly.
  Scope:64x64,30ppd,age24,D65 luminance; local Octave11.3 and Intel Arc Pro Vulkan.
- GPU now evaluates adaptation input/mean/log10, pupil, all13 JND stages,
  resize/CSF, raw/pool/neighbor masking, N_mask, both D transforms, quality
  reduction and scores. CPU remains responsible for calibration/resize plans,
  residual DC, normalized difference preparation, base-frequency/adaptation/noise
  preparation and the original/double-resized comparison mask.
- Ten fresh exports pass2140960 independent GPU intermediate comparisons with
  zero differences. JND knot checks pass258048 values using native/production LUTs.
  Power, log10, resize, CSF, pool and clipping fixtures also pass. No tolerance
  substitution or Vulkan validation errors were accepted.
- Final shader: `validation/map_gpu_abi7_639278683626481099/map.spv`.
  Read `validation/map_gpu_final_shader_path.txt` and
  `validation/map_gpu_completed_path.txt` for current local pointers. ABI7 rejects
  older map shaders. Generate from `tests/generate_power_shaders.ps1`; do not
  compile the map template directly. Generated math does not read Windows DLLs
  or require shaderFloat64. Default CPU maps remain available and verified.
- Baseline: `validation/quality_gpu_maps_suite_639278684479473340/PASS.txt` plus
  `GPU_INTERMEDIATES_PASS.txt`. HDR:
  `validation/quality_hdr_gpu_maps_suite_639278686318506540/PASS.txt`.
  Full logs, sources and binary hashes remain local; the committed summary
  records their identifiers/hashes and scores without uploading large artifacts.
- Next: finish the remaining CPU preparation ports with independent native
  fixtures, generalize dimensions/viewing conditions and real HDR/color inputs,
  then expose a usable metric API and implement other required output modes.
  This is not a completed general-purpose implementation or a percentage claim.

Preserve the existing staged corrdn-test shader and dirty `lib/VkFFT`. Publish
implementation separately only after reviewing the mixed local changes. Keep
commit messages and published documentation in English. Per the user's latest
instruction, update this handoff when publishing to GitHub, not every small step.

## Objective and constraints

Reimplement official HDR-VDP-3 v3.0.7 in C++/Vulkan, prioritizing numerical
and algorithmic parity over speed. Preserve all existing changes, staged
files, untracked artifacts and dirty `lib/VkFFT`. No resets/cleaning. User
authorized publishing verified changes; the scoped test checkpoint below is
committed/pushed. Other unfinished work remains local.
The earlier every-session handoff request was superseded by updates on GitHub publication.

Original handoff read from `C:/Users/DenizAksoy/Downloads/AGENTS.md`.
Official local reference: `C:/HDR-VDP-3_Ref` (`hdrvdp_version()` reports 3.07).
Do not edit official reference files or overwrite old `C:/temp` references.

## Validated so far

- SPYR (`sp0Filters`, reflect1, height 3) has five bands for 64x64:
  64x64, 64x64, 32x32, 16x16, 8x8 = 9536 values per image.
- Three patterns (diagonal modulo, asymmetric modulo, edge/off-center impulses),
  each with reference and 5%-brighter test images: all shared-JND SPYR values
  match Octave single exactly, including raw and DC-removed residuals.
- `corrDn` requires reverse column tap order. Standard shader uses explicit
  FMA; level 2 (16x16 in this harness) uses separate multiply/add. This is
  validated for the current size/Octave build, not arbitrary dimensions/MEX.
- Native single pupil/LMSR/JND is implemented in `src/hdrvdp3/jnd_single.cpp`,
  compiled with `/fp:strict`. On identical GPU adaptation maps, all six images'
  pupil coefficients, 13 per-pixel intermediate values and final JND match.
  The double API remains. Spectral constants apply to age-24 D65 luminance.
- Separate docs: `tests/SPYR_VALIDATION.md`, `tests/JND_PATH_VALIDATION.md`.

## Current preprocessing integration

- Harness: `bin/shared/wrappers/hdrvdp3.cpp`.
- Default `IQM_PREPROCESSING=none` preserves isolated MTF-off tests.
- Explicit `IQM_PREPROCESSING=fft` runs GPU optical MTF -> native single L+M
  adaptation input -> GPU FFT adaptation -> CPU native pupil/JND -> GPU SPYR.
- Optical MTF defaults to symmetric post-padding (`surround='none'`). The old
  standalone MTF test used replicate padding; its result is not default parity.
- Adaptation uses log/clamp, replicate post-padding, Gaussian FFT, crop/exp.
  Gaussian filter construction uses double then casts to float. The FFT path
  rejects sigma <4.3 (reference then uses spatial filtering).
- Experimental FFT preprocessing uses `LocalAdaptFFT`, complex float32.
  Default backend remains VkFFT; explicit `IQM_FFT_BACKEND=reference` loads
  the verified parity SPIR-V via `IQM_FFT_REFERENCE_SHADER`, requiring column
  axis order and native twiddle option. No VkFFT plan is created in this mode.
  Four optional trace snapshots: prepared, forward, filtered, inverse.
  Radial filters and buffers are fixed at 128x128; image input/output at 64x64.
- Every run writes a unique `validation/spyr_spatial_*` directory, with JND,
  luminance, adaptation, metadata, raw SPYR bands and optional FFT snapshots.
- SPYR exports now have explicit ref/test prefixes; fragile per-band counter
  naming was removed. Values are row-major; Octave band exports column-major.

## Full-chain baseline (before FFT axis-order experiment)

Full original-luminance comparison still fails exact parity. Maxima over each
reference/test pair, from `validation/fft_*_comparison.log`:

| Pattern | Retinal MTF max | Adaptation max | JND max | Raw residual max |
|---|---:|---:|---:|---:|
| Diagonal | 5.7220458984375e-6 | 8.58306884765625e-6 | 0.0003662109375 | 0.001953125 |
| Asymmetric | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.001953125 |
| Impulse | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.0029296875 |

Same-input JND/SPYR tests pass for all six FFT-preprocessed images. GPU runs
had no VUID/validation errors. Full-chain `--strict` test correctly fails.
Existing ramp FFT integration test passed its old tolerances only:
max=0.00016022, RMSE=0.00002903. See `tests/FFT_PREPROCESSING_VALIDATION.md`.

## Latest session: source-backed math passes; direct CSF age correction validated

- Replaced Windows binary/table dependency with original MIT-licensed AMD
  win-libm sources, commit6121d02aef0c775947d5dc655d7fc4ccfe2144ab:
  https://github.com/amd/win-libm. Preserved logf.asm, expf.asm and3 original
  table files with full notices in tests/math_reference/amd_win_libm/.
  SOURCES.json records LF-normalized hashes; generator verifies them. No DLL
  load/read required by tests/generate_reference_math_shaders.ps1.
- tests/math_reference/native_math.glsl uses the source FMA graphs and existing
  compensated helpers. A fresh seed exposed1 near1 log difference from relaxed
  GPU division. Finite compensated-division fix failed at1-eps(single); final
  normal-operand integer long division rounds ties-to-even exactly. Failed
  intermediate shaders/logs preserved, not parity passes.
- Final source-backed shader passes3 datasets,65536 pairs each:196608 tested
  pairs /393216 function evaluations, zero error. Includes repeated image pairs
  and distinct random seeds71439,20261017,121339; not all unique random samples.
  Scope log[1e-6,1e10], exp[-80,80]; no special/subnormal-log/overflow guarantee.
  Shader feature usage is float32/uint32 only; only current device/reference
  environment tested. Source-backed does not mean universal libm equality.
- Generator output validation/source_math_exact_division_639278581939061846:
  math/prepare/finish/multiply .comp/.spv. Tests check_*.log there plus
  validation/source_math_seed121339_final.log. Source generator's added hash
  check rerun: all4 .comp files byte-identical to tested final sources.
  Manifest regeneration under source_math_manifest_check_*.
- Fresh integrated source-math runs all pass strict stage checks, full
  preprocessing strict maps, native JND: validation/source_math_<pattern>_
  {run,stages,full,jnd}.log. Exports spyr_spatial_17922614797262557 (asymmetric),
  spyr_spatial_17922615112177470 (diagonal),17922615425028076 (impulse).
  No Vulkan validation errors. Main build source_math_csf_integration_build.log.
- CSF inspection found substantive AESL formula bug: old inner
  10^(slope*log2(...)) differs from official 10^slope * log2(...). Fixed in
  src/hdrvdp3/csf.cpp, also matching interpolation/reciprocal/power order.
  csf.cpp now gets strict FP/no contraction in main CMake, like jnd_single.cpp.
- New isolated IQM-csf-reference-test, tests/verify_csf_reference.cpp and
  generate_csf_reference.m:1152 paired queries across ages24/45/70, AESL on/off,
  12freqs and16lums. Before:320 float mismatches,444 double mismatches,
  max1.2169949371813404. After: **0 float mismatches**, but283 double differences
  remain, max2.2204460492503131e-16. --float-output passes exact float comparison;
  default strict double still exits1. Do NOT claim exact double API equality.
- Initial scalar-rho/vector-lum low-cutoff fixture hit reference helper's scalar
  logical-index shape quirk; final paired rho/lum vectors use point-query cutoff
  semantics. See tests/CSF_VALIDATION.md. Final reference dir
  validation/csf_paired_reference_639278581311962498; logs csf_paired_before_fix.log,
  csf_corrected_comparison.log, csf_float_output_comparison.log.
- Documentation tests/math_reference/README.md and tests/CSF_VALIDATION.md;
  ADAPTATION_MATH_VALIDATION.md points to source-backed follow-up. Prior pinned
  binary probe preserved as historical local evidence, not used by final generator.
- No new commit/push; last remote checkpoint b46666d. Shared prior work, dirty
  VkFFT, original staged shader and all references preserved.
- Exact next step: validate256-entry CSF LUT/grid interpolation and full CSF maps
  with reference/test mean adaptation, matching log-domain Octave imresize
  (bicubic/antialias/boundaries/precision). Current bilinear map is still an
  approximation. Direct float nCSF pass does not validate LUT/maps or masking.

## Previous session: pinned math probe makes all current strict preprocessing pass

- Investigated remaining adaptation log/exp differences with65536 isolated pairs
  (24576 actual image pairs + synthetic log[1e-6,1e10], exp[-80,80]). Added
  tests/float_math.comp and generate_float_math_reference.m. Initial series
  experiments failed: FMA residual36930 total differences; split-product
  compensation24 differences (21log/3exp), including5 actual-log differences.
  Some series outputs equal double-to-float rounding but not native logf.
- Actual liboctave-13.dll imports logf/expf from Windows UCRT math API, not the
  generic MinGW wrapper. Read-only import/export/arithmetic inspection under
  validation/octave_math_imports.txt, ucrt_math_exports.txt,
  ucrt_logf_disassembly.txt, ucrt_expf_disassembly.txt.
- Local version-pinned generator validation/generate_ucrt_log_probe.ps1 rejects
  any ucrtbase.dll hash except5C52E3A303BAAAC0E0AF8BD9B96134993DA34BC9D834A31EF37E1D2CDC7FE192
  (version10.0.26100.9444). Native log graph plus compensated native exp graph
  matches all65536 pairs /131072 function outputs exactly. No float64 feature
  required. Binary-derived tables/probe artifacts stay LOCAL, not portable math.
  Evidence validation/ucrt_log_exp_probe_comparison.log. Ref dir
  validation/float_math_reference_639278571099031674; earlier failed logs preserved.
- Added original tests/float_multiply.comp (integer24x24 significand product,
  explicit ties-to-even, gradual underflow) and generate_float_multiply_reference.m.
  All262144 finite pairs match, including196608 actual spectrum products plus
  randomized and subnormal/tie/signed-zero cases. Evidence float_multiply_comparison.log;
  reference validation/float_multiply_reference_639278575349483287.
- LocalAdaptFFTInput.referenceMathDirectory is an explicit runtime override for
  prepare.spv, finish.spv, multiply.spv, reference FFT backend only. Harness
  IQM_FFT_MATH_SHADER_DIR sets it; exports fft_math_shader_dir.txt. Empty retains
  existing math. Config/path changes after initialization rejected. Defaults
  unchanged. Full probe folder validation/ucrt_complete_math_639278575887831116.
  Earlier math-only folder ucrt_math_shaders_639278574065405048 lacks multiply;
  not usable with the current complete-override loader.
- **All3 patterns /6 images now pass --strict-all-stages and full preprocessing
  --strict with zero mismatches**, including every filter/prepare/FFT/multiply/
  inverse/finish comparison and full retinal/L+M/adaptation/JND maps. Pupil
  coefficients equal; all57216 raw SPYR values and centered residuals equal.
  Native single JND13-stage checks also pass. No Vulkan validation errors.
  Scoped64x64,30ppd,age24,D65 luminance fixtures on this UCRT/device environment.
- Final exports: validation/spyr_spatial_17922608381270175 (asymmetric),
  spyr_spatial_17922608432949170 (diagonal), spyr_spatial_17922608486247470 (impulse).
  Logs validation/ucrt_complete_<pattern>_{run,stages,full,native_jnd,spyr}.log.
  Build ucrt_complete_math_build.log completed exit0. Default VkFFT regression
  passes old tolerance, max0.00015259/RMSE0.00002840, ucrt_complete_default_regression.log.
- First log/exp-only asymmetric probe spyr_spatial_17922606838939870 already
  yielded exact exported maps, but subnormal products still differed internally;
  final integer-product probe removes those differences too. Do not conflate scopes.
- tests/diagnose_fft_stages.m adds --strict-all-stages; shared GPU harness adds
  --float-math/--float-multiply with per-function diagnostics. Full documentation
  tests/ADAPTATION_MATH_VALIDATION.md; no production math shader defaults edited.
- No commit/push: version-specific binary-derived probe tables and shared files
  remain local; last verified remote checkpoint b46666d unchanged. Existing
  staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7, other work and VkFFT preserved.
- Next: replace the pinned binary probe with maintainable portable source-backed
  math behavior (preserve exact target/reference precision), broaden its domain
  tests, then CSF LUT/imresize/masking/pooling/score parity. Current pass is
  preprocessing/JND/SPYR on fixtures, not full HDR-VDP or entirely GPU completion.

## Previous session: reference FFT integrated; MTF/JND/SPYR now exact on6 images

- Added explicit opt-in referenceShaderPath in LocalAdaptFFTInput and runtime
  parity pipeline/scratch buffer in LocalAdaptFFT. Four immutable descriptor
  sets route work->scratch->work forward and filtered->scratch->work inverse.
  Uses proven graph8/9 and3/4 with barriers, no VkFFT plan for reference mode.
  Defaults preserved. Backend/path cannot change after initialization.
- Harness options: IQM_PREPROCESSING=fft, IQM_FFT_BACKEND=reference,
  IQM_FFT_AXIS_ORDER=column, IQM_FFT_TWIDDLES=native,
  IQM_FFT_REFERENCE_SHADER=<absolute validated SPIR-V path>. Used existing
  validation/fft_forward2d_initial.spv. Trace backend and inverse storage
  metadata exported; reference inverse stays column-major, crop honors it.
- diagnose_fft_stages.m and diagnose_fft_order.m now honor inverse storage
  contents, not just metadata file existence. New --strict-transforms asserts
  identical-input forward/inverse equality while reporting other differences.
- Completed build validation/fft_reference_backend_build.log.3 GPU runs with
  Vulkan validation: asymmetric spyr_spatial_17922599150543331,
  diagonal spyr_spatial_17922599673027534, impulse spyr_spatial_17922599723530707.
  No Vulkan validation errors found. Each covers ref/test,6 images total.
- **Every MTF stage, full retinal and L+M input match exactly** (24576 pixels
  each). Same-input forward/inverse match for both MTF/adaptation, all6 images.
  Native pupil/JND coefficients and24576 JND pixels match full official output.
  All57216 raw SPYR coefficients match, including raw and DC-removed residual.
- **Full preprocessing still fails strict parity due to adaptation maps**:
  asymmetric ref/test max6.67572021484375e-6 /5.7220458984375e-6,4426 mismatches;
  diagonal5.7220458984375e-6 /1.049041748046875e-5,4885 mismatches;
  impulse8.58306884765625e-6 /6.67572021484375e-6,4437 mismatches.
  First difference is prepare log (max2.384185791015625e-7). Same-input exp
  finish differs by up to1.9073486328125e-6; spectrum multiplication has
  subnormal differences up to about1.36e-38. Filters and FFTs themselves equal.
  Identical JND/SPYR on mild stimuli does not waive adaptation-map failure.
- Evidence validation/fft_reference_backend_<pattern>_{run,stages,strict_transforms,native_jnd}.log;
  full-chain logs asymmetric_full_absolute.log, diagonal_full.log, impulse_full.log
  with same fft_reference_backend_ prefix (strict exit1 expected/documented).
  Initial asymmetric_full.log used a relative path and failed after nested
  Octave run changed cwd; invalid numerical run, corrected with absolute path.
- Existing default VkFFT regression rebuilt/passes old tolerance:
  fft_reference_backend_legacy_build.log /legacy_regression.log, max0.00015259,
  RMSE0.00002840. Do not describe it as exact or assert unchanged numerical error.
- New documentation tests/REFERENCE_FFT_INTEGRATION.md. This integration remains
  LOCAL: shared files contain pre-existing uncommitted work and full adaptation
  still fails strict parity. No new commit/push. Last remote verified checkpoint
  remains b46666d. Existing staged shader1cf7bae..., dirty VkFFT and other work
  preserved. No files deleted or references overwritten.
- Exact next step: match native single log/exp rounding and subnormal spectral
  products, rerun all3 strict comparisons, then CSF/imresize/masking validation.

## Previous session: full forward128x128 matches and verified tests pushed

- Composed forward transform in test shader graph8/9. First dispatch applies
  real128 per column and writes65 transposed frequency vectors per matrix.
  Second dispatch uses n2fv_16/t3fv_8 forfreq0..62 and distinct n1fv_128 for63..64.
  Writes column-major full output, completing partner F(-x,128-k)=conj(F(x,k))
  for0<k<64. Both axes reflected; no normalization/DC removal. Compute barrier
  and distinct SSBOs, completion fence. Real modes reject nonzero imaginary input.
- CPU --forward2d <new directory> [GPU export directory] uses actual Octave
  single DLL,22-thread estimate/unaligned R2C2D with compact offset8064.
  Verifies all expected graphs including distinct tail. Shared CPU transform2d
  helper preserves inverse fixture generation; inverse regressions rerun.
- New tests/validate_forward2d_reference.m independently compares every expected
  matrix with native single fft2 and the native probe, asserting actual22 threads.
- **19 matrices /311296 full-spectrum complex values /622592 float components:
  max=0, RMSE=0, mismatches=0** on GPU and independent Octave checks. Includes
  conjugates;158080 compact bins computed independently. Seven synthetic plus
  twelve ref/test MTF/adaptation stage0 snapshots across3 spatial patterns.
- Evidence under validation/: fft_forward2d_initial_comparison.log,
  fft_forward2d_asymmetric_comparison.log, fft_forward2d_diagonal_comparison.log,
  fft_forward2d_impulse_comparison.log. Oracle dirs fft_forward2d_reference_639278561661925639,
  fft_forward2d_asymmetric_639278562033305292,
  fft_forward2d_diagonal_639278562208629720,
  fft_forward2d_impulse_639278562273635020 (each includes Octave PASS log).
  Shader fft_forward2d_initial.spv; build logs fft_forward2d_build.log,
  fft_forward2d_final_build.log (completed exit0).
- Final rebuilt tools generated fresh forward2d/inverse2d fixtures, both pass;
  existing R2C/main/tail also pass: validation/fft_forward2d_final_639278562885963009/.
  No Vulkan validation errors found. Production code/settings unchanged.
- Following user's authorization to publish only verified changes, committed
  nine test/doc files as **b46666df138c5b2770f004ad59603a2d0eaf28ab**,
  `Validate complete Vulkan forward 128x128 FFT against Octave`, and pushed
  origin/feature/hdr-vdp3. ls-remote confirms remote matches local HEAD.
  Existing staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7 preserved;
  all other dirty/untracked production work and VkFFT preserved. Handoff local.
- Exact next step: integrate proven forward/inverse into an explicit opt-in
  preprocessing test path, preserve existing defaults and axis/layout traces,
  then compare full MTF/adaptation stages against official Octave. Same-input
  FFT success does not settle filter/log/exp/subnormal differences or full metric.

## Previous session: verified test checkpoint committed and pushed

- User explicitly requested sending only verified changes to GitHub.
  Published commit **2a333e615055daa0ebb4838c9009811891c86ceb**,
  `Add isolated Vulkan FFT parity validation tools`, to origin/feature/hdr-vdp3.
  Push succeeded and ls-remote confirms remote HEAD equals local commit.
- Commit contains24 test/source/license/documentation files only. No production
  metric/shader/preprocessing/submodule changes, binary outputs, local validation
  artifacts or root build changes included. No full metric parity claim.
- Added standalone tests/fft_parity/CMakeLists.txt and README so tools build
  without pending main-project/JND changes. MSVC /EHsc explicitly enabled.
  Build validation/fft_publish_final_build.log succeeded; no warnings/errors.
- Regenerated/compiled shipped shader and generated fresh synthetic fixtures
  using standalone binaries: all8 groups PASS (leaf8/16/128, normalized inverse2d,
  R2C, inverse second-axis6/2, forward second-axis65/3, forward tail2). No Vulkan
  validation errors. Evidence validation/fft_publish_checks_639277456979720179/.
  Public summary tests/fft_parity/VALIDATION.md; reproduction instructions in README.
- Git commit --only with explicit24-path list excluded pre-existing staged
  shaders/hdrvdp3/hdrvdp3_corrdn_test.glsl. Its staged blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7, exactly as before. Unstaged changes,
  other untracked files and dirty VkFFT preserved. Pre-publish staged backup:
  validation/prepublish_staged.patch; selected list validation/fft_publish_paths.txt.
- This handoff remains local; broader unverified production changes are not
  published. Next algorithm step unchanged: full128x128 forward composition,
  correct tail placement and 2D Hermitian completion against native Octave fft2.

## Previous session: forward second-axis main and distinct tail match

- Test-only generator now translates ordinary n2fv_16/t3fv_8 to graph6.
  Eight decimated16 transforms read j+8*l; twiddle8 writes k+16*j. Powers1,3,7;
  AVX2 complex multiplication and constant FMA operations preserved. STM2
  float-offset2*k maps complex indexk; STN2 is no-op for AVX2-128.
- **Native forward tail correction:** 65 compact frequencies under22 threads
  form21 groups3 and a final group2. The group2 uses n1fv_128_avx2_128, NOT
  n2fv_16/t3fv_8. Existing native forward plan confirms it. Added graph7 for
  this ordinary SIMD128 routine. Original three C files preserved unchanged
  in tests/fftw_reference/ with existing GPL attribution/license.
- CPU --forward-second <new directory> [65|3|2] uses actual Octave single DLL,
  in-place one-thread estimate/unaligned, stride=group distance1. Plan asserts
  composite for65/3 and n1fv_128 for2. GPU fixtures contiguous per transform;
  2D compact layout/dispatch and full fft2 remain next, not yet validated.
- **Group65:520 transforms /66560 complex; group3:513 /65664; group2:512
  /65536. Total1545 transforms /197760 complex outputs. All max=0, RMSE=0,
  mismatches=0**, graph6/fused0 for65/3, graph7/fused0 for2. Constant, complex
  impulse, ramp and seeded random at four scales. No Vulkan validation errors.
- Oracle dirs validation/fft_forward_second65_639277453522684322,
  fft_forward_second3_639277453764368273, fft_forward_tail2_639277454446772600.
  Each contains codelet_plans.txt and input128.txt/forward128.txt.
  Earlier fft_forward_second2_639277453829417292 rejected unexpected tail plan;
  not a GPU parity failure and not a usable fixture. Preserve as diagnostic.
- GPU logs under validation/: fft_forward_second65_comparison.log,
  fft_forward_second3_comparison.log, fft_forward_second3_final_comparison.log,
  fft_forward_tail2_comparison.log. Shaders fft_forward_second_initial.spv and
  fft_forward_tail_initial.spv (latter adds tail graph, used for final3/tail).
  Completed builds fft_forward_second_build.log, fft_forward_tail_build.log.
- R2C and full synthetic inverse2d regressions pass with final extended shader:
  fft_forward_simd_r2c_regression.log, fft_forward_simd_inverse_regression.log.
  Production code/settings untouched; existing staged/unstaged/untracked work
  and dirty VkFFT preserved. No commits or destructive commands.
- Exact next step: compose full128x128 forward R2C; scalar real128 per column,
  compact65 frequencies; graph6 on freq0..62, graph7 on63..64; 2D Hermitian
  completion includes conjugating both axes. Compare native22-thread fft2 on
  synthetic and all three patterns' MTF/adaptation stage0 snapshots. Then
  experimental preprocessing integration and strict full-chain reruns.

## Previous session: forward first-axis R2C128 matches

- Added original FFTW r2cf_128.c (same archive/license, notices preserved) in
  tests/fftw_reference/. SHA256:
  9333D9DC8E068ABE1CA1C6F48558C42ADAA2B1B86EB0E60E0A220C249C65192F.
  Test-only generator translates ordinary graph to graph5, with precise float
  intermediates and separate product/add rounding. R0/R1 map to even/odd samples.
- GPU outputs65 bins (imaginary DC/Nyquist zero) then completes to128 by
  Hermitian conjugation. No normalization/DC removal. This is a 1D first-axis
  test, not a complete forward2d implementation. SSBO input imag components0.
- Added CPU --r2c <new directory> [GPU export directory], actual Octave single
  DLL, one-thread estimate/unaligned; rejects unexpected r2cf_128 plan.
  Optional stage0 imports honor row/column metadata and record source files.
- Added tests/validate_r2c_reference.m: independently compares every vector
  against native single Octave fft(real(input),[],1), with strict assertions.
- Four sets of512 transforms (synthetic; asymmetric/diagonal/impulse ref/test
  MTF/adaptation stage0 columns): **2048 transforms / 133120 computed bins /
  262144 full-spectrum complex values including conjugates; max=0, RMSE=0,
  mismatches=0**. Vulkan and Octave checks passed. No Vulkan validation errors.
- Artifacts validation/fft_r2c_initial.spv; build logs fft_r2c_build.log and
  fft_r2c_final_build.log (completed exit0). GPU logs fft_r2c_initial_comparison.log,
  fft_r2c_asymmetric_comparison.log, fft_r2c_diagonal_comparison.log,
  fft_r2c_impulse_comparison.log. Initial synthetic log says inverse for graph5;
  reporting label corrected in later harness build (arithmetic unchanged).
- Oracle dirs under validation/: fft_r2c_reference_639277449506445875,
  fft_r2c_asymmetric_639277450118078253, fft_r2c_diagonal_639277450256072226,
  fft_r2c_impulse_639277450390040066. Each includes r2c_plan.txt, source.txt,
  input128.txt, forward128.txt, octave_reference_check.log (PASS).
  Snapshot sources same three existing spyr_spatial directories as below.
- Full synthetic inverse2d regression passed with extended shader:
  validation/fft_r2c_inverse2d_regression.log. Production path/settings unchanged;
  existing staged/unstaged/untracked changes and dirty VkFFT preserved.
- Next: isolate forward SIMD n2fv_16_avx2_128 and t3fv_8_avx2_128, including
  native batch-tail choices. Compose full128x128 R2C/Hermitian completion and
  compare against native22-thread fft2, then experimental preprocessing
  integration and full-chain strict reruns. log/exp/subnormal issues remain.

## Previous session: full normalized 128x128 Vulkan inverse matches

- Extended test-only generator/harness with two dispatches: graph3 scalar
  first axis writes transposed intermediate, compute write/read barrier,
  graph4 SIMD second axis transposes back and divides once by 16384.
  SSBO layout is interleaved complex float32, column-major per matrix.
  Distinct input/intermediate/output buffers and descriptor sets; fence before
  readback. Both complex components compared; no DC removal/crop/tolerance.
- CPU tool `--inverse2d <new directory> [GPU export directory]` uses the actual
  Octave single FFTW DLL, 22-thread estimate/unaligned 2D plan; verifies expected
  scalar/SIMD decomposition. Optional export imports ref/test mtf/la stage2,
  honoring fft_axis_order.txt, and records exact source files.
- New `tests/validate_inverse2d_reference.m` independently asserts every CPU
  expected matrix equals native Octave ifft2 and native single probe; actual
  single thread count asserted22. All four reference sets passed (exit0).
- **19 matrices / 311296 complex outputs / 622592 float components match
  exactly: max=0, RMSE=0, mismatches=0.** Seven synthetic (constant, off-center
  complex impulse, asymmetric ramp, random four scales) plus twelve snapshots
  (ref/test MTF/adaptation for asymmetric, diagonal, impulse).
- GPU comparison logs under validation/: fft_inverse2d_initial_comparison.log,
  fft_inverse2d_asymmetric_comparison.log, fft_inverse2d_diagonal_comparison.log,
  fft_inverse2d_impulse_comparison.log. No Vulkan validation errors found.
  Prior size8/16/128 regression passes: fft_inverse2d_leaf_regression.log.
- Oracle dirs under validation/: fft_inverse2d_reference_639277446909181448,
  fft_inverse2d_asymmetric_639277447266540857,
  fft_inverse2d_diagonal_639277447313943663,
  fft_inverse2d_impulse_639277447340166724. Each includes inverse2d_plan.txt,
  input128.txt/inverse128.txt, source.txt and octave_reference_check.log.
  Snapshot sources: spyr_spatial_17921430489879733, 17921430997117838,
  17921431152989965. Octave exit diagnostic after PASS is the previously known
  execution_exception message; assertions and actual exit0 confirmed.
- Built test targets successfully (reviewed MSBuild escalation):
  validation/fft_inverse2d_build.log. Shader validation/fft_inverse2d_initial.spv.
  Production preprocessing code/settings unchanged; GPL-derived routines still
  test-only. Existing dirty/staged/untracked changes and VkFFT preserved.
- Next: forward R2C first-axis and SIMD second-axis/tail graph parity with
  native fft2. Then integrate proven transforms into an explicit experimental
  preprocessing path and repeat full-chain strict comparisons. Same-input
  inverse success does not prove input spectra, log/exp or full metric parity.

## Previous session: second inverse axis and native batch tails match

- Extended test-only shader generator with the ordinary `t3bv_16` SIMD graph,
  preserving AVX2 explicit FMA in complex multiplication and constant FMA
  operations. The generic FMA-oriented graph failed and is not selected.
  Original t3bv_16.c and simd-avx2-128.h preserved under tests/fftw_reference/
  with original notices and existing GPL COPYING; no production integration.
- Added CPU `--second-axis <new directory> [128|6|2]` oracle mode. Must plan
  **in place**, complex stride128, distance1; out-of-place chooses scalar
  t2_8 and was an invalid oracle for this target. Final oracle rejects a plan
  lacking t3bv_16_avx2_128 or n1_8. Reference still uses actual Octave DLL.
- SIMD twiddle powers are 1,3,9,15. Sixteen decimated n1_8 transforms read
  j+16*l; eight twiddle16 groups write k+8*j. Host-generated float twiddles;
  no GPU trig, DC removal or normalization. GPU fixture transforms contiguous;
  this tests arithmetic/within-transform layout, not cross-axis 2D dispatch.
- **Group128: 512 transforms / 65536 complex outputs; group6: 516 / 66048;
  group2: 512 / 65536. All max=0, RMSE=0, mismatches=0**, selected graph2/fused0.
  Total 197120 complex outputs. Groups6/2 match main/tail batch sizes of the
  native 22-thread inverse plan; not a full 2D parity claim.
- Final build: validation/fft_second_axis_strict_build.log (completed exit0).
  Shader: validation/fft_second_axis_ordinary.spv. Comparison logs:
  fft_second_axis_ordinary_comparison.log, fft_second_axis_group6_comparison.log,
  fft_second_axis_group2_comparison.log under validation/ (each passed).
  Earlier size8/16/128 selected-path regressions also pass with the extended
  shader: validation/fft_second_axis_leaf_regression.log. No Vulkan validation
  errors found in successful runs.
  Oracle dirs: fft_second_axis_inplace_639277442984315744,
  fft_second_axis_group6_639277444124781670,
  fft_second_axis_group2_639277444365317727 under validation/.
- Earlier initial/out-of-place/FMA-graph failures are preserved as diagnostics,
  not parity passes. Existing staged/unstaged/untracked work and VkFFT preserved.
- Next: compose both proven 128-point axes into a full GPU 128x128 inverse,
  with explicit column/row dispatch and normalization. Compare arbitrary complex
  fixtures and real MTF/adaptation stage2 snapshots against the proven native
  22-thread Octave plan. Forward FFT/log/exp/subnormal issues remain afterward.

## Previous session: composed 128-point inverse matches on Vulkan

- Extended the existing test-only generator, CPU oracle and Vulkan harness to
  size 128; production FFT code and settings remain unchanged.
- Preserved original FFTW `t2_8.c` in `tests/fftw_reference/` with its existing
  GPL attribution/COPYING. Generator translates both original graph variants.
- Important layout: first eight decimated n1_16 transforms read `j+8*l`, store
  `j+8*k`; sixteen t2_8 groups then write `k+16*j`. Twiddle multiplication is
  before the radix-8 butterfly. Compact twiddle powers are **1,3,7**, not 1,2,4.
  Host double trig is rounded once to float32 and embedded in the test shader;
  no GPU sin/cos. Inverse uses real/imag swap, with no normalization/DC removal.
- Actual local single FFTW DLL prints 128-point plan `dft-ct-dit/8`,
  `t2_8`, `n1_16`, matching the native first inverse axis decomposition.
- **512 composed transforms / 65536 complex outputs: max=0, RMSE=0,
  mismatches=0**, selected graph=1/fused=0. Size-8/16 regressions also pass;
  combined total 77824 complex outputs / 155648 float components. Strict test
  now requires the selected path for all three sizes, not any matching variant.
  Fixtures retain constant/impulse/ramp and seeded complex random values over
  four scales. This establishes current fixture parity, not arbitrary inputs.
- Completed build log `validation/fft_inverse128_build_reviewed.log` (exit 0).
  Initial sandbox build hit the known MSBuild FileTracker error; reviewed
  escalation succeeded. No machine script policy changes.
- Oracle directory: `validation/fft_inverse128_reference_639277431765714130`.
  Shader: `validation/fft_inverse128_initial.spv`; comparison:
  `validation/fft_inverse128_initial_comparison.log` (exit 0). No Vulkan
  validation errors found. See `tests/fftw_reference/README.md` for reproduction.
- Existing dirty/staged/untracked changes and VkFFT preserved; no commits.
- Exact next step: isolate buffered `t3bv_16_avx2_128` plus `n1_8` second-axis
  arithmetic, including batch tails; then compose 128x128 inverse with explicit
  axis layout and normalization and compare with proven native 22-thread plan.
  1D success does not yet validate full inverse, forward FFT, or full metric.

## Previous session: isolated inverse codelets match on Vulkan

- Added test-only `IQM-fft-codelet-test`, `tests/fft_codelet_gpu_test.cpp`,
  `tests/generate_fft_codelet_shader.ps1`, and the `--codelets` mode of
  `IQM-fftw-plan-test`. No production FFT/shader arithmetic was changed.
- CPU expected values come from the actual installed Octave single FFTW DLL,
  with printed inverse plans `n1_8` and `n1_16` (one-thread estimate/unaligned).
  These isolate leaf routines; they do not change the native 22-thread 2D oracle.
- Selected GPU graph=1/fused=0 matches exactly: 512 transforms per size,
  4096 + 8192 = **12288 complex outputs / 24576 float components**, max=0,
  RMSE=0, mismatches=0. Fixtures include constant, impulse, ramp and seeded
  complex random inputs at scales 1e-3, 1, 10 and 1e4. Inverses are unnormalized.
- Initial radix-16 translation had 257 differing float components. Explicit
  separate `precise` products in graph-1 product pairs eliminated them.
  Other arithmetic variants are diagnostic only; strict success requires the
  selected graph=1/fused=0 for both sizes. No universal input-equivalence claim.
- Strict completed build: `validation/fft_codelet_strict_build.log`; completed
  run (exit 0): `validation/fft_codelet_strict_comparison.log`. No Vulkan
  validation errors found. Oracle: `validation/fft_codelet_reference_639277418751685599`;
  final SPIR-V: `validation/fft_codelet_products_639277420167425265/fft_codelet_test.spv`.
  Earlier failed comparison logs are preserved and are not parity passes.
- Official FFTW 3.3.10 archive SHA256:
  `56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467`.
  Original n1_8.c/n1_16.c and COPYING are preserved in `tests/fftw_reference/`;
  generated shader carries attribution and GPL-2.0-or-later. These remain
  test-only, not integrated into production. See its README for reproduction.
- Windows generator required an approved process-only ExecutionPolicy Bypass;
  no machine policy changed. Existing staged/unstaged files and VkFFT preserved.
- Next: validate `t2_8` twiddles and stride/layout composition with `n1_16`
  for the first inverse 128-point axis, then buffered `t3bv_16_avx2_128` plus
  `n1_8` for the second axis and final normalization. Compare intermediates
  against the proven native plan before integrating. Full FFT/metric parity
  remains unresolved; forward R2C, log/exp and subnormals still need work.

## Previous session: exact native FFTW plan identified

- Production shader/FFT arithmetic was not changed in this session. Added only
  diagnostic tools, a Windows-only CMake target, documentation and inspection
  copies of official source under `validation/`. No commits or cleanup.
- **Correction to earlier thread notes:** `fftw('threads') == 3` reports the
  double planner. Installed Octave 11.3.0's native single planner actually uses
  **22 threads on this host**. Query via the native probe, not that getter.
  Do not silently set the reference to 3; it changes single codelet selection.
- Confirmed exact release source: `validation/oct-fftw-release-11-3-0.cc`,
  SHA256 `DB730C34E2C6DDB5FB078D569FA8A6A259903FA14DE278B8374668658B7545B6`.
  Default-branch inspection copy had the same hash. Source URL is recorded in
  `tests/FFTW_PLAN_VALIDATION.md`. Official C:/HDR-VDP-3_Ref was untouched.
- Added `tests/inspect_fftw_native.cc`: builds as an Octave extension, calls
  native public single FFT wrappers, prints their exact plans and thread count.
  `tests/validate_native_fft_probe.m` verifies returned results equal ordinary
  fft2/ifft2 before accepting plans. All 8 asymmetric MTF/adaptation transforms
  passed (131072 complex values). Probe binary/plans:
  `validation/octave_plan_probe_639277410904713098/`; log
  `validation/fftw_native_probe_threads.log`.
- Added `tests/inspect_fftw_plan.cpp` / target `IQM-fftw-plan-test`: independent
  diagnostic loading the same local FFTW DLL. Mirrors native R2C offset 8064
  complex values, compact output/Hermitian reconstruction, and normalized C2C
  inverse; production remains Vulkan (no CPU fallback introduced).
- With 22-thread estimate/unaligned planning, **all 3 patterns, 6 images,
  forward+inverse = 196608 complex values match exactly** (max=0, RMSE=0,
  mismatches=0). `validate_fftw_plan.m --strict-native` asserts equality for
  the same native candidate on both images and directions.
- Logs: `validation/fftw_true_single_strict.log` (asymmetric),
  `validation/fftw_native_diagonal_comparison.log`,
  `validation/fftw_native_impulse_comparison.log`.
  Result directories: `fftw_true_single_639277411128626625`,
  `fftw_native_diagonal_639277412003115239`,
  `fftw_native_impulse_639277412261117616` under validation/.
- Native forward: r2cf_128 -> buffered t3fv_8_avx2_128 + n2fv_16_avx2_128;
  operation counts (adds,muls,FMAs)=(140658,38106,19584).
  Native inverse: t2_8 + n1_16 -> buffered t3bv_16_avx2_128 + n1_8;
  counts=(408576,108544,47104). Thread partitions affect tail codelet selection;
  GPU must mirror arithmetic/batch-tail choices, not run 22 CPU threads.
- Earlier 3/1-thread experiments are preserved but are not forward parity
  passes. Their residual differences on rows 64/66 were not a Hermitian bug;
  correct thread-dependent plan selection eliminated them. Importing wisdom
  alone also did not establish a matching plan (all cache-key probes failed).
- Diagnostic runtime notes: Windows LoadLibraryEx requires the correctly
  formed backslash absolute path here. FFTW sprint_plan text uses ordinary
  malloc/free, not FFTW's aligned allocator. Early failed probe directories
  have incomplete artifacts and are not validation results.
- Full Vulkan preprocessing parity remains unresolved; prior GPU error
  measurements below are unchanged. Next: isolated inverse radix-8/16 GPU
  arithmetic tests against the now-proven native plan.

## Previous session: FFT ordering and twiddle investigation

- Added `tests/diagnose_fft_order.m`. Local Octave reports estimate planner
  and 3 double threads; actual single=22 (see correction above).
  FFTW wisdom reports 3.3.10/AVX2 codelets.
- Current row-first GPU forward spectrum is much closer to Octave row-first
  sequential FFT than to official fft2. Asymmetric reference spectrum:
  GPU vs real fft2 max=0.007682190371777677; vs row-first max=4.464988674438334e-5.
- Important diagnostic correction: `fft2(real_array)` and
  `fft2(complex_array_with_zero_imaginary_values)` are different numerical
  paths. The full reference uses real input. Stage diagnostic now uses
  `fft2(real(gpu0))`, and separately reports complex-input comparison.
- Added optional `LocalAdaptFFTInput.columnFirst` and harness
  `IQM_FFT_AXIS_ORDER=column` (default `row`). Prepare stores transposed padded
  data. VkFFT inverse reverses the forward traversal; transpose filtered data
  back using transfer-only element copies before inverse, then finish reads
  row-major. Radial square filter is unchanged. Prepare/finish push constants
  now have four uint32 values (16 bytes).
- Shader binaries rebuilt individually and previous binaries backed up in
  `validation/fft_shaders_before_axis_*`. No blanket shader rebuild.
- Initial prepare/finish-only transpose worsened the full chain; preserve its
  `validation/fft_column_*` artifacts as a rejected experiment. Both-direction
  alignment (`fft_aligned_*`) improved asymmetric full MTF maxima from
  3.814697265625e-6/3.814697265625e-6 to
  2.86102294921875e-6/1.9073486328125e-6 (ref/test). JND mismatches 1171/882
  fell to 444/433. Maximum JND error remained 0.000244140625.
- Added `IQM_FFT_TWIDDLES=lut` / `LocalAdaptFFTInput.useTwiddleLUT` (default
  `native` / false, VkFFT useLUT=-1). A reused plan rejects twiddle mode changes.
- LUT exposed a latent null-fence bug: VkFFT was given a pointer to a null
  fence handle. LUT upload submits/waits/resets this fence. Fixed with an owned
  RAII fence that outlives the plan. Do not edit `lib/VkFFT` to work around it.
- Successful asymmetric LUT run: `spyr_spatial_17921430489879733`.
  Forward MTF spectrum max vs native real fft2 = 2.750817318316642e-5 (ref),
  3.075507258719844e-5 (test). Full MTF maxima unchanged vs aligned-native,
  JND mismatches 434/411, max JND error still 0.000244140625.
- Build completed successfully. Post-fix LUT GPU runs had no validation errors.
  All three patterns (six images) passed identical-map native JND/13-stage
  trace checks and identical-JND SPYR checks after the fence fix.
- Full-chain maxima for column-aligned + LUT: diagonal retinal max
  2.86102294921875e-6/1.9073486328125e-6, impulse retinal max
  1.9073486328125e-6/1.9073486328125e-6 (ref/test). All six full reference
  pupil coefficients now match. All six full JND maxima remain
  0.000244140625 (not exact parity). Full JND mismatches are asymmetric
  434/411, diagonal 470/374, impulse 460/389.
- Existing default/non-LUT FFT integration regression passed its old tolerance
  after the fence fix, unchanged max=0.00016022, RMSE=0.00002903.
  Evidence: `validation/fft_lut_postfix_legacy_integration.log`.
- Invalid artifacts: the first `fft_lut_asymmetric_*` ran an old executable
  before link completed, causing LNK1104; subsequent `fft_lut_verified_*`
  exposed the null-fence crash, so neither is a numerical LUT pass. See
  `validation/fft_lut_asymmetric_INVALID.md`. Valid post-fix prefix is
  `validation/fft_lut_fixed_*`.
- Always wait for a successful completed build before launching IQM. For a LUT
  run verify `fft_twiddles.txt` actually contains `lut`; do not compare an old
  executable's output or an incomplete/crashed run directory.
- No default settings changed: preprocessing=none, FFT axes=row, twiddles=native.
  Full exact preprocessing parity remains unresolved.

## Commands and tools

PowerShell, workspace root. CMake may require sandbox escalation due to MSBuild
FileTracker E_ACCESSDENIED. Use the normal approval tool flow when necessary.

```powershell
glslc shaders/hdrvdp3/hdrvdp3_fft_prepare.glsl -o shaders/hdrvdp3/hdrvdp3_fft_prepare.spv --target-env=vulkan1.2
glslc shaders/hdrvdp3/hdrvdp3_fft_finish.glsl -o shaders/hdrvdp3/hdrvdp3_fft_finish.spv --target-env=vulkan1.2
cmake --build build-vs2026 --config Release --target IQM IQM-jnd-single-test
$env:IQM_PREPROCESSING='fft'
$env:IQM_SPYR_PATTERN='asymmetric' # diagonal, asymmetric, impulse
$env:IQM_FFT_AXIS_ORDER='column' # row is the baseline/default
$env:IQM_FFT_TWIDDLES='lut' # native is the baseline/default
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
# Use this run's printed export directory, not a guessed old reference:
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/diagnose_fft_stages.m '<directory>'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fft_preprocessing.m '<directory>' --strict
& './build-vs2026/bin/Release/IQM-jnd-single-test.exe' '<directory>'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_spyr_spatial.m '<directory>'
```

The full-chain script without `--strict` reports mismatches (exit 0 is NOT
full parity). Its shared-input native checks still assert equality. Octave
often prints `ignoring const execution_exception& while preparing to exit`
even with successful exit code 0; check actual assertions/status.

## Remaining project work

Roadmap review (2026-10-17): FFT passes are isolated same-input checks, not
full metric parity. Implement quality/Q/Q_JOD end to end first; add other
reference task modes only if included in product scope. Native pupil/JND and
CSF/pooling currently include CPU work; an entirely Vulkan implementation
requires separately validating those shader ports after a trusted C++ baseline.
The 2023 paper is a high-level overview; pin executable behavior to official
3.0.7 options, precision and input/viewing parameters. Current CSF and masking
map resizing still explicitly use bilinear approximations to Octave imresize.

1. Source-backed reference FFT/math experiment now passes all current strict
   preprocessing/JND/SPYR checks. Windows binary dependency removed; broaden
   domains/devices without assuming universal equality to all libm builds.
   Keep current GPU experimental modes opt-in; no tolerance-based parity claims.
2. Native log/exp behavior and finite subnormal products match in the local
   probe; validate broader ranges/devices without assuming universal equivalence.
3. Direct CSF float outputs now pass1152 cases after age fix; double exactness
   still pending. Validate CSF LUT/maps, adaptation-map resizing (currently
   bilinear approximation to Octave imresize), masking, pooling and final scores.
4. Extend beyond current 64x64/30-ppd/age-24/D65 scope to other dimensions,
   luminance ranges, color/spectral parameters and real HDR images.
