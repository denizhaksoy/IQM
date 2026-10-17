# Vulkan CSF, resize and masking maps

> Publication scope: this document describes locally validated work. The implementation, new test tools and full artifacts are not included in this documentation checkpoint. See SESSION_HANDOFF.md at the repository root for current status; earlier measurements below retain their historical scope.


`MapGPU` is an opt-in, synchronous numerical-parity backend. It executes
single-precision adaptation/pupil/JND, bicubic resize, CSF LUT sampling, masking/D
and quality pooling/scores in Vulkan compute. Default behavior remains CPU maps. Filter weights
and CSF LUT/slopes are prepared on the CPU in double precision, then packed
as single coefficients; this is parameter preparation, not per-pixel GPU work.

## Numerical behavior

- Resize uses the same CPU-generated double coordinate/weight plan as the
  validated CPU implementation. Horizontal filtering precedes vertical
  filtering; symmetric boundaries, antialiasing, zero taps and accumulation
  order follow installed Octave image-2.20.0. Single products and sums are
  explicitly separate. The refactored CPU resize still passes45 cases/32643
  pixels in both float and double against independent Octave fixtures.
- Resize normalization uses integer significands and ties-to-even division.
  This avoids relying on the GPU's relaxed floating division. The current
  image tests use finite, normal intermediates; they do not establish a full
  NaN/Inf/subnormal arithmetic contract for the complete shader pipeline.
- CSF lookup uses single-rounded nodes for interval selection, double-derived
  slopes rounded to single, and separate single offset/product/addition.
  GLSL precise decorations prevent contraction. LUT construction remains CPU.
- Resized CSF log maps are explicitly clamped to[-5,5] before both LUT
  sampling and the base-band geometric mean. The old prototype only clamped
  the sampler query. This matters when bicubic interpolation overshoots a
  luminance boundary; the final GPU backend performs the map clamp as well.
- Raw masking normalizes by the pyramid power-of-two factor and takes the
  minimum of the absolute reference/test amplitudes. Supported normalization
  factors are1,2,4,8,16. ABI8 now removes residual DC on GPU within its
  explicitly guarded64-sample domain; earlier ABI3/7 runs used CPU removal.
- Pooling uses zero padding and the native tap traversal. The pinned square
  bands64/32 use one-rounding multiply-add;16/8 use separate multiply/add.
  Other pooling shapes are explicitly rejected rather than assumed equivalent.
- Native shader fma() differed in2788 pooled outputs of the first localized
  export on this Intel Arc device. The final shader instead uses unsigned
  integer significands, jammed alignment and ties-to-even rounding for the
  positive pooling multiply-add. This is not a general signed/special-value
  replacement for fma. Historical failed shader artifacts remain preserved.
- A SPIR-V specialization constant with SpecId31/default3 marks shader ABI3.
  The host verifies it before creating the pipeline; old CSF-only binaries are
  rejected before a newer masking dispatch can misinterpret their parameters.

Buffers use explicit host-write/compute-read and compute-write/host-read
barriers and a real fence per dispatch. Each dispatch returns a host vector;
GPU-resident chaining/performance changes are intentionally deferred. Images
and tables have explicit uint32/float32 word layout, with bitcasts for floats.

## Standalone results

Use ONLY the current ABI3 shader pointed to by
validation/map_gpu_final_shader_path.txt. Earlier shader pointers are historical
and are intentionally not rewritten to pretend they were the final binary.

| Independent check | Compared values | Result |
| --- | ---: | --- |
| GPU resize vs native Octave,45 cases | 32643 | Exact float32 |
| GPU CSF,15 tables, reference and production LUTs | 45060 GPU queries | Exact float32 |
| Same CSF fixture, CPU regression | 22530 queries | Exact float32 |
| GPU raw activity and pooling,24 input pairs | 65280 | Exact float32 |
| Resize/clamp/CSF boundary fixture,10 cases | 43564 | Exact float32 |

The pool fixture uses deterministic positive log-uniform magnitudes1e-20..1e20,
sparse impulses, near-unit signed inputs, zero fields, uniform10000 fields and
random signed fields, across64/32/16/8 square dimensions. These are synthetic
pyramid inputs, not claims of that HDR luminance range for the full metric.

