# IQM HDR-VDP-3 session handoff

Last updated: 2026-10-21 (Europe/Prague). Branch: `feature/hdr-vdp3`.

## Latest continuation: source-to-reference-shader reproduction

- Committed32 scoped generator/source files in
  ffeed634246fca6530165c122cd065b8f65eaa54. Includes seven reference shader
  generation, FFTW runtime codelets with source/member/LF-hash provenance,
  AMD sources/hash manifest/licenses and required math helpers. No push.
  FFT128 generator gained a fresh-output option;FFT64/base use vendored
  hash-checked codelets by default,optional original archive still supported.
- Fresh clean worktree under validation/asset_generation_639281159480285385/
  checkout and clean VkFFT. API built from committed sources;all seven
  reference shaders generated without old archive/binaries/downloads. All7
  SHA256s equal accepted prior binaries;seven embedded headers also equal.
- Fresh direct32/64 API runs with generated shaders preserve score bits and
  203 numerical text export hashes (96/107),plus14 negative cases.16 poisoned
  environment variables ignored;checkout/submodule remain clean. These compare
  prior native-validated baselines,not another fresh Octave/GPU checker run.
- Added default hash-pin verification,license copying and compiler metadata
  to generation;fresh final_manifest_check reproduces all7 binaries unchanged.
  Tampered FFTW source andexisting output directory guards pass,old binary
  preserved. Spectral CSVs andPFMs remain explicit external input data.
- Evidence:validation/asset_generation_639281159480285385/COMPLETED.json;
  pointer validation/asset_generation_completed_path.txt. Includesclean source,
  build,generation/run logs,comparison.json,assets_verified/manifest.json,
  generator_negative_checks.json andfinal_manifest_check. Local drivers:
  validation/validate_generated_api.ps1,check_asset_generator_guards.ps1.
  Initial premature API launch did not execute;initial_launch_note.txt preserves
  this. Verified retry uses assets_verified;all old artifacts retained.
- See tests/REFERENCE_ASSET_VALIDATION.md. Existing staged shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved. New commits
  are local;origin remains6b18daa. Main wrapper/root CMake remain mixed.

Next implementation area:general dimensions. Start from the actual native
256-point FFT plans before translating larger graphs;do not replace the native
graph with an arbitrary mathematically equivalent FFT. Existing bounded API
guards remain until new FFT,base-residual andresize/masking stages pass.

### Native256 frontier started

- Extended only diagnostic tests/generate_fft_size_reference.m to128-pixel
  inputs andtests/inspect_fftw_plan.cpp to256-point FFT. Public API/GPU guards
  remain unchanged. Fresh asymmetric ref/test native MTF/LA fixture generated;
  staged MTF matches official fast_conv_fft exactly at30ppd,single inputs,
  double filters,symmetric post padding,inverse normalization1/65536.
-16 native DLL/Octave plan comparisons:all8 unaligned forward/inverse cases
  (threads1/3,ref/test) match1,048,576 float components exactly.8 aligned
  alternatives differ;they remain rejected evidence,not tolerance passes.
- Native estimate planner uses3 threads. Observed real256 graph is composite
  hc2cf_32/r2cf_32/r2cfII_32/r2cf_8;complex forward includes
  t3fv_16_avx2_128/n2fv_16_avx2_128,and inverse t3bv_32_avx2_128/n1_8.
  Changing a shader size constant or choosing an arbitrary scalar256 FFT
  would not reproduce these native operation orders.
- Evidence:validation/fft256_frontier_639281615622631080/COMPLETED.json;
  pointer validation/fft256_frontier_completed_path.txt. Includes actual
  wisdom,plan trees,all candidate spectra,comparison/build/run logs andsource
  hashes. See tests/FFT256_FRONTIER.md. No GPU128 metric parity claim.

Exact next step:translate the observed native256 real/complex/inverse graphs
as an isolated shader experiment andcompare intermediate results against these
fixtures. Then extend base residual FFT beyond8x8 and128 map reductions/resize;
only widen API guards after isolated andfull-stage tests pass. Existing32/64
regressions andoriginal source/reference assets must remain preserved.

## Latest continuation: clean committed-source build

- Scoped18 API/runtime/build files committed as
  `7c7f4736c324ce2d648151b6f21003f519132f95`,English message
  `Expose bounded luminance quality API with isolated shader build`. This
  promotes the previously verified bounded runtime,not the mixed main wrapper
  or root/source CMake integration. No push;origin still at6b18daa.
- Detached clean worktree at
  validation/clean_api_639281156569491989/checkout. Its VkFFT is clean at
 539be29f30ef40384f3ccd24d7884166f82cbef8,cloned from local Git objects using
  a per-command submodule URL override;no dirty files or configuration changes
  were copied from the original submodule. Original dirty VkFFT is preserved.
- Fresh independent configure/build passes from committed sources,using
  tests/hdrvdp3_evaluation,with build and exports outside the checkout. Seven
  embedded shader headers regenerated from GLSL match prior headers' hashes.
  No root build cache,workspace shaders_out,wrapper,other metrics or dependency
  downloads. Clean checkout and submodule remain clean after all tests.
- Two fresh direct32/64 photo-localized API calls return exact error/Q/Q_JOD
  bits and203 matching numerical text export hashes (96/107),despite16 poisoned
  environment variables. Fourteen negative cases pass. This compares against
  prior native-validated baselines;it is not another fresh Octave/GPU checker
  execution. Scope remains30ppd/age24/D65 positive square luminance.
- Evidence:validation/clean_api_639281156569491989/COMPLETED.json;pointer
  validation/clean_api_completed_path.txt. Includes checkout,build,source and
  binary hashes,configure/build/run/negative logs andassets/manifest.json.
  Local driver:validation/validate_clean_api.ps1;use a fresh work path on rerun.
  Fourteen runtime assets are copied and hashed inassets (FFT2,map/base2,
  math3,spectral3,PFM4);golden files are comparison evidence. Original reference
  files and earlier failed/successful artifacts are preserved.
- Remaining distinction:external reference FFT/math/map/baseFFT assets were
  NOT regenerated from clean committed generator inputs. The clean checkout
  build with pinned assets is verified;the full self-contained source-to-assets
  package is not. See tests/CLEAN_CHECKOUT_VALIDATION.md for exact scope.
- Preserve staged shader index blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7.
  Root/source CMake,wrapper,other existing experiments remain local and mixed.

Exact next step:package external reference shader generation and its source
dependencies/provenance,then reproduce the14 runtime assets from a fresh
location andcompare hashes/numerical results. Audit FFTW graph source/archive,
reference math source manifests andpower/map/base FFT generators before
promoting them. Do not reclassify copied binaries as regenerated assets.
General dimensions,RGB-native encodings andfull frames follow separately.

## Latest continuation: library quality API

- Added `include/IQM/hdrvdp3/evaluation.h` and
  `src/hdrvdp3/evaluation.cpp`, compiled into `IQM-HDRVDP3` with strict FP.
  `evaluate_quality(instance,reference,test,options,trace)` accepts two
  `LuminanceImage` objects and explicit viewing/age, FFT/math/map/baseFFT
  assets and raw spectral directory or sampled data. Returns `QualityScores`
  plus per-band errors. No environment reads or synthetic generation in the
  evaluator. Native arithmetic, padding/layout/DC processing and shaders are
  unchanged. Only validated square32/64 positive luminance inputs are accepted.
- Wrapper now prepares inputs/configuration and calls the library. The old
  preliminary non-FFT/CPU quality modes are rejected explicitly; isolated
  corrDn/SPYR probes remain. Historical constant-image comparison code was
  removed from this path; its source snapshot/reference files are preserved.
- New independent target `IQM-quality-api-test` links the library without the
  wrapper. With16 poisoned legacy environment variables, direct32/64 calls
  return bitwise exact error/Q/Q_JOD and4/5 band errors.203 API-generated
  numerical text exports match baseline hashes. Fourteen invalid shape/count,
  luminance, viewing/age/FFT/map and trace-directory cases are rejected before
  dispatch/overwrite. Diagnostic export to a fresh explicit directory remains
  required by this first API; the shared VulkanInstance must not be concurrent.
- Ten fresh full regressions pass:6 photo luminance pairs +2 synthetic controls
  at30ppd/age24, and2 HDR ramp localized-stronger at60ppd/age60.001, both32/64.
  Official Q/Q_JOD, CPU JND, raw/centered SPYR, CSF/masking/D and independent GPU
  stages pass.1,425,505 floats and59,520 binary64 masks compare with zero
  differences;1,218 full-suite text export hashes remain identical.
