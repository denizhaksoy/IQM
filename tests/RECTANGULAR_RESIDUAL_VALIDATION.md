# 128-sample residual integration

Evidence: `validation/rectangular_residual_639282893861381197/`.
This follows the [rectangular MTF/LA integration](RECTANGULAR_FILTER_VALIDATION.md).
The quality API still rejects rectangles.

The isolated128-sample reduction shader is enabled explicitly with
compile_base16.ps1 -Reductions -Include128. Default generation is unchanged:
the64/256 reduction SPIR-V reproduces the accepted SHA256
822C80471F082A0F07675278610622D0FE8D75A8C55D500EB367554625B61DE9.
The regenerated ABI14 map likewise retains its accepted84620708... hash.
Old executable copies and all interrupted artifacts are preserved.

The completed isolated run is identified by resumed_run_path.txt in the
checkpoint. Native fixtures contain512 cases spanning zero/constant, signed
cancellation, adjacent means, extreme exponent controls and random inputs.
All67,072 centered/mean/log-mean/power10 values match native Octave exactly.
Two further GPU executions each process512 cases in8x16 and16x8 layouts.
Each compares131,072 centered interleaved values (including imaginary zeros)
and197,120 FFT/magnitude/first-index scalars:328,192 comparisons per execution,
zero numeric mismatches. The raw FFT is retained;only selection magnitudeDC
is zero. Column-major first-max traversal uses strict greater-than.

These two isolated chains keep reduction->FFT->hypot/selection on GPU in
one command buffer, with no intermediate host readback or native-output
substitution. Arithmetic is Float32, denormal-preserving/RTE;no Float64/Int64
shader capabilities. FFT inputs/exports are row-major interleaved, with no
padding or FFT normalization. Native mean accumulation is exact in the stated
input domain:nonzero exponents[-90,100],span<=16;log inputs[-5,5],span<=16.
This is a local native/driver profile,not universal floating-point parity.

MapGPU now has a separate RectangularResiduals profile (ABI15),with128-sample
DC/base adaptation and explicitly shaped base_spectrum_rectangular /
base_selection_rectangular methods. Residual FFT ABI85 accepts8x16/16x8
row-major inputs;ABI13/14 and81/82 are retained. No automatic shape inference
from128 samples is used. Public masking/resize guards are not widened.

Actual MapGPU API validation in map_api/ covers:

| Check | Cases | Float32 comparisons |
| --- | ---: | ---: |
| 128-sample DC/base adaptation | 512 | 66560 |
| 8x16 DC/FFT/magnitude/first-max | 64 | 32832 |
| 16x8 DC/FFT/magnitude/first-max | 64 | 32832 |
| ABI13/14 legacy64/256 reductions | 512 each | 165888 |
| ABI15 same64/256 reduction fixtures | 512 each | 165888 |

All132,224 new-domain and331,776 regression comparisons have zero numeric
mismatches. API residual cases use the first eight controls followed by56
distributed cases from the512-case native fixture. MapGPU methods synchronously
read GPU results back and pass those actual results into subsequent calls;
this API check is distinct from the isolated GPU-only chain. Twelve legacy
and16 new profile/ABI/dimension/domain guard checks reject correctly.

The unchanged legacy256 DC->FFT16/selection chain also passes655,872
comparisons. Current public32/64/128 API replays preserve Q/Q_JOD/error bits
and96/107/107 numeric export hashes. Numeric stage checks do not assert
signed-zero bit identity. Counts for repeated native inputs/profiles are kept
separate;they are not new independent input domains.

Reproduce native fixtures with `generate_base_reductions_reference.m fresh-output 128`.
Compile the isolated selection shader with compile_rectangular_inverse.ps1
-Selection,or add-MapABI for85. Generate ABI15 using
generate_map_rectangular_shader.ps1 in a fresh directory. Run
validate_rectangular_residual.ps1 for the isolated chains and
validate_rectangular_map_residual.ps1 for actual API/regression checks.
Both accept the built fft256-filter-test/map128-api-test executables from
tests/hdrvdp3_fft256. Source/native/binary hashes and compiler logs are retained.

Remaining work:rectangular comparison-mask resize,CSF/adaptation resizing,
masking/pooling and dominant-frequency integration,then continuous native
preprocessing/JND/SPYR/map/official Q/Q_JOD checks. No full rectangular score
or public API completion is claimed here.