The boundary fixture uses log maps at both LUT limits, steps/checkerboards,
four pyramid scales and a noninteger enlargement. Native imresize overshoots
the range at4136 pixels; all clipped maps and resulting CSF values match.

Historical ABI3 evidence: validation/map_gpu_abi3_{resize,csf,pool}.log,
map_gpu_csf_bounds.log, csf_bounds_reference_path.txt,
pool_reference_path.txt, map_gpu_abi3_reject.log,
map_gpu_abi2_rejected_by_abi3.log and matching build logs.
Earlier ABI2 results remain historical evidence for the pre-clamp kernel.
The initial pool test was accidentally launched before linking finished and
prevented that link; two exit2 logs are marked invalid by
map_gpu_pool_fixture_initial_INVALID.txt. They are not numerical passes.

The ABI3 standalone --maps mode checked each of the ten previously verified image
exports, including resized log maps, sensitivity maps, neighbor resizes, raw
masking and pooling:28 comparisons/53056 values per export. See
validation/map_gpu_final_exports_path.txt and its PASS.txt for completion.

## Reproduce

Build IQM and IQM-jnd-single-test before running integration tests, and
IQM-map-gpu-test in validation/fft_publish_build for standalone checks. Generate
the ABI8 shader into a fresh directory; do not use compile_shaders.sh:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_power_shaders.ps1 -OutputDirectory '<fresh directory>'
glslc -fshader-stage=compute --target-env=vulkan1.2 '<fresh directory>/map.comp' -o '<fresh map.spv>'
spirv-val --target-env vulkan1.2 '<fresh map.spv>'
& './validation/fft_publish_build/Release/IQM-map-gpu-test.exe' '<map.spv>' '<resize fixture>' --resize
& './validation/fft_publish_build/Release/IQM-map-gpu-test.exe' '<map.spv>' '<CSF fixture>' --csf
& './validation/fft_publish_build/Release/IQM-map-gpu-test.exe' '<map.spv>' '<pool fixture>' --pool
& './validation/fft_publish_build/Release/IQM-map-gpu-test.exe' '<map.spv>' '<bounds fixture>' --bounds
& './validation/fft_publish_build/Release/IQM-map-gpu-test.exe' '<map.spv>' '<JND knot fixture>' --knots
```

Runtime selection is IQM_MAP_BACKEND=vulkan and IQM_MAP_SHADER=<absolute SPV>.
The wrapper exports map_backend.txt and map_shader.txt, and the suite verifies
the selected backend. For full current reference-FFT integration use:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/run_quality_validation.ps1 `
  -ReferenceShader '<verified FFT SPV>' -MathShaderDirectory '<verified math dir>' `
  -MapShader '<ABI8 map.spv>' -HDR
# Repeat without -HDR for the original six pairs.
```

The GPU-map suites have separate quality_hdr_gpu_maps_suite_latest_path.txt
and quality_gpu_maps_suite_latest_path.txt pointers. Every batch records shader
and executable hashes and writes PASS.txt only after every case passes strict
preprocessing,13 native JND intermediates, CSF/masking/D and direct official
Q/Q_JOD comparisons. Preserve failed/incomplete batches.

## Remaining CPU work and scope

Calibration LUT construction, resize plans, base-frequency hypot/selection,
base adaptation/noise and double diff-mask resize still include CPU work.
The default base route retains CPU DFT; the opt-in ABI81 shader replaces it
with native-matching GPU FFT. CPU diagnostic sums/readback also remain. Reconstruction
and visibility/detection outputs have not been ported/validated here.

The integrated scope is still64x64,30ppd,age24,D65 single-channel luminance,
the ten synthetic pairs and the opt-in reference FFT/math backend. This is
progress toward a Vulkan implementation, not a completed public metric API
or a guarantee across GPUs, dimensions, color spaces and viewing conditions.

## Completed ABI3 integration (2026-10-17)

All ten input pairs pass with GPU resize/clamp/CSF/raw masking/pooling:

- validation/quality_hdr_gpu_maps_suite_639278638762989712/PASS.txt (four HDR pairs)
- validation/quality_gpu_maps_suite_639278639847891179/PASS.txt (six original pairs)

Each checks strict preprocessing and pyramid results,13 native JND
intermediates for both images, CSF/masking/D maps and direct official Q/Q_JOD
equality. No tolerance was introduced. Vulkan run logs contain no validation
errors or VUID diagnostics. The localized case remains exactly
Q=9.1295385360717773 and Q_JOD=9.5646781921386719 in both implementations.