- Root: `validation/evaluation_api_639281145664415302/COMPLETED.json`;
  pointer `validation/evaluation_api_completed_path.txt`. Contains before
  snapshots, source/binary hashes, per-case suite paths/logs, direct/extended
  results and14-rejection log. Drivers in validation:run_evaluation_api_regression,
  run_evaluation_api_direct,run_evaluation_api_extended and
  finalize_evaluation_api_checkpoint.ps1. Use fresh work/export paths on rerun.
  Preserve initial failed build (missing transpose helper) and initial direct
  comparison attempt:the latter wrongly expected Octave-added files in API
  output, although the initial direct32 returned scores were exact. Corrected
  direct attempts use fresh directories and compare all actual API files.
- Follow-up CLI error propagation:wrapper rethrows full-quality exceptions;
  main already converts them to nonzero exit. A32-PFM/64-selected rejection
  returns-1 (`cli_reject_dimension.json`). The numerical API checkpoint keeps
  its pre-CLI-change executable/source snapshot; CLI status verification is a
  subsequent build, recorded separately in `CLI_COMPLETED.json`. One additional
  fresh32/asymmetric-localized full strict success regression passes on that
  build:57,046 floats and2,368 double masks exact,116 unchanged text exports.
  The final executable is `IQM_cli_status.exe`,with separate source/hash files.
- Added independent `tests/hdrvdp3_evaluation/CMakeLists.txt` and README.
  Fresh configure/build passes without the root build cache, other metrics,
  FetchContent or workspace shaders_out. Generates exactly seven embedded
  headers from current GLSL into isolated_build/generated;all seven SHA256s
  equal the previously verified embedded headers. Uses SDK discovery rather
  than the main build's hardcoded SDK library paths. Independent32/64 direct
  calls then pass bitwise scores and203 numerical export hashes with poisoned
  environment. Evidence:ISOLATED_BUILD.json,isolated_source_inventory_hashes.json,
  isolated_shader_header_hashes.json,isolated_binary_hash.json and
  isolated_direct_results.json under the API root. This builds the current
  source snapshot,not a clean committed checkout. VkFFT tracked header changes
  are empty;its existing dirty changes affect CMakeLists.txt and an untracked
  glslang directory. The isolated project does not use that modified CMake.
- Current runtime/API integration remains local/uncommitted with existing
  mixed changes. Do not blindly commit whole CMake/wrapper/shader files.
  Published HEAD remains `6b18daa`;earlier11 verified commits were successfully
  pushed, origin and HEAD matched at that checkpoint. Staged shader blob
  `1cf7baedb1d54be0ca8ad02d14485e510b0434e7`,dirty VkFFT and old artifacts preserved.

Exact next step:review and promote the minimal runtime dependency set used by
the new isolated target into a scoped commit,then validate from a fresh clean
checkout with complete external reference shader generation/assets. The
independent build already regenerates all embedded headers;reference FFT,
math,map/baseFFT assets still arrive as explicit pinned runtime inputs.
Resolve shared bin Vulkan resource ownership as needed for packaging.
Then generalize image/FFT dimensions and validate additional official RGB/HDR
encodings and full frames. Those remain unfinished; a measured overall
completion percentage is not available. Do not label square32/64 luminance
validation as full HDR-VDP-3 input-domain parity or a fully GPU-only pipeline.

## GitHub publication checkpoint

- User explicitly requested publication on 2026-10-20. Fetched
  `origin/feature/hdr-vdp3`; the remote has no divergent commits. Prepared
  the ten verified local commits through `d03c2c3`, plus this handoff update,
  for a normal push to `origin/feature/hdr-vdp3` (no force push).
- Publication scope: scalar numerical corrections, spectral calibration/raw
  loading, lossless luminance IO, isolated validation tools and documented
  numerical checkpoints. Commit messages and technical documents are English.
- Full runtime integration, including the validated FFT config boundary,
  remains local with mixed existing changes. The remote branch must not be
  described as a complete standalone implementation of the validated local
  runtime. Existing untracked artifacts and dirty `lib/VkFFT` are preserved;
  the staged corrdn_test shader is excluded. Next implementation step remains
  the reusable quality API described below.

## Latest continuation: explicit FFT configuration boundary

- Committed the independently verified luminance input module, standalone
  tools and validation documents as `d03c2c3931c616b3f38399de7bf109bff392a59a`
  (`Add lossless luminance file input and validate real image crops`). No push.
  Mixed main application integration was excluded; staged shader preserved.
- Added `HDRVDP3FFTConfig` and an explicit `compute_fft_filter_gpu` overload
  in the local wrapper. Axis order, twiddle mode, reference shader path and
  math shader directory are passed explicitly to FFT execution. The test
  harness parses its environment once for the four MTF/adaptation calls.
  The old overload remains a compatibility adapter. Arithmetic, shaders,
  padding, layouts and DC processing are unchanged. This is the first
  extraction boundary, not a complete public quality API.
- Release build passed. Eight fresh full strict native-reference regressions
  passed: six photo-derived luminance pairs and two synthetic controls at
  32/64,30ppd,age24,D65/raw spectral input. GPU checks cover 1,140,404 float
  comparisons and 47,616 binary64 mask values with zero differences.
  Before/after hashes match for 976 text exports (116 per32 run,128 per64
  run; three asset-path metadata filenames excluded from the comparison).
  Full raw/centered SPYR checks, native JND traces and official Q/Q_JOD pass.
- Evidence: `validation/fft_explicit_config_639281136637103838/COMPLETED.json`;
  pointer `validation/fft_explicit_config_completed_path.txt`. Preserves old
  wrapper/header/executable, build log, per-case logs, authoritative suite
  paths from completed logs, verified executable/source hashes, and isolated
  continuation patches against the before snapshots. Driver:
  `powershell -NoProfile -ExecutionPolicy Bypass -File
  validation/run_fft_explicit_config_regression.ps1`. Before rerunning, create
  a fresh timestamped work directory with before snapshots and update
  `validation/fft_explicit_config_work_path.txt`; preserve existing logs.
- Wrapper/header changes remain local with pre-existing mixed integration;
  do not commit their entire contents indiscriminately. The staged shader
  index blob remains `1cf7baedb1d54be0ca8ad02d14485e510b0434e7`.

Exact next step: continue extracting the validated evaluation into a reusable
quality API accepting two `LuminanceImage` objects, explicit viewing/age,
spectral data and shader configuration, returning `QualityScores`. Move
remaining environment parsing and synthetic generation into the harness;
use the explicit FFT overload internally. Preserve the same stage arithmetic
and repeat the six file/two synthetic before/after comparisons. General image
sizes, full original frames, zero-input policy and RGB-native parity remain
unfinished; no measured overall completion percentage is available.

## Latest continuation: file-based absolute luminance input

- New standalone input API:include/IQM/hdrvdp3/luminance.h and
  src/hdrvdp3/luminance.cpp,load_luminance_pfm. Grayscale Pf,three-line
  header,both byte orders,bottom-up raster ->top-down row-major float32;
  header scale magnitude applied once. Requires positive finite luminance.
  No IO resize,gamma normalization or tone mapping. RGB/zero/generalformats
  remain outside this contract. Existing8-bit shared reader is unchanged.
- Independent tests/hdrvdp3_input:25 cases (18 valid,7 rejected),30,810
  float bit values exact;CTest passes. Nonsquare3x5 IO tested separately from
  fixed32/64 full-pipeline support. Photo-content files add15,360 bit values.
- Validation-only prepare_hdr_luminance uses bundled stb float HDR decoding,
  explicit binary32 Rec709 numeric Y andone cd/m2 perderivedY unit assumption.
  Source primaries/photometric calibration are not independently verified.
  External source C:/HDR-VDP-3_Ref/examples/nancy_church.hdr is768x1024.
  Center crops32@(368,496),64@(352,480);no resizing. This is shared luminance
  input validation,not full RGB-native metric parity or whole-frame validation.
- Full wrapper accepts IQM_LUMINANCE_REFERENCE / IQM_LUMINANCE_TEST;both required
  andmust match selected32/64 shape. Synthetic generation/transforms bypassed
  in file mode;input_source.txt records routing. Runner options
  -LuminanceReference/-LuminanceTest hash inputs,use _file prefixes andclear
  stale file flags for synthetic runs. Separate native PFM decoder proves
  30,720 input/export float bits identical across six photo pairs.
