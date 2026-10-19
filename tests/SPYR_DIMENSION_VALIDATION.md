# SPYR dimension and32x32 mask validation

Checkpoint updated2026-10-19 (Europe/Prague). These measurements describe
local GPU/integration work;they do not establish a generalized public metric.
Scalar CSF/resize checkpoints remain committed in51525e0 and7ce32a0.

## Raw single pyramid

Three inputs (asymmetric modulo,edge/off-center impulses,deterministic random)
were tested at nine shapes against official HDR-VDP-3 v3.0.7 using installed
Octave11.3.0 `corrDn.m`, `sp0Filters` and `buildSpyr`:

| Input HxW | Height at30ppd | Bands | Residual HxW |
|---|---:|---:|---|
|32x32|2|4|8x8|
|48x48|2|4|12x12|
|64x64|3|5|8x8|
|96x96|3|5|12x12|
|128x128|3|5|16x16|
|49x65|2|4|13x17|
|64x96|3|5|8x12|
|65x49|2|4|17x13|
|96x64|3|5|12x8|

All27 cases pass:360816 raw float values compare numerically equal,with zero
maximum error. All3918 independently centered residual values also compare
equal. This float check is numerical equality;it does not inspect signed-zero
bit patterns. All raw bands must pass before centered residuals are accepted.

Both paths consume identical exported binary32 inputs. C++ serializes
row-major text with round-trip float precision;Octave reshapes/transposes it.
Padding is `reflect1`,subsampling starts at the first pixel and output shapes
use ceil division. Height follows official
`min(ceil(log2(ppd))-2,maxPyrHt(size(input),13))`. The new optional C++ height
cap is explicitly3 in the probe;generic callers still need to supply the
appropriate viewing-parameter cap. DC processing is double sum/once-rounded
single mean on the host versus the official single mean;both results are
measured here. GPU residual-DC support itself remains guarded to64 samples.

## Native arithmetic paths

The previous level2 non-FMA choice happened to work at64x64. Fresh measurements
show the installed native convolution uses the longer image axis for inner
tap traversal,with descending taps. Blocks of32 elements use FMA;the tail uses
separate product/add rounding. `diagnose_spyr_dimensions.m` and
`diagnose_spyr_rectangular_order.m` isolate these paths without changing the
reference. This is measured behavior for this native build,not a general
convolution rule or a cross-device proof.

The revised local harness uses separately compiled FMA/non-FMA pipelines.
When there is a vector tail,it reads both GPU results and selects already
rounded values on the host by the pre-subsampling coordinate. Convolution
arithmetic remains on GPU;selection/readback remains host work. Initial
attempts to mix both arithmetic paths in one shader broke even64x64 parity on
the validation Intel GPU. Failed exports and candidate SPIR-V files are kept.

Staging allocation now follows actual image dimensions instead of a fixed
64x64 capacity. The staged `hdrvdp3_corrdn_test.glsl` and its generated include
were preserved. The new non-FMA include is named
`hdrvdp3_corrdn_native_tail.inc`,generated from the common corrDn source with
`IQM_CORRDN_NON_FMA`.

For reproducible shader artifacts,use a fresh directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/compile_corrdn_reference.ps1 -OutputDirectory validation/corrdn_fresh
```

The script validates both SPIR-V variants and records hashes. It does not
install includes. The local build used the resulting `hdrvdp3_corrdn.inc` and
`hdrvdp3_corrdn_native_tail.inc` in `shaders_out/hdrvdp3/`;prior binaries were
copied into evidence directories before replacement. Do not run the blanket
shader script;it deletes existing generated artifacts.

Raw test commands after building the local IQM harness:

```powershell
$env:IQM_SPYR_DIMENSION_PROBE='1'
$env:IQM_SPYR_PROBE_PATTERN='asymmetric' # also impulse,random
./build-vs2026/bin/Release/IQM.exe --method HDRVDP3 --enable-validation
# Use the newly printed SPYR dimension directory:
octave-cli --quiet --no-gui tests/validate_spyr_dimensions.m <fresh-directory>
```

Unset `IQM_SPYR_DIMENSION_PROBE` before running full metric tests.

Evidence per pattern:

- `validation/spyr_dimensions_17923555764323481`
- `validation/spyr_dimensions_17923555832003632`
- `validation/spyr_dimensions_17923555942649684`

## Comparison-mask resize

The local MapGPU input guard now admits32x32 binary masks,with square outputs
32/16/8,and preserves64x64 outputs64/32/16/8. The horizontal plan offset uses
the actual input count rather than4096. Shader ABI12 is unchanged. Existing
Q16/unit-normalization/L1 guards and signed Q32 bounds remain enforced;
arbitrary dimensions and fractional masks are still rejected.

An isolated checker build,without replacing the checker used by the concurrent
full pipeline regressions,passes192 cases/86016 binary64 outputs for32x32.
A fresh64x64 regression passes256 cases/348160 binary64 outputs. All434176
values match native `imresize(double(mask),...)` bitwise. Fixtures include
zero/one masks,corners,an off-center impulse,checkerboards,stripes,steps and
deterministic random masks. Evidence:
`validation/mask32_639279525250877995`.

## Full metric scope and remaining work

After the SPYR change,eight fresh64x64 pairs (original6,HDR stress2) pass the
strict preprocessing,JND,CSF,masking,D,noise and official Q/Q_JOD checks. These
are a new regression set;the earlier eighteen-pair checkpoint is preserved
with its own binaries and scope. The new mask32 implementation is tested in
a separate checker build;the application used by these eight runs predates
that small mask32 host change. No32x32 full-metric claim follows from this.

Next:parameterize the MTF/local-adaptation path for32x32 input and64x64 FFT,
including native FFT plan/codelets,padding,crop dimensions and math probes.
Its current reference backend still fixes64x64 input/128x128 FFT. Then make
reference validators discover four pyramid bands instead of assuming five,
and validate the entire32x32 chain. Native8x8 base FFT remains applicable at
32x32/30ppd. Other raw-SPYR shapes require additional residual FFT/DC,
comparison-mask and masking work before full-metric support.