Current --maps evidence is validation/map_gpu_final_exports_path.txt and its
ABI3 PASS.txt:530560 compared values across ten saved exports, zero differences.
The final ABI3 build also passes resize/CSF/pool/bounds fixtures above. Its
SPIR-V has40 NoContraction decorations and no native Fma extended instruction.

The default CPU backend was checked again after the clamp correction:
validation/map_gpu_abi3_cpu_default_path.txt and
map_gpu_abi3_cpu_default_{csf,quality}.log. CSF maps and direct official scores
still match. No default backend/FFT configuration was silently changed.

## Power-port experiment (2026-10-17; rejected candidates)

Before porting masking/D powers, an isolated positive finite single-precision
probe compared 65536 pairs against Octave `single(x).^single(y)`. Seed71440,
random x in [1e-6,1e6], nine metric-related exponents, plus five explicit
cases including zero and the known square-rounding boundary. Text exports
use nine significant digits and interleaved float pairs. No image layout or
DC operation is involved. These are synthetic math probes, not image tests.

On the local Intel Arc Pro Vulkan device:

- GLSL pow:53562 unequal results of65536.
- GLSL exp(y*log(x)):56201 unequal results of65536.
- Previously verified source-backed float log/exp, with a separately rounded
  float product:52746 unequal results of65536. Both output components repeat
  this candidate, so the runner counts105492 component differences.

Evidence:validation/power_probe_639278670194189009/{native,source}.log and
results.json. Both candidate runs exit1 (numerical rejection); the experiment
runner exits0 only to indicate successful execution. No PASS marker is written.
The reused runner's `log`/`exp` labels mean output components0/1 here;
its `exported` counts are legacy index subsets and do not represent real
image exports in this probe. No Vulkan validation errors were logged.

Reproduce with tests/run_float_power_probe.ps1 -MathShaderDirectory pointing
to the verified generated math directory. Each run preserves fresh artifacts,
shader hashes and candidate exit codes. The source-backed exp candidate stays
inside its previously tested [-80,80] argument domain for positive inputs;
zero explicitly returns zero.

This demonstrates that composing individually matching rounded log/exp
functions does not reproduce native powf. At that checkpoint production
shaders/backend selection remained unchanged. The continuation below supersedes
the power-port blocker; the rejected candidates remain preserved.

## Native-style power and further GPU stages (2026-10-17)

An independent positive-finite power implementation now retains compensated
float-pair intermediates across log, multiplication by the exponent and exp.
It uses257 log/reciprocal nodes and64 exponent nodes, with explicit operation
order and final float rounding. Tables are generated from mathematical
definitions, not extracted from DLLs or populated from test answers. The
near1 path uses an atanh series. Source is math_reference/power_reference.glsl;
generate_power_shaders.ps1 composes it with the already source-backed AMD
log/exp implementation. Neither generation nor execution loads a Windows DLL.
Local UCRT arithmetic inspection was used only to identify the native graph;
binary-table diagnostic experiments remain under validation/.

The native graph's low-result guard is applied before float rounding. A
65536-pair boundary probe initially found85 cases where an IEEE-style rounding
to the smallest subnormal disagreed with native zero. Reproducing the native
log-result cutoff corrected all85. This is native-reference parity, not a
claim of correctly rounded mathematical pow for all inputs.

Validated powers before the boundary correction:10 datasets /655360 pairs,
including572160 image-derived arguments (with repeated values), all exact.
The datasets cover self/neighbor masking, reciprocal CSF, raw difference,
psychometric D and quality .8 powers; distinct seeds fill remaining entries.
The additional65536 boundary pairs include32768 dense near1 inputs and
subnormal bases/results. Preserve both failed and corrected boundary logs.

GPU MapGPU additions:

- D65 L+M adaptation input, two-image mean adaptation and pupil reduction.
  Pupil logs accumulate sequentially in column-major order; geomean and
  both age/reference diameters stay in single precision.
- Neighbor clamp/sum and N_mask powers/scales, with separate products/sum.
- Excitation, neural power, mask square, square root, raw D division and
  psychometric power. Mask square deliberately uses multiplication.
  Integer sqrt rounding, exact finite division and integer significand
  multiplication avoid relaxed GPU rounding and flushed products.