- Six photo pairs at30ppd/age24/D65/raw spectral loading:localized1.5 patch,
  global1.05 brighter andidentical at32/64. All official scores,strict stages,
  CPU JND traces,raw/centered SPYR andGPU intermediates match. Photo-only:
  855,303 float comparisons,35,712 binary64 masks,zero differences.
  Two synthetic regressions keep prior scores exact andprove stale flags
  clear. Combined8 fresh runs:1,140,404 floats,47,616 double mask words exact.
- Photo Q/Q_JOD:32localized8.0709753036499023/8.7933588027954102;
  64localized9.2685623168945312/9.6516733169555664;32brighter
  9.9970340728759766/9.999699592590332;64brighter9.9958648681640625/
  9.9995403289794922;both identical10/10. Source crop ranges are recorded
  precisely inmetadata andtests/LUMINANCE_INPUT_VALIDATION.md.
- Expected dimension mismatch is rejected without resize. Legacy wrapper can
  returnexit0 after printing its error;runner correctly treats that as failure.
  rejected_dimension_suite.txt preserves the expected rejection artifact.
- Root:validation/luminance_input_639280968498275868/COMPLETED.json;pointer:
  validation/luminance_input_completed_path.txt. Includes before/source,binaries,
  fixtures,nancy metadata/derivedfiles,all logs,loader/CTest results andnegative
  test. Local finalizer:validation/finalize_luminance_input_checkpoint.ps1.
  No shader changes inthis continuation;corrected ABI13 hash remains0B023987...
- Important export-path race:negative test replaced the shared64 _file latest
  pointer while64-identical wasinflight. Provisional parent suite metadata
  was savedas suites64_from_mutable_latest.json. Canonical validated_suites.json
  parses the unique completed Suite PASS lines;corrected suites64.json now
  agrees. latest_pointer_audit.json records this. No numerical result changed;
  never use mutable global latest pointers as evidence under shared prefixes.
- Preserve staged corrdn_test shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  dirty lib/VkFFT andall pre-existing changes. Scoped input module/tools/docs
  may be committed;main pipeline integration remains local. No push.

Exact next step:extract the validated evaluation from hdrvdp3_test_run into
a reusable quality API accepting a LuminanceImage pair,explicit viewing/age
parameters andasset configuration,without global environment dependence.
Keep the harness thin andcompare before/after against these six file pairs
andtwo synthetic controls before changing arithmetic. Full original frames,
RGB-native/other encodings,zero-input handling andgeneral dimensions remain
unfinished. No measured overall completion percentage exists.

## Previous continuation: raw spectral loading and corrected power rounding

-2026-10-20:raw C++ D65/cone/rod table loader andnonuniform pchip are complete.
  420 native wavelengths,2,520 sampled double values exact;feeding those into
  calibration preserves16,822 double trace values across13 ages. Independent
  scalar CTests spectral_age_double andspectral_loading_double pass2/2.
  New source src/hdrvdp3/spectral_loader.cpp;header/API andstandalone test target
  updated. Raw original data files remain external,unmodified inputs.
- Raw evidence:validation/raw_spectral_639280955944105771/COMPLETED.json;
  pointer validation/raw_spectral_completed_path.txt. COMPONENT_COMPLETED.json
  retains the isolated checkpoint. Two full raw-input runs at32/64,60ppd,
  age60.001,HDR ramp/stronger localized match official scores andall stages:
  285,101 GPU floats and11,904 double mask words,zero differences.218 common
  text export SHA256 hashes equal corrected resampled-input controls (103/115).
- Harness now accepts IQM_SPECTRAL_DATA / -SpectralDataDirectory for the three
  original files;raw andresampled inputs are mutually exclusive. Exports
  spectral_source.txt='raw_d65';runner hashes all three source tables.
  The tested runtime path no longer needs an Octave-produced sampled file.
- Four cross-domain60ppd/new-age HDR ramp stress pairs pass at32/64 andages
  60/60.001:570,202 floats and23,808 binary64 mask words exact. Evidence:
  validation/age_ppd60_639280954526250977/COMPLETED.json;pointer
  validation/age_ppd60_completed_path.txt. Original application binary is
  preserved in its verified_bin;raw-loading build is a later executable.
- Important genuine failure retained:first64/60ppd/age60 HDR ramp failed one
  final D pixel by1 ULP (5.5511151231257827e-17),raw D/earlier stages exact.
  abs(rawD)=0x30473d44,pf=0x3f7e96d5;old power0x305fbd82,native0x305fbd83.
  initial_failed64_age60_suite.txt pins failure;do not label it a pass.
- Higher polynomial degree was rejected after breaking4 random native cases.
  Accepted fix retains native-style cubic log/exp anduses triple-float
  expansion only near ambiguous normal-output rounding. No per-pixel/input
  lookup or tolerance. Generator emits additional table/constant residuals;
  helper tests/math_reference/power_triple_reference.glsl is currently local.
- Accepted power probes:65,536 random +65,536 boundary +65,536 neighboring
  counterexample pairs exact. Old shader fails exactly the neighborhood center.
  Ten old full-age exports replay on the corrected map shader with zero
  GPU differences (1,254,496 floats,52,352 double masks);this is replay,not
  ten fresh full application runs. All accepted/rejected artifacts preserved.
- Corrected map ABI13 SHA256:
  0B023987B9A2C9267B8979A072574DCC04AD8F56EA56CB1B7AACF164EA041BFF.
  Shader-only SPIR-V capability;no Float64/Int64. Original ABI13 SHA andABI12
  artifacts remain historical. Do not substitute hashes across checkpoints.
- Commands:finalize_full_age_checkpoint.ps1 -CrossPPD and
  finalize_raw_spectral_checkpoint.ps1 under validation/. Raw root stores
  full32_path/full64_path,verified_bin,build logs andnative oracle comparisons.
  See tests/RAW_SPECTRAL_VALIDATION.md andtests/FULL_AGE_VALIDATION.md.
- Mixed pipeline/GPU/power integration remains local/uncommitted;scoped raw
  loader implementation andverified tools/docs may be committed. Preserve
  existing staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,dirty
  lib/VkFFT,all old references anddirty/untracked files. No push performed.

The explicit32/64 float luminance input next step is completed above.
Normal public metric API,full RGB/color andgeneral dimensions remain open.

## Previous continuation: full age pipeline ABI13

- Calibrated L/M/R coefficients now reach CPU JND,GPU adaptation_input and
  GPU JND;observer age also reaches pupil,band CSF LUTs andbase CSF/noise.
  Native validators/checkers read pupil.txt age and spectral_lmr.txt instead
  of assuming24. Runner adds -ObserverAge and -SpectralSamples,with age-specific
  prefixes and input hashing. Accepted harness ages are24,60,60.001.
- Ten full pairs at30ppd,32/64,D65 synthetic luminance pass native Q/Q_JOD,
  strict preprocessing,CPU JND traces,raw/centered SPYR and GPU intermediates:
  1,254,496 float values,52,352 binary64 mask words,zero differences.
  Includes localized cases atall3 ages/bothsizes,stronger32 at60/60.001,
  andHDR edge phase shifts at60.001/bothsizes. Scope is pinned native
  resampled spectra,not independently loaded raw spectral data.
- Age24 regression:216 common text-export SHA256 hashes equal the previous
  accepted binaries (102 at32,114 at64). Detailed scores/equivalence and
  remaining work:tests/FULL_AGE_VALIDATION.md.
- Root:validation/full_age_639280300414897968/COMPLETED.json;pointer:
  validation/full_age_completed_path.txt. Finalizer:
  validation/finalize_full_age_checkpoint.ps1. Contains before/source snapshots,
  verified_bin executables,suite JSON,all cases/logs,age_source_hashes.json,
  age24_text_regression.json and fresh shader source/binary. Preserve these.
- Map shader ABI13 SHA256:
  871E3916717BFFCC49B4E59AD9E52EB311A148E277D99F32DCEAA8FE1C2FC0FE.
  Adaptation mode receives two appended coefficient words;JND's existing
  coefficient payload is parameterized. New host rejects oldABI12 shader.
  Base FFT ABI81 unchanged;SPIR-V valid with Shader capability only.
  Old ABI12 shader/executables remain saved;no blanket shader rebuild.
- Subsequent cross-domain60ppd/new-age tests andraw loading are completed
  above;the original10-case30ppd checkpoint retains its earlier shader hash.
- Mixed pipeline integration remains local/uncommitted. Preserve staged shader
  blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,dirty lib/VkFFT and all other
  changes. Scoped verified tools/docs can be committed;no push.

