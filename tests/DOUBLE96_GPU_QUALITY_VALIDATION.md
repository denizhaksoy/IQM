# Double96 GPU quality arithmetic and pooling (v9)

Validated 2026-10-30. Artifacts: `validation/double96_quality_639289541135475932`.
Run `node tests/hdrvdp3_evaluation/audit_double96_quality.cjs <artifact-root>`.

The optional `-GPUQuality` runtime builder switch enables version 9 (21 payloads)
and implies all earlier GPU stages. Default packaging remains version 1.
GPU quality operations include weighted masking arithmetic, signed excitation,
Double sqrt, corrected division, comparison multiplication and column-order
pooling with division of each term before accumulation. Positive infinite noise
and signed zero are supported. The optional denominator and raw pooling traces
check results before the final CPU power can hide rounding differences.

CPU power queries, psychometric values, final error power and final scores remain
on CPU. This is a hybrid implementation, not fully GPU quality prediction.

## Measured results

- `stage_rawpool`: 1,802,444 Double comparisons, zero bit differences:
  1,158,714 native/derived values and 643,730 controls.
- Of these, 1,501,990 values returned from GPU; 300,454 CPU values.
  Power preparation inputs are not separately counted.
- Native maps use preserved HDR-VDP-3 v3.0.7 exports, converted from column
  layout to row layout. Denominator and raw pooling checks use derived C++
  oracles; they are not independent native exported fields.
- Nine CLI configurations: 72 quality Doubles and 497,664 decoded Float32
  input words exact. Two distorted pairs and identity across three displays;
  these reuse the preceding corpus and do not represent nine new photographs.
- Ten stage, thirteen CLI and thirteen runtime rejection checks passed.
- Versions 1 through 8 retained 24 byte-identical numeric/input exports.
  Inspected Vulkan validation logs contained no errors.

The earlier `stage` run overlaps this coverage and must not be added to it.
Controls cover signed differences/zeros, negative comparison ringing, infinite
noise, sparse inputs and tiny differences. Direct native GPU Double sqrt passed;
no sqrt correction or Float32 conversion is used.

## Scope and remaining work

Fixed 96x96 sRGB-display input, 30 ppd, age 24, default sp0, band widths
96/96/48/24/12, tested NVIDIA RTX A500 driver 595.95. Double RTE and signed
zero/infinity behavior are required; generic denormal/device parity is not
established. Existing input precision, padding and DC processing are retained.

Remaining CPU work includes display preprocessing, nonlinear log/exp/JND and
CSF model preparation, all quality powers and final scalar prediction. Arbitrary
dimensions, other viewing conditions and other devices are outside this exact
validation claim. The next numerical task is an isolated Double log/exp/pow
diagnostic against the existing CPU/native arithmetic before any production
replacement. Preserve the exact hybrid path if candidate GPU math differs.