- Quality masking, .8 power, divide-before-sum, column-major single reduction,
  1.25 power, cross-band error, Q and Q_JOD. Quality still pools raw D.

Base-band neural noise is still prepared on the CPU using double pow, then
rounded once to float; it must not be silently replaced by float pow.
Difference normalization/subtraction, original comparison-mask construction,
its double bicubic resize and calibration LUT construction remain CPU work.
At the ABI6 checkpoint per-pixel JND and log10 of mean adaptation also remained
CPU work; ABI7 below supersedes that limitation. No detection or reconstruction
output was added. Existing default CPU selection is retained.

ABI6 includes pupil/adaptation and quality modes; ABI3/4/5 binaries are
rejected by the current host. Compile generated map.comp, not the shader
template directly:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_power_shaders.ps1 -OutputDirectory '<new directory>'
glslc -fshader-stage=compute --target-env=vulkan1.2 '<new directory>/map.comp' -o '<new directory>/map.spv'
spirv-val --target-env vulkan1.2 '<new directory>/map.spv'
```

The generator verifies the existing AMD source hashes when composing log/exp.
Generated table bits and shader/binary hashes belong to each validation batch.
This remains a local native-library/device parity result for the stated
synthetic test scope, not a cross-platform libm guarantee.

## GPU JND/log10 continuation (ABI7)

The fixed AMD source manifest now additionally pins
log10_128_lead_tail_table.asm (original MIT notice retained). log10 conversion
uses the table's split log10(2), including its native tail rather than a newly
rounded mathematical tail. Compensated float-pair FMA avoids a nine-case GPU
native-FMA discrepancy near1; using a newly rounded tail independently caused
39 other failures. The corrected source-backed log10 passes65536 inputs,
including32768 densely spaced floats near1. Failed candidates are preserved.

MapGPU now implements all13 photoreceptor stages: three D65 products, three
clamp/pupil products, three log10 queries, three JND LUT interpolations and
the final L+M+rod sum. The host builds double LUTs/slopes and rounds them to
float, as Octave interp1 does. Binary upper_bound compares rounded float nodes,
which matters at LUT knots. Interpolation product/add are separate. It also
computes the clamped log10 of mean adaptation used by CSF.

An independent knot fixture tests18432 inputs around2048 nodes for all three
spectral channels. GPU comparison with both the Octave LUT and the production
C++ LUT passes258048 queried/channel/response values with zero differences.
The standalone --maps check now compares62 outputs /214096 scalar values per
image pair, including all13 GPU JND intermediates against native Octave traces
for each image. It additionally checks pupil trace, adaptation input/mean/log,
all prior maps and GPU scores. The integration runner now requires this extra
GPU check when -MapShader is supplied; build IQM-map-gpu-test first.

ABI7 rejects all previous map shader versions. Generate/compile using the same
commands above. No new shaderFloat64 feature requirement was introduced.
JND calibration tables, difference/DC/base-band preparation and double
comparison-mask resize remain CPU work. General dimensions, viewing conditions,
color input, reconstruction and a public metric API remain separate tasks.

## Completed ABI7 integration (2026-10-17)

Final generated sources and SPIR-V:
validation/map_gpu_abi7_639278683626481099/.
PASS.txt and COMPLETED.json capture the accepted artifact and scope;
map_gpu_final_shader_path.txt and map_gpu_completed_path.txt now point here.

All ten freshly executed pairs pass strict preprocessing, native JND,
CSF/masking/raw D/final D and direct official Q/Q_JOD comparisons:

- validation/quality_gpu_maps_suite_639278684479473340/PASS.txt (six pairs).
- validation/quality_hdr_gpu_maps_suite_639278686318506540/PASS.txt (four HDR pairs).

Each fresh export also passes62 independent GPU comparisons /214096 values,
including all13 JND stages for both images:2140960 values across the ten pairs,
zero differences. The baseline batch began before automatic GPU-intermediate
checking was added to the runner, so its six checks were performed afterward
and recorded in GPU_INTERMEDIATES_PASS.txt plus per-case logs/binary hashes.
The HDR batch includes those checks directly in the updated runner.
This is numerical float equality; no tolerance was substituted. Localized
Q=9.1295385360717773 and Q_JOD=9.5646781921386719 remain unchanged.

Final shader independently passes45 resize cases/32643 values, CSF LUT tests
(45060 GPU plus22530 CPU checks),48 raw/pool comparisons/65280 values,20
bounds comparisons/43564 values and258048 knot comparisons. Power's final
random+boundary runner passes131072 tested pairs; its two output components
repeat the same power, not two different functions. Source-backed log10 passes
65536 inputs with duplicate output components. Earlier broader power evidence
and rejected candidates retain their own stage/source scope above.

The accepted SPIR-V has176 NoContraction decorations and no Float64 capability.
ABI6 is explicitly rejected with exit2. No Vulkan validation errors/VUIDs occur
in final run/intermediate logs. Fresh localized CPU-default CSF/masking/D and
official scores still match; see CPU_DEFAULT_PASS.txt in the final directory.
Regenerating all map/power/log10/helper sources gives identical bytes, and the
four existing FFT math sources remain identical to the previously verified
source_math_exact_division files. Source and binary hashes are saved.

Remaining implementation work is now the CPU preparation listed above,
generalizing dimensions and conditions, real HDR/color validation, other
required outputs and exposing a usable metric API. This milestone does not
claim a completed general-purpose HDR-VDP-3 implementation or a percentage.

## ABI8 preparation and native8x8 base FFT continuation

ABI8 adds GPU normalized band differences, original comparison-mask construction
and residual DC removal. The difference operation divides each image's band
separately before subtracting; subtracting first would change rounding.
Comparison masks use native single subtraction, absolute value and correctly
rounded division, followed by the strict >0.001 single threshold. Binary0/1
results are widened on the host for the existing double bicubic resize; this
does not substitute a single-precision resize. Reference luminance must remain
positive and finite in this GPU operation; general black/exceptional-input
behavior is not claimed here.

DC removal is explicitly limited to64 samples, finite nonzero exponents in
[-90,100] and an exponent span no greater than16 (zeros are allowed). At most
24+16+6=46 significant bits are needed in intermediate sums in this domain;
the native double sum is exact, and compensated float pairs retain the sum
before the once-rounded float mean. Divide by64 commutes with float rounding
in this normal range. Unsupported inputs throw rather than silently switching
precision. These guards are a current harness limitation, not general image
support. The standalone native fixture includes signed fields, zero/constant
fields, three shapes, six normalizations including3, and float neighbors around
the relative-difference threshold:100 comparisons/59296 values, zero differences.
The full preparation-enabled chain passes all ten official quality pairs in
quality_gpu_maps_suite_639278692891709180 and
quality_hdr_gpu_maps_suite_639278694995648294 under validation/.

Base-frequency work has a separately versioned FFT shader, ABI81. The observed
Octave native8x8 real2D plan uses8 scalar r2cf_8 transforms followed by5 n1_8
transforms and conjugate reconstruction:420 additions,36 multiplications,
zero FMAs; local single FFTW reports22 threads. It is not the128x128 real/SIMD
plan used by preprocessing. generate_base_fft_shader.ps1 reads the existing
SHA-pinned FFTW3.3.10 archive and preserves original C notices and COPYING
beside the generated GPL-2.0-or-later reference shader. This experimental
runtime path is opt-in, like the reference preprocessing backend.

The independent64-input fixture includes zero/constant/impulse, directional
cosines, unequal-frequency equal-amplitude ties, a perturbed tie, checkerboard
and random signed fields. All8192 real/imaginary spectral values and64
first-max index/frequency selections match native Octave exactly. Packed
runner inputs additionally have zero padding; padding is not extra FFT data.
The selected spectrum is on GPU; native single hypot, column-major first-max
selection and physical double frequency calculation remain CPU work. Setting
IQM_BASE_FFT_SHADER selects this route, replacing the CPU double DFT. Without
that setting the existing CPU DFT remains. Calibration/base adaptation/noise,
comparison-mask double resize and public-API/generalization work also remain.

The updated suite accepts -BaseFFTShader alongside -MapShader and records
both shader hashes and selected backends. It compares the native base spectrum
and first-max index in each fresh export. Its separate latest-path pointers
are quality_gpu_base_fft_suite_latest_path.txt and
quality_hdr_gpu_base_fft_suite_latest_path.txt under validation/.
An enabled base FFT adds128 scalar spectrum comparisons per pair to the
ABI8 --maps checks:71 comparisons /227984 values, plus exact index/frequency
assertions. Map ABI8 and base FFT ABI81 reject swapped or obsolete binaries.

Generate the two shaders in fresh directories and compile/validate separately:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_power_shaders.ps1 -OutputDirectory '<new map directory>'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_base_fft_shader.ps1 -OutputDirectory '<new base directory>'
glslc -fshader-stage=compute --target-env=vulkan1.2 '<new map directory>/map.comp' -o '<map.spv>'
glslc -fshader-stage=compute --target-env=vulkan1.2 '<new base directory>/base_fft.comp' -o '<base_fft.spv>'
spirv-val --target-env vulkan1.2 '<map.spv>'
spirv-val --target-env vulkan1.2 '<base_fft.spv>'
# Run the existing quality suite with both -MapShader and -BaseFFTShader.
```