The cross-domain/raw-resampling next steps are completed above. Real HDR/color,
arbitrary full dimensions andpublic API remain unfinished.

## Previous continuation: exact age-dependent spectral calibration

- New independent C++ component:include/IQM/hdrvdp3/spectral.h and
  src/hdrvdp3/spectral.cpp. Input is native-resampled double wavelength,
  D65 emission and LMSR sensitivity. Computes optical-density nodes,pchip,
  transmission,filtered emission,trapz integrals and normalized LMSR matrix.
  Not connected to the main metric yet;no production defaults/shaders changed.
- All six stages match native v3.0.7 numerically in double across13 ages
  (15,19.999,20,24,28,32,45,60,60.001,75,83,83.001,100):16,822 compared
  values,zero differences. Includes the age60 lens branch and age32 zero
  correction. Age24 L/M/R coefficients equal the old constants exactly.
  Strict non-FMA standalone target spectral_age_test passes CTest.
- Evidence:validation/spectral_age_639280297796803042/COMPLETED.json;
  pointer validation/spectral_age_completed_path.txt. hashes.json pins code,
  native visual/loader/data files,reference samples and final executable.
  Initial/final stage logs,ctest_final.log and official_integration.log kept.
- Official loader actually emits420 wavelengths over360..780,not421 at1nm.
  Preserve the exact linspace grid. Raw spectral file interpolation has NOT
  been ported;new API consumes the official resampled fixture. Do not claim
  independent raw spectral loading or full observer-age metric parity.
- Native-only official hdrvdp3 quality calls at13 ages check26 printed LMSR
  matrices at15 decimal places and104 single coefficients;identical-image
  Q/Q_JOD are10. This is an oracle consistency check,not C++ full-age parity
  or full-double validation. The separate16822-value trace is the exact proof.
- Tools:generate_spectral_age_reference.m,verify_spectral_age.cpp,
  validate_spectral_age_official.m;independent tests/hdrvdp3_scalar target.
  Configure IQM_HDRVDP3_SPECTRAL_FIXTURE to the absolute fresh fixture path,
  build spectral_age_test and run CTest -R '^spectral_age_double$'.
  Details and source attribution:tests/SPECTRAL_AGE_VALIDATION.md.
- Preserve staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  dirty lib/VkFFT and all mixed changes. Only verified new component/tools/docs
  are eligible for this scoped commit;no push and no blanket shader rebuild.

Exact next step:pass calibrated L/M/R coefficients into CPU JND and GPU
adaptation_input/JND operations (currently fixed age24);retain exact default
regression. Propagate age into pupil and CSF/AESL plus validators,then run
fresh full metric cases at24,60,60.001 before broadening. A pinned native
resampled spectral fixture can bootstrap integration,but raw spectral data
loading/resampling remains an explicit separate task. Use fresh shader/export
directories and preserve ABI12 and all prior binaries if shader inputs change.

## Previous continuation: ordinary HDR60 and isolated age components

- Eight fresh ordinary HDR60 pairs are complete at32/64: ramp localized,
  edges localized, ramp brighter and edges stronger. All strict native
  stages, raw/centered SPYR, official Q/Q_JOD and GPU intermediate comparisons
  are exact. New comparisons:1,140,404 float values and47,616 binary64 mask
  words, zero differences. Eight actual exported input geometry checks pass.
  Root:validation/ppd60_hdr_standard_639280292107529786/COMPLETED.json;
  pointer:validation/ppd60_hdr_standard_completed_path.txt. Run the local
  validation/finalize_ppd60_checkpoint.ps1 -HDRStandard to recheck.
- All three viewing checkpoints now cover22 fresh pairs (20 at60ppd,two
  controls at30ppd),2,623,084 floats and109,440 double mask words. This is
  distinct from the older stress32 checkpoint. Do not count reruns as unique
  cases. tests/PPD60_VALIDATION.md and portable JSON retain separate records.
- Application and original checker hashes match the prior verified viewing
  checkpoint throughout HDR runs; neither executable was rebuilt. New pupil
  checker was built separately under the new age root. No shaders changed.
- Isolated pupil ages:0,15,19.999,20,24,28,45,60,60.001,75,83,83.001,100;
  sizes32/64,ppd30/60,five constant/ramp/random adaptation maps. Official
  geomean/pupil_d_unified vs GPU:260 cases,1,040 single trace values exact.
  Out-of-range ages exercise the pupil20..83 clamp,not full observer support.
- Scalar CSF/AESL across the same13 ages,on/off,12 frequencies,16 luminances:
  4,992 double-luminance plus4,992 single-luminance queries;both compared in
  double,all9,984 exact. Default24/45/70 generator regressions also pass
  1,152+1,152 double comparisons;these repeats are not new coverage.
- Age evidence:validation/pupil_age_639280292892783820/COMPLETED.json;
  pointer:validation/pupil_age_completed_path.txt. Saved source snapshots,
  final_hashes.json,build/configure logs,reference metadata and GPU/CSF logs
  pin the implementation and native sources. Accepted CSF fixture is
  csf_precise_age;first csf fixture retained,not counted twice. The generator
  now accepts optional comma-separated ages and exports ages at17 digits.
  Default historical age list remains unchanged. New generator is
  tests/generate_pupil_age_reference.m;local GPU checker adds --pupil-age.
- Read tests/OBSERVER_AGE_VALIDATION.md before extending full age support.
  Native quality enables do_aod/do_aesl/do_slum and disables do_aslw. The
  lens-density formula branches at60,then pchip/transmission/trapz/normalizing
  produces an age-dependent LMSR matrix. Existing C++ single JND and GPU
  shader still hardcode age24 D65 coefficients. CSF defaults also remain24.
  Full metric observer-age parity is NOT established by these component tests.
- Preserve existing mixed changes,dirty lib/VkFFT and staged shader blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7. Scoped commits cover only verified
  reference tools/docs;main integration and GPU checker remain local. No push.

The spectral calibration next step from this checkpoint is completed above
for native-resampled inputs. Full age integration,raw spectral loading,real
HDR/color,public API and arbitrary full-pipeline dimensions remain unfinished.

## Previous continuation: 32x32 stress/HDR and 60 ppd

This section supersedes older remaining-work statements below. Historical
measurements and failures are retained; do not treat old tolerance passes as
exact parity or combine repeated runs into unique case coverage.

- The 32x32 stress/HDR checkpoint is complete:
  `validation/stress32_639280137550795829/COMPLETED.json` (selected by
  `validation/stress32_completed_path.txt`). Seventeen fresh pairs cover six
  stress32, four HDR32, two HDR stress32, one corrected impulse32 and four
  HDR64 controls. All strict native stages, raw/centered SPYR and official
  scores match. Independent GPU comparisons: 1,653,818 floats and 68,928
  binary64 mask words, zero differences.
- Shape-aware test input generation fixes HDR ramp normalization, eight-level
  HDR edge tiling and off-center/right-edge impulse coordinates at 32x32.
  Eleven exported input geometry checks pass; eight 64x64 HDR luminance file
  hashes equal the earlier accepted controls. Canonical32 coverage is now
  18 labels across baseline/shape-aware binary groups, not one 18-case run.
  The obsolete one-impulse32 case is replaced. See tests/FFT32_VALIDATION.md.
- The new full-metric parameter path accepts IQM_PIX_PER_DEGREE=30/60;
  runner option -PixelsPerDegree sets it. It updates MTF, local adaptation,
  CSF and physical residual frequencies, and visual area=width*height/ppd^2.
  pixels_per_degree.txt is exported; the shared helper defaults old exports
  to30. Age24/D65 remain fixed. Size caps still produce four/five bands.
  60ppd requires FFT preprocessing and Vulkan maps; other values are rejected.
- Evidence root: `validation/ppd60_639280267907904462`, selected by
  validation/ppd60_work_path.txt. suite_paths.json records four localized
  size32/64 x ppd30/60 controls plus the six-case stress32/60ppd suite.
  PASS.txt and COMPLETED.json were issued by
  validation/finalize_ppd60_checkpoint.ps1: ten pairs, 912,478 float values,
  38,016 double mask words, zero differences. Both 30ppd controls retain
  the earlier scores exactly. tests/PPD60_VALIDATION.md records parameters,
  scores, layout, precision and DC equivalence.
