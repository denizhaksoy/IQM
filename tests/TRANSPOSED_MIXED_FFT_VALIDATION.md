# Transposed192x160 FFT components

Native inputs are the prior h96_w80 asymmetric/impulse/HDR-ramp component
fixtures, ref/test factor1.05, MTF/LA at30ppd. They are not full-quality pairs.
Production shaders and public guards remain unchanged; these are diagnostic
GPU transforms from independently exported native stage0 or filtered stage2.
All FFT comparisons require numerical equality; signed-zero bits are outside
this check. Every run uses fresh directories and retains old references.

## Real192 first axis

Checkpoint: `validation/real192_axis_639283673902853437`.
`generate_fft192_forward_axis_shader.ps1` ports the selected UNALIGNED native
graph: outer real8 over inner real24, with hc2cf_8/r2cf_8/r2cfII_8 endpoints.
Inner24 uses twelve r2cf_2 sum/difference leaves followed by r2cf_12 and
r2cfII_12. The hf_12 body has no interior frequency here: the pinned
hc2hc-direct.c solver gives mb=me=1 for inner size2, so only its endpoint
children execute. Original GPL sources, solver contracts and LF SHA256/member
provenance are in `fftw_reference/real192/SOURCES.json`.

Input:192 real samples per160 columns, column-major interleaved Float32,
imaginary zero, text header192 320. Output:160 columns each97 compact complex
frequencies, header160 194. Forward is unnormalized; no padding/filter/DC is
executed here. The prepared MTF input uses symmetric POST padding, LA replicate
POST padding. Scalar operation order and helper rounding remain precise.

Native `--forward192-axis` exports fresh3-thread ESTIMATE|UNALIGNED oracles.
`validate_real192_axis.ps1` checks every whole isolated plan against the
specifically selected rank2 UNALIGNED/out-of-place forward subtree, not against
an arbitrary aligned plan. All12 plans agree. On Intel Arc Pro Graphics,
`verified_complete/PASS` records372480 Float32 values exactly equal.
The initial `verified/` run had a harness result-tag error (looked for
inverse_axis instead of forward_axis), not a numerical mismatch; preserved.

## Inverse192 first axis

Checkpoint: `validation/inverse192_axis_639283803326908322`.
`generate_fft192_inverse_axis_shader.ps1` uses scalar n1_12 leaves followed by
t2_16 with powers1,3,9,15. Its descriptor assertion caught an initial assumed
power7 before GLSL compilation; `shader_correct_descriptor/` is the accepted
shader. Backward scalar pointer roles are swapped as in the native solver.
Input is column-major192x160 stage2; output is the transposed storage equivalent
to row-major192x160, unnormalized. No second axis/division/DC occurs here.

Native `--inverse192-axis` and `validate_inverse192_axis.ps1` compare all12
selected rank2 first-axis subtrees and GPU outputs. On Intel Arc Pro Graphics,
`verified/PASS` records737280 Float32 values exactly equal. These remain fresh
candidate graphs, not recovered original cached Octave objects.

Both isolated shaders use explicit DenormPreserve32/RoundingModeRTE32 via
`compile_forward192_axis.ps1` / `compile_inverse192_axis.ps1`; Vulkan1.2
spirv-val passes. Device capability checks alone are insufficient.

## Complete forward192x160

Checkpoint: `validation/forward192x160_639283801741962283`.
`generate_fft192x160_forward_shader.ps1` composes real192 with complex160:
n2fv_8_avx2_128 leaves (FMA source branch) and t3fv_20_avx2_128 (ordinary
source branch plus native compiler contraction). Powers1,3,9,19 come from
the pinned descriptor. Native batch groups are33/33/31; each GPU invocation
executes one frequency lane's graph. Complex160 sources are pinned under
`fftw_reference/complex160/`, with original GPL notices.

Crucial scope: this graph needs different vector helper contraction from the
previous complex192 graph. Precise scalar helpers are retained for real192;
vector helper results permit contraction to match native machine code.
The native DLL was inspected read-only with local GNU objdump; registration
and kernel disassembly, PE scan metadata and DLL hash are preserved in the
checkpoint. Additional FMA instructions occur in the compiled t3fv_20 path.
This establishes the tested device/compiler contract; it is not a portability
claim for arbitrary drivers or GPUs. Pin/revalidate this contract on changes.

`--complex160-leaves` diagnostic with aligned out-of-place input stride20,
distance1, output stride1/distance8 selects n2fv_8_avx2_128. Its31040 values
match the GPU leaf probe exactly (`native_leaves_aligned/gpu_check.txt`).
An earlier in-place UNALIGNED probe selected scalar n1_8 and had7565 differences;
it is excluded because it is a different graph. The GPU leaf shader exports
an intermediate, so its raw full-forward rejection is intentional; only the
separate leaf comparison is accepted.

Failed complete-forward probes are retained: precise helper variants had
31598,23205,23996 and10322 mismatches across graph choices; changing nested
product fusion gave18421. No tolerance was relaxed. Allowing the native
vector-helper contraction gives zero; default generator SPIR-V reproduces
the successful probe byte-for-byte (`default_reproduction.txt`).

`compile_forward192x160.ps1` enables explicit Float32 controls.
`validate_forward192x160.ps1` checks full output against independent official
fft2 and separately checks real192 compact values. Accepted `verified/PASS`:
all12 streams,737280 full-forward Float32 values and372480 first-axis values,
zero differences on Intel Arc Pro Graphics. Final output is GPU column-major,
checker row-major,192x160 interleaved; Hermitian expansion conjugates both axes.
No filter, inverse or DC processing is included in this forward unit.

## Complete inverse192x160

Checkpoint: `validation/inverse192x160_639283807856355507`.
The shader combines inverse192 t2_16/n1_12 with inverse160 scalar n1_8 leaves
and t3bv_20_avx2_128 using the native contraction contract. Fixed30720 division
is embedded unchanged from the separately validated integer normalizer.
`compile_inverse192x160.ps1` supplies explicit Float32 modes; the shader
declares only Shader/DenormPreserve/RoundingModeRTE capabilities.

On Intel Arc Pro Graphics, all12 streams pass737280 raw comparisons against
prior matched native FFTW unscaled exports and737280 normalized comparisons
against independent official ifft2. First-axis values remain exact in both
modes. Acceptance: `verified/PASS`,24 per-mode logs and hashes. The checker
does not assert signed-zero bit identity for the whole transform. These are
native-filtered stage2 inputs; GPU filtering and full quality are not claimed.

Reproduce with `compile_inverse192x160.ps1` and
`hdrvdp3_dimensions/validate_inverse192x160.ps1`; pass the prior native mixed
checkpoint, inverse192-axis `verified/` oracle, GPU executable, shader and a
fresh output directory. All publication remains separate from this local work.