### Completed ABI8 + ABI81 integration

Accepted artifacts are in validation/map_gpu_abi8_639278691970167183/;
COMPLETED.json and PASS.txt record the checkpoint. Current local pointers are
map_gpu_final_shader_path.txt, map_gpu_base_fft_shader_path.txt and
map_gpu_completed_path.txt under validation/.

All ten freshly executed pairs pass with GPU DC/differences/mask and native
base FFT enabled, including full native preprocessing/JND, all maps and direct
official Q/Q_JOD equality:

- validation/quality_gpu_base_fft_suite_639278697831326240/PASS.txt (six pairs).
- validation/quality_hdr_gpu_base_fft_suite_639278700680928595/PASS.txt (four HDR pairs).

Each fresh export passes71 comparisons /227984 values, plus exact base index
and double-frequency assertions:2279840 compared scalar values and zero
differences across ten pairs. The localized scores remain exactly
Q=9.1295385360717773 and Q_JOD=9.5646781921386719. Source/binary hashes and
per-case native base spectra/selections are retained in the fresh directories.

The final host also passes preparation/resize/CSF/pool/bounds/knot fixtures.
Old ABI7 maps, a swapped map shader supplied as a base FFT, and DC inputs
outside the guarded exponent domain are deliberately rejected (exit2), not
counted as numerical passes. The map shader has182 NoContraction decorations;
base FFT has55 and no native FMA instruction. Neither requires shaderFloat64.
No Vulkan validation errors or VUIDs occur in the final pipeline logs.
CPU_DEFAULT_PASS.txt confirms localized CPU maps/default CPU DFT still match
official scores; SOURCE_REPRODUCIBILITY_PASS.txt confirms both generators
reproduce identical sources. Earlier ABI8 tests with CPU base DFT retain their
separate checkpoint and pre_base_source_hashes.json records.