- Commands: build IQM and the separate checker32_build target successfully,
  then tests/run_quality_validation.ps1 -InputSize 32/64 -PixelsPerDegree
  30/60 -CaseName asymmetric_localized, plus -Stress at32/60 (no CaseName).
  Use the same pinned math/map/base shaders as the earlier32 checkpoint and
  fft32_native_639280099461894780/shader64/fft64.spv for32. For64 use
  validation/fft_forward2d_initial.spv. Full commands and output logs remain
  in the root; saved candidate executables/source snapshots/hashes preserve
  this build. Do not rerun older binary-hash finalizers against overwritten
  executable paths; stress32 finalizer already maps to its saved binaries.
- Tests/summarize_quality_stress.m now records actual dimensions, band count,
  ppd and actual mask denominator. tests/validate_input_geometry.m independently
  checks HDR endpoints/levels and three impulse positions. These are reference
  tools; mixed application/GPU integration still remains local/uncommitted.
- Additional 60ppd HDR stress checkpoint is complete:
  validation/ppd60_hdr_639280272206510764/COMPLETED.json, selected by
  validation/ppd60_hdr_completed_path.txt. HDR edges/phase shift and HDR
  ramp/stronger localized distortion pass at both sizes (four pairs), adding
  570,202 exact float comparisons and 23,808 bitwise double mask comparisons.
  Four exported input geometry checks pass. Combined viewing checkpoints:
  14 fresh pairs, 1,482,680 floats, 61,824 double mask words, zero differences.
  Run validation/finalize_ppd60_checkpoint.ps1 -HDRCheckpoint to recheck.
  Actual scores, selected physical frequencies and commands are recorded in
  per-suite logs, all_cases.json and tests/PPD60_VALIDATION.md.
- Preserve staged corrdn_test shader index blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7 and dirty lib/VkFFT. No reset,
  clean, blanket shader rebuild or push. Only scoped verified tools/docs
  should be committed; earlier user authorization permits those commits.

The next-step statement from this checkpoint is superseded above:ordinary
HDR60 transforms and isolated pupil/CSF age checks are now complete. Full age,
real HDR/color loading,arbitrary full-pipeline dimensions,public API and
remaining CPU preparation remain unfinished. No measured total completion
percentage is available.

## Current local checkpoint: six full32x32 pairs plus64x64 regressions

-2026-10-19:implemented and validated32x32 MTF/local adaptation using64x64
  reference FFT,then extended the full test pipeline to four bands. Six32
  pairs andsix64 regressions pass direct official Q/Q_JOD and strict stages.
  No push. Mixed GPU/application integration remains local/uncommitted;
  reference tools/docs are eligible for a separate scoped checkpoint.
-32 pair Q/Q_JOD:asymmetric_localized8.3328084945678711/8.9990530014038086;
  stronger9.9952077865600586/9.9994449615478516;brighter
  9.9999408721923828/9.9999980926513672;diagonal9.9999361038208008/
  9.9999980926513672;impulse andidentical10/10. All equal official outputs.
  Independent32 GPU checks:6*57046=342276 float values,6*2368=14208 double
  masks,zero differences.64 regressions:1368330 float,57216 double,zero.
  Raw/native SPYR and centered residual logs also explicitly checked.
- Isolated native scalar FFT64 shader ABI6401:four forward/inverse transforms,
  32768 float components exact. Graphs r2cf_64/n1_64 from pinned FFTW3.3.10
  GPL sources;original C files and COPYING retained in artifact shader folder.
  SHA256=8F339ADF6FF03BB5E1D44EF1D767682ABDF136C7E82EF1C9FD30C719CAAD5460.
  Only Shader SPIR-V capability;no float64/int64. Same graph selectors8/9 and
  3/4 as reference128,with64 dimensions and inverse division by4096 once.
- Native DLL inspection accepts optional FFT side64/128,default128. '-' wisdom
  argument means fresh estimate. Native scalar/unaligned one/three-thread
  plans match eight comparisons;aligned AVX2 plans differ in eight. Native
  wisdom import failed with registry-header checksum mismatch,even after
  initializing thread count first. Loaded DLL path/version verified against
  the running Octave process. No wisdom rewrite/forced import. Root cause
  remains unresolved;direct scalar numerical comparisons establish the path.
- LocalAdaptFFTInput width/height default64,explicit32/64 square guards;
  32 requires reference backend. Scratch/dispatch/crop/trace/kernel sizes now
  use dimensions. Main test compute_fft_filter_gpu infers32/64 square input.
  IQM_FFT_REFERENCE_SHADER64 is separate from original128 shader path.
- Isolated MTF+local adaptation now exact for asymmetric/impulse/HDR ramp at32
  and asymmetric64:1075200 float comparisons total,zero differences. Shared
  binary32 luminance,MTF symmetric post padding,LA replicate post padding,
  native L+M/log/exp,sigma10^-0.781367*30 and column-major interleaved FFT.
  Full input32 uses area1024/900 and residual8x8 with7.5ppd base sampling.
- Full wrapper IQM_SPYR_SIZE=32/64;localized patch x=width/2..width/2+7,
  retaining y13..20. band_count.txt records4/5. Validators/checker discover
  this count;old exports default5. Quality score indexing/pooling and physical
  base frequency are dynamic. Runner adds-InputSize/-GPUChecker,32 prefixes,
  and clears isolated probe flags. Default64 behavior passed six regressions.
- First32 application matched official Q but checker retained literal band4
  filename;fixed to dynamic b. Preserved failure,then fresh checker build and
  fresh six-case32 suite. Concurrent64 checker was not overwritten. New32
  suite:quality_gpu_base_fft_suite_32_639280130799228493;64:
  quality_gpu_base_fft_suite_639280129214275004 under validation/.
- Evidence:validation/fft32_native_639280099461894780/COMPLETED.json,PASS.txt,
  full_suite_paths.json,final_source_and_binary_hashes.json,all failed/success
  logs and prior/accepted binaries. validation/fft32_completed_path.txt selects
  it. finalize_fft32_checkpoint.ps1 checks all counts,raw/DC SPYR logs,score
  equality,source/binary hashes and Vulkan logs before completion. Updated
  portable JSON retains older18-pair/dimension checkpoints separately.
- New tools:generate_fft_size_reference.m,prepare_fft_size_gpu_reference.m,
  generate_fft64_reference_shader.ps1,validate_fft_size_plans.m,
  validate_fft_size_pipeline.m,hdrvdp_validation_band_count.m;native inspector
  and codelet checker add64 support. Root artifacts include exact commands and
  metadata;generation requires fresh folders. See tests/FFT32_VALIDATION.md.
- Added independently publishable tests/hdrvdp3_fft64 CMake project and
  verify_fft64_reference.cpp (one64x64 matrix,strict equality). A clean HEAD
  archive plus only these selected sources and updated native inspector builds
  both tools and passes all four GPU transforms/eight native scalar comparisons.
  CANDIDATE_PASS.txt and fft64_candidate_* folders retain that proof. This
  avoids committing the pre-existing mixed changes in fft_codelet_gpu_test.cpp,
  the main wrapper,LocalAdaptFFT and the general test CMake project. Published
  tools do not require those uncommitted integration changes.
- Exact next step:add32 phase_shift/localized_stronger/threshold comparisons
  using the new runner,keeping fresh exports. Before full-HDR32/three-impulse
  metric coverage,fix main wrapper's315 HDR ramp denominator and64-specific
  impulse coordinates;separate preprocessing generators already scale bysize.
  Then viewing parameters,real HDR/color,public API/other modes and CPU double
  preparation remain.32 VkFFT and arbitrary rectangles remain unsupported.
- Preserve staged shader1cf7baedb1d54be0ca8ad02d14485e510b0434e7,all pre-existing
  changes/untracked artifacts and dirty lib/VkFFT. Existing scalar commits and
  e65799b docs/tools remain local;no push in this continuation.

## Historical local checkpoint: generalized raw SPYR and32x32 comparison masks

- Continued without requesting approval. Existing local commits51525e0,
  7ce32a0 andf0ce345 are preserved;no push in this continuation. New verified
  documentation/standalone test tools are scoped separately from mixed GPU
  integration. Do not commit whole wrapper/CMake/map sources without reviewing
  their dependencies;the committed wrapper still has the old callback API.
- Nine raw-SPYR shapes at30ppd:32x32,48x48,64x64,96x96,128x128,49x65,64x96,
  65x49,96x64. Three inputs:asymmetric modulo,edge/off-center impulses and
  deterministic random. All27 cases/360816 raw float values match official
  Octave numerically exactly;3918 separately DC-centered residual values also
  match. Float signed-zero bits are not checked. No Vulkan errors/VUIDs.
- Important findings:official pyramid height is min(ceil(log2(ppd))-2,
  maxPyrHt(...)),not solely maxPyrHt. Add optional maximum_height to Spyr::compute;
  probe explicitly passes3 at30ppd. Raw128x128 now produces five bands,not six.
  At32x32/48x48 there are four bands;at49x65 residual is13x17.
- Native installed single conv2 traverses the longer image axis inside the
  tap loop. FMA applies to32-element vector blocks;the remaining coordinates
  use separate product/add rounding. Prior level2-only non-FMA logic was valid
  only for64x64. Diagnostics measure these paths independently of GPU output.
- Keep FMA/non-FMA in SEPARATE compiled pipelines. Three mixed/dual-accumulator
  shader candidates broke even64x64 despite explicit fma/precise;their failed
  exports/SPIR-V are retained. Accepted common shader compiles normally and
  with-DIQM_CORRDN_NON_FMA. New generated include:
  shaders_out/hdrvdp3/hdrvdp3_corrdn_native_tail.inc. Existing test shader source,
  its index blob and prior generated test include are preserved. Source CorrDn
  now includes native_tail.inc. Use tests/compile_corrdn_reference.ps1 with a
  fresh OutputDirectory,then explicitly install the two includes after keeping
  prior binaries. The blanket shader script does not generate this new include
  and deletes shaders_out;do not run it.
- Wrapper run_corrdn_gpu runs a second GPU pass for vector tails and selects
  already-rounded outputs on the host by pre-subsampling coordinate. No CPU
  convolution arithmetic. Staging buffers now size to actual dimensions with
  unsigned-allocation bound checks. Full metric remains64x64;only the isolated
  IQM_SPYR_DIMENSION_PROBE=1 branch exercises these nine shapes. Set
  IQM_SPYR_PROBE_PATTERN=asymmetric/impulse/random and unset probe for metric runs.
- Raw evidence:validation/spyr_dimensions_17923555764323481,
  spyr_dimensions_17923555832003632,spyr_dimensions_17923555942649684.
  compare with tests/validate_spyr_dimensions.m;arithmetic probes are
  tests/diagnose_spyr_dimensions.m and diagnose_spyr_rectangular_order.m.
- After accepted SPYR pipelines,eight fresh full64x64 metric pairs pass
  preprocessing,JND,CSF,masking,D,noise and direct official Q/Q_JOD:original6
  plusHDRstress2. Independent checks:1824440 float values,76288 binary64 mask
  values,zero differences. Suites under validation/:
  quality_gpu_base_fft_suite_639279523931956826 and
  quality_stress_hdr_gpu_base_fft_suite_639279524032176925. Same application/
  original checker binaries;no Vulkan errors/VUIDs. Earlier18-pair evidence
  remains historical and separate;do not claim18 new pairs after this change.
- Next bounded component completed:MapGPU comparison-mask resize accepts
  square32x32 binary input ->32/16/8,retains64->64/32/16/8. Replace fixed4096
  plan offset with actual h*w. Unit normalization,Q16,L1<=2 and signedQ32
  guards remain. Unchanged map ABI12 shader. A separate checker build passes
  192 cases/86016 binary64 outputs at32 and256 cases/348160 at64,all bitwise
  equal. New checker also independently rechecks all eight full-case exports:
  again74/228055 float and5/9536 double per pair,zero differences.
- Mask evidence:validation/mask32_639279525250877995. Application used for eight
  full regressions predates mask32 host change;new checker is separate under
  this evidence directory. Do not confuse it with validation/fft_publish_build
  checker or claim a32x32 full-metric result. Original IQM/checker binaries
  and source-before-mask32 hashes are preserved;main IQM has NOT been rebuilt
  to link mask32 support yet. Next build must preserve accepted executables.
- Aggregate evidence:validation/spyr_dimension_checkpoint_639280047991477467/
  COMPLETED.json and PASS.txt;spyr_dimension_completed_path.txt selects it.
  Includes source snapshot/hashes and accepted IQM/new checker executables.
  validation/finalize_dimension_checkpoint.ps1 validates all counts/logs and
  binary manifests. Cast Get-Content strings before nested ConvertTo-Json:
  otherwise PS5 serializes extended path/drive metadata very slowly. The first
  metadata attempt was interrupted;its partial directory is retained.
- Updated tests/SPYR_DIMENSION_VALIDATION.md and portable validation summary
  with a separate dimension_checkpoint,retaining previous18-case scope/hashes.
- Exact next step:implement32x32 MTF/local-adaptation with64x64 FFT. Current
  LocalAdaptFFTInput lacks dimensions;local_adapt.cpp fixes128 FFT and64 crop/
  preparation. Capture native64 FFT plans/codelets and intermediate maps first;
  parameterize dimensions with explicit guards,update reference math shaders,
  then make validators/checker discover four bands instead of fixed five.
  Existing8x8 base FFT/DC remains applicable to32x32/30ppd. General rectangular
  masking and non-8x8 residual FFT/DC remain unsupported. Raw SPYR success
  alone does not validate their full metric. Other pending work:real HDR/color,
  viewing parameters,CPU double preparation,public API and other outputs.
  Overall approximately75/100 remains an engineering estimate,not coverage.
- Preserve staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  its working-tree edits,all untracked artifacts and dirty lib/VkFFT.

## Historical checkpoint: committed scalar parity and eighteen fresh pipeline pairs

2026-10-18: user authorized scoped commits of verified work. Two local code
commits now exist; neither was pushed in this session:

- `51525e01f1f4230ba8f6e6b7d0e323f8d4fdf4b6`: direct CSF/resize parity,
  official generators, strict scalar checks and standalone CMake project.
- `7ce32a01f814c3f925f5f85314aedaf7333161e6`: exact double CSF LUT grid,
  strict double LUT test and independent knot/boundary/random generator.
- Reviewed candidate files were copied into an isolated source tree before
  commit. Both actual Git commits were then exported using git archive and
  built independently. Final committed-source CTest:8/8 pass. The scalar
  project has no Vulkan or mixed-workspace integration dependency.
- Direct CSF equation division order removes all15675 sampled double CSF/noise
  differences; age24/45/70 and AESL on/off queries also pass. CPU resize has
  45 cases/32643 values exact in both single and double.
- LUT log exponents now use the symmetric fixed256-node grid, rather than
  single-endpoint interpolation. This removes539 double node and3194 double
  query differences. Current3840 nodes/22530 queries have zero double/float
  differences. Additional42300 boundary,neighbor,clipping and random queries
  also have zero differences using the executable built from the commit.
- Eighteen fresh full-pipeline pairs all pass with the SAME application and
  independent GPU-checker binaries after the LUT fix. Four separate suites
  cover original6,HDR4,stress6,HDRstress2. All Q/Q_JOD equal official outputs;
  4104990 float values and171648 binary64 mask words match exactly. This is
  exact sampled parity,not a tolerance pass or a claim of general coverage.
- No Vulkan errors/VUIDs in eighteen application or GPU-checker logs. Shader
  map ABI12/base FFT ABI81 and their hashes are unchanged. Source snapshots
  verified unchanged during all runs;all suite binary manifests match.
- Current evidence:validation/scalar_checkpoint_639279508489310995 and
  validation/lut_grid_regression_639279511058157010,each with COMPLETED.json.
  completed_path pointers select them;the latter suite_paths.json selects
  all four fresh suites. Existing map/stress historical checkpoints remain.
- Input equivalence:both paths use the same exported binary32 luminance;
  strict preprocessing/layout,padding,parameters,DC and native column-major
  frequency selection checks remain enabled. Binary masks compare IEEE
  binary64 words;DC removal is confined to the reference-defined residual
  processing,not applied to hide errors in arbitrary bands.
- Commands:cmake build/ctest for tests/hdrvdp3_scalar;four existing
  run_quality_validation.ps1 invocations with ReferenceShader,MathDir,
  MapShader,BaseFFTShader and optional HDR/Stress. Exact artifacts/logs and
  hashes are retained. validation/finalize_scalar_lut_checkpoint.ps1 checks
  all eighteen case logs,source/binary manifests and committed scalar logs
  before generating the current metadata. PowerShell uses process-local
  ExecutionPolicy Bypass for local scripts;no machine setting was changed.
- GPU/integration work remains mixed and uncommitted;these code commits only
  publish the independently buildable scalar subset locally. Preserve staged
  corrdn_test shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  its unstaged edits,all untracked artifacts and dirty lib/VkFFT.