Remaining CPU work is calibration/plans, base hypot/index/frequency and
geometric adaptation/noise/scalar double power, and double comparison-mask
resize. Parameter/dimension/color generalization, real HDR validation, other
required outputs and a usable metric API are still outstanding. Preparation
ports do not expand the existing full-metric validation beyond the stated
64x64/30ppd/age24/D65 scope.

## Short pre-publication checks (2026-10-17)

Using the unchanged accepted binaries (SHA256 checked), three fresh sequential
runs enabled Vulkan maps ABI8 and the native-matching base FFT ABI81: two
asymmetric/localized runs and one asymmetric/identical run. Each localized run
matched the previous accepted export in all53 selected numeric text files by
SHA256 (106 file comparisons). This verifies repeatability on this device and
configuration; it is not a concurrent-process test or a cross-device guarantee.

Fresh direct official Octave comparisons passed exactly for localized
Q=9.1295385360717773, Q_JOD=9.5646781921386719 and identical Q=10, Q_JOD=10.
The identity export has all five band errors and total error exactly zero.
The three pipeline logs contain no Vulkan validation errors or VUIDs.

Three deliberately incompatible configurations were rejected with exit2:
ABI7 map as ABI8 map, ABI81 base shader as map, and ABI8 map as ABI81 base shader.
These are host ABI-guard checks, not numerical passes.

Local evidence: validation/abi8_quick_checks_639278706524535142/ contains
PASS.txt, RESULT.json, exports.txt, binary_hashes.json, repeatability_hashes.json
and full run/official/rejection logs. Full artifacts and implementation remain
local/uncommitted; this publication contains documentation only.