- Next exact step:generalize the full-pipeline dimension harness beyond64x64,
  starting with one square size supported by native pyramid depth. First
  inspect hard-coded64/8 assumptions and explicit backend guards in
  map_gpu.cpp,verify_map_gpu.cpp and the reference shader. Keep unsupported
  dimensions guarded;record official band shapes/padding/DC before changing
  dispatch or FFT/resize code. Retain the eighteen-case suite as regression.
  Remaining CPU preparation includes double nCSF/MTF/AESL/noise/power and
  physical frequency;calibration/plans and readbacks also remain on CPU.
  Real HDR/color,viewing parameters,public API and other output modes are
  unvalidated/incomplete. Overall completion is only an engineering estimate,
  approximately75/100;no measured whole-metric coverage percentage exists.

## Historical local checkpoint: ABI12 plus eight stronger/threshold comparisons

2026-10-18: continued reference comparison with unchanged metric/map ABI12
and base FFT ABI81. Only test input branches,Stress runner and native summary
were added. Original ten accepted pairs remain a separate core checkpoint.

- Eight new pairs:sinusoidal_x/y phase_shift,asymmetric localized_stronger,
  asymmetric threshold_below/nominal/above,hdr_edges phase_shift and hdr_ramp
  localized_stronger. Phase shift wraps x/y by8;localized_stronger multiplies
  the existing8x8 patch by2. Threshold gains are nextafter neighbors of1.001f.
  C++ single exported luminances are the common reference input.
- All eight pass strict preprocessing/JND/CSF/masking/raw/final D,double noise,
  bitwise double masks and direct official Q/Q_JOD. New GPU comparisons:
  1824440 single values,76288 double mask bit patterns,zero differences.
  No Vulkan errors/VUIDs in eight run or independent GPU logs.
- Example exact Q/Q_JOD:sinusoidal_x5.3645796775817871/6.2898049354553223;
  sinusoidal_y5.3645696640014648/6.2897944450378418;asymmetric localized2x
  3.6115429401397705/4.4040126800537109;HDR edges shift
  3.4347231388092041/4.204805850982666. Quality validation now includes low Q.
- Native base selections:x/y periodic indices17/3 (one-based column-major),
  both0.9375cyc/deg;threshold cases index33/frequency1.875;HDR shift index10/
  frequency0.66291260736238833. This broadens end-to-end frequency coverage.
- Native active masks for threshold below/nominal/above:0/4056/4096 pixels.
  Nominal gain does not imply each rounded ratio is exactly0.001.
- Fresh canonical asymmetric/localized control matches58 selected numeric text
  files from the earlier ABI12 export by SHA256. Original core binary is
  preserved in the new stress evidence directory;shader was not regenerated.
- Evidence:validation/quality_stress_639279178899870989/COMPLETED.json,PASS.txt,
  case_summary.json/log,CONTROL_PASS.txt,COMMANDS.txt,source/binary hashes.
  New suites:quality_stress_gpu_base_fft_suite_639279179397567819 and
  quality_stress_hdr_gpu_base_fft_suite_639279179398732111 under validation/.
  quality_stress_completed_path.txt selects new evidence;map final/completion
  pointers still select the original ABI12 core checkpoint,not the stress set.
- Updated tests/HDRVDP3_VALIDATION_SUMMARY.json locally to ABI12,18 cases with
  separate original_10/stress_8 groups,relative paths and remaining-work list.
  Combined groups:4104990 single values,171648 double masks,zero differences.
  These are separate accepted runs/binaries;do not describe18 fresh runs of one
  binary. Published remote summary remains ABI8;no commit/push performed.
- Source changes:input-only wrapper branches,two extra sinusoidal patterns,
  phase/strong-localized/threshold transforms;run_quality_validation.ps1 adds
  -Stress with separate prefixes;new summarize_quality_stress.m;validation docs
  and portable JSON. A malformed JSON-array append was detected and corrected
  before acceptance;the earlier diagnostic manifest is retained. All final
  source hashes and native eight-case summaries were validated explicitly.
- Remaining CPU and metric work unchanged:double CSF/MTF/AESL/noise/power,
  physical frequency,calibration/plans,diagnostics;general dimensions/viewing,
  real HDR/color,public API and other outputs. Next:continue double preparation
  port or parameter/dimension generalization using these stronger regressions.
  Full metric scope is still64x64/30ppd/age24/D65 positive synthetic luminance.
- Preserve mixed work,staged shader1cf7baedb1d54be0ca8ad02d14485e510b0434e7
  and dirty VkFFT. End-of-session handoff updated;no permission is needed for
  routine authorized continuation.

## Historical local checkpoint: ABI12 GPU exact double comparison-mask resize

2026-10-18: supported64x64 binary comparison masks now resize on GPU for
square64/32/16/8 outputs. Horizontal Q16 and vertical signed Q32 integer
convolution reproduce native double results exactly; output is IEEE binary64
low/high words,bitcast on host. CPU still prepares bicubic/symmetric plans.
Guards require unit normalization,exact Q16 weights and L1<=2 per destination.
Horizontal magnitude<=2*2^16,vertical<=2^34:all native double products/sums are
exact within53-bit precision. No approximate float resize/pair/tolerance used.
Negative bicubic lobes/overshoots are retained. General double resize remains
CPU code; full-metric scope still64x64/30ppd/age24/D65 positive luminance.

- Independent native fixture:64 fields x4 sizes,256 cases/348160 binary64
  outputs,0 bit differences. Covers zero/one,corners/center impulses,checker,
  directional edges/stripes and random densities. Final explicit count is in
  mask_resize_bitwise.log (initial log used a generic float label).
- Ten fresh full chains match official Q/Q_JOD and all earlier strict stages,
  including exact double noise. GPU checks retain74 float comparisons/228055
  values per pair and add5 binary64 comparisons/9536 values per pair. Totals:
  2280550 single values and95360 binary64 mask bit patterns,zero differences.
  Both highpass/orientation first bands are64x64,explaining the mask count.
- Actual pipeline quality_mask_bandN.txt exports use17 digits and are compared
  with native imresize before quality pooling. The independent checker also
  compares GPU masks bit-for-bit with native exports. New --maps checks require
  fresh quality_mask_native_bandN.txt files; do not reuse older incomplete exports.
- Fresh CPU maps/default DFT fallback passes raw/final D,exact double noise,
  double masks and official Q/Q_JOD. Unsupported shape/nonsquare/nonbinary masks
  and old ABI11 shader reject with exit2. No Vulkan errors/VUIDs in ten run or
  GPU-checker logs. Shader:200 NoContraction,no Float64/Int64;base FFT still81.
- Evidence:validation/mask_resize_gpu_639279174670035352/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source/binary hashes,shader metadata,all native/bitwise/guard/
  fallback/full-suite logs. Previous ABI11 executables are preserved there.
  Suites:quality_gpu_base_fft_suite_639279175960765914 and
  quality_hdr_gpu_base_fft_suite_639279175961596220 under validation/.
  Current final shader/completion pointers select ABI12.
- Changed:map_gpu.h/.cpp,shader template,wrapper(GPU branch plus templated
  float/double export preserving9/17 digits),native quality validator,checker,
  suite backend guard,new generate_binary_mask_resize_reference.m and docs.
  State::execute_words preserves raw integer/double payloads;float execute is
  a memcpy wrapper. Numerical sources remained unchanged during final suites.
- Remaining CPU:double nCSF/MTF/AESL evaluation,noise reciprocal/scalar double
  power,physical double frequency,calibration/filter/resize plans,diagnostics.
  Next:port remaining double preparation or generalize dimensions/conditions
  while retaining strict native/bitwise checks. Real HDR/color,public metric
  API and other outputs remain unfinished. No percentage/completion claim.
- No commit/push performed;published summary JSON still ABI8,refresh it on
  publication. Existing staged shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved. Handoff updated.

## Historical local checkpoint: ABI11 GPU CSF parameters; double order fixed

2026-10-18: the remaining measured double CSF/noise mismatch was caused by
reciprocal/multiply rewrites. evaluate_ncsf now preserves the reference's
term1/pow(term2,p3) and p4/denominator divisions. Before/after binaries on the
same15675-case grid demonstrate5146->0 double CSF differences and4933->0
double noise differences. This is exact equality,not a tolerance/float-only pass.

- Map ABI11 mode22/base_csf_parameters moves single log10,clamp,upper-bound
  lookup and four parameter interpolations onto GPU. CPU prepares the fixed
  7-node/28-coefficient/24-slope LUT. Input luminance domain[1e-5,1e5],finite.
  Trace is5 floats/query. GPU fixture:1045 queries/5225 values,zero differences;
  downstream15675 double noise and single neural coefficients also exact.
- Strict regression modes:IQM-csf-reference-test --single-luminance-strict and
  IQM-base-noise-test --strict-double fail on any double difference. Existing
  1152-case age24/45/70,AESL on/off double-luminance and single-luminance fixtures
  both match exact double results. This does not broaden full-metric scope.
- Ten fresh full chains match official Q/Q_JOD,all maps and exact base double
  noise. Native D validation now asserts noise equality (expected Inf at zero
  frequency included). Independent GPU:74 comparisons/228055 values per pair,
  2280550 total,zero differences. Pipeline trace/backend also checked.
- Fresh CPU maps/default DFT localized fallback passes raw/final D,double noise
  and official Q/Q_JOD. Unsupported zero/below/above luminances and obsolete
  ABI10 shader reject with exit2. No Vulkan validation errors/VUIDs in ten run
  logs. Shader:200 NoContraction,no Float64/Int64; base FFT remains ABI81.
- Evidence:validation/base_csf_order_639279166321142223/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source_hashes.json,binary_hashes.json,strict/before-after/GPU/
  age/fallback/guard logs,old csf_before.cpp and preserved ABI10 executables.
  Suites:quality_gpu_base_fft_suite_639279167705195338 and
  quality_hdr_gpu_base_fft_suite_639279167713649747 under validation/.
  Final map/completion pointers now select ABI11. Previous artifacts preserved.
- Changed:csf.h/.cpp(CPU operation order plus calibration/evaluation helpers),
  map_gpu.h/.cpp,map shader template,wrapper,verify_map_gpu.cpp,native CSF/D
  generators and validator,strict CSF/noise checkers,suite runner and validation
  document. Numerical source hashes remained unchanged during final suites.
  No commit/push performed; portable published JSON still describes ABI8 and
  should be refreshed when publishing. Staged shader/VkFFT untouched.
- Remaining CPU:double nCSF/MTF/AESL evaluation,noise reciprocal/scalar double
  power,physical double frequency,calibration/plans,double mask resize and
  diagnostics. Next: independently port the remaining double preparation or
  tackle parameter/dimension generalization without losing these strict checks.
  Full scope remains64x64/30ppd/age24/D65 positive luminance;real HDR/color,
  public API and other outputs remain unfinished. No completion percentage.
- End-of-session handoff updated per latest AGENTS. Preserve mixed work and
  staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7.

## Historical local checkpoint: ABI10 GPU rounded norm and first maximum

2026-10-18: finite_hypot uses exact96-bit integer-limb squared/midpoint
comparisons to reproduce binary32 nearest/ties-to-even norm. GPU mode21
suppresses DC,stores magnitudes row-major and chooses the first maximum in
column-major order. ABI81 FFT + ABI10 maps now remove CPU hypot/index selection
from the optional GPU FFT route. Physical frequency remains double CPU work.
No Float64/Int64 capability;183 NoContraction. Domain:8x8 spectrum,zero or finite
normal components of unbiased exponent[-90,100]; unsupported inputs throw.

- Independent final fixture:1024 arbitrary complex spectra,65536 magnitudes
  and1024 indices exactly match Octave. Includes exact Pythagorean midpoint
  ties,rounded unequal-norm ties,DC-only/zero,signs and broad exponents.
- Existing64 native FFT cases pass8192 spectral values and GPU-selected
  indices/frequencies;512 adaptation cases still pass1024 scalar values.
  Exponent-91/101 and obsolete ABI9 shader guards reject with exit2.
- Ten fresh full chains match official Q/Q_JOD exactly. Independent GPU:
  73 comparisons/228050 values per pair,2280500 values total,zero differences.
  Native magnitude and actual pipeline index/backend are checked explicitly.
  No Vulkan validation errors/VUIDs in the ten run logs. CPU fallback passes.
- Additional base CSF/noise audit:15 frequencies x1045 luminances at age24,
  15675 cases. CSF single differences0,double differences5146. Noise double
  differences4933,max finite delta1.3322676295501878e-15. Final single neural
  coefficient differences0. Double CSF/noise parity remains UNRESOLVED; do not
  describe the entire scalar preparation chain as double-exact.
- Evidence: validation/map_gpu_abi10_639279156150953405/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source/binary hashes,shader metadata and all fixture/guard/logs.
  Suites: quality_gpu_base_fft_suite_639279157199426535 and
  quality_hdr_gpu_base_fft_suite_639279157317562109 under validation/.
  Final map/completion pointers select ABI10; base FFT remains ABI81.
  Earlier ABI9 executables are preserved in the ABI10 directory.
- New sources:tests/math_reference/hypot_reference.glsl,
  tests/generate_base_selection_reference.m,tests/generate_base_csf_reference.m,
  tests/verify_base_noise.cpp. Existing map generator/template/host,quality
  helper,wrapper,checker,native D validation and suite runner were updated.
  Only a diagnostic CMake target was added after full suites; the earlier
  source hash snapshot is preserved separately. Numerical sources did not
  change after the full-suite builds. No commit/push performed.
- Next: resolve/port base CSF/noise/scalar double power while retaining exact
  final float coefficients. CPU frequency/calibration/plans/double mask resize
  and diagnostics remain. General dimensions/viewing/color/real HDR,public API
  and other outputs remain unfinished. Scope is still64x64/30ppd/age24/D65.
- Preserve all mixed work/staged shader/dirty VkFFT. Shader staged blob:
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7. Published portable summary still
  describes ABI8; refresh on publication. End-of-session handoff updated.

## Historical local checkpoint: ABI9 base geometric adaptation

2026-10-18: base mean/power10 now execute on GPU (mode20), with separately
rounded single mean and source-backed native power. The GPU branch no longer
computes their CPU equivalents. Shader ABI9 rejects obsolete ABI8 maps; the
optional base FFT remains ABI81. No Float64 capability;183 NoContraction.

- Independent native fixture:512 cases/1024 values,zero differences. Tests cover
  constants, adjacent floats, signed cancellation, small and random fields.
- Ten fresh full-chain synthetic pairs match official v3.0.7 Q/Q_JOD exactly.
  Independent GPU checks:72 comparisons/227986 values each,2279860 total,zero
  differences. Includes native mean-log and geometric adaptation outputs.
  Existing64x64/30ppd/age24/D65 positive-luminance scope remains unchanged.
- Fresh CPU maps/default DFT localized fallback also matches official scores.
  Out-of-range/span/tiny inputs and old ABI8 shader reject with exit2. No Vulkan
  validation errors or VUIDs occur in the ten completed pipeline logs.
- Local evidence: validation/map_gpu_abi9_639279140029974208/COMPLETED.json,
  PASS.txt,COMMANDS.txt,source_hashes.json,binary_hashes.json,shader_metadata.json.
  Suites: quality_gpu_base_fft_suite_639279141141654008 and
  quality_hdr_gpu_base_fft_suite_639279141707759061 under validation/.
  Current final shader/completion pointers select ABI9. Previous ABI8 executables
  were preserved in the ABI9 evidence directory before rebuilding.
- Domain:64 finite single logs in[-5,5],nonzero exponent span<=16,minimum>=-90;
  zeros allowed. Unsupported inputs throw. The initial native fixture included
  outward +/-5 neighbors; corrected to inward neighbors,old failure retained.
  Native stderr informational output required Continue during one fixture run;
  the final retry completed successfully. All initial logs remain preserved.
- Changed: map_gpu.h/.cpp,map shader template,wrapper base-mean branch,
  verify_map_gpu.cpp,validate_difference_maps.m,new native fixture generator,
  tests/MAP_GPU_VALIDATION.md. All implementation remains local/uncommitted.
  No commit or push was performed. Staged corrdn-test shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved.
- Next: independently validate/port base hypot and first-max selection or base
  CSF/noise/scalar double power. CPU calibration/plans,double mask resize and
  diagnostics also remain. Then generalize dimensions/viewing/color/real HDR,
  expose the public API and implement remaining outputs. Published summary JSON
  still documents the historical ABI8 checkpoint; refresh it when publishing.
- Per the latest AGENTS instructions (2026-10-18),this handoff is updated at the
  end of the session. Preserve historical evidence and mixed working changes.

## Historical published checkpoint: ABI8 maps and ABI81 native-matching base FFT

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
