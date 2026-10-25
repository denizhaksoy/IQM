# Mixed filter runtime validation

The seven-dispatch mixed-axis filter chain now lives in
`src/hdrvdp3/mixed_filter.cpp`, exposed by `IQM/hdrvdp3/mixed_filter.h`.
`run_mixed_filter` accepts a Float32 input vector, dimensions, filter mode,
explicit forward/inverse/IO shader paths, and an abstract `IQM::VulkanInstance`.
It returns the output vector; optional diagnostics expose four complex stages
and the generated model filter. Native fixture loading stays in the checker.

The current contract is positive finite luminance,80x96 or96x80,30ppd. MTF
uses endpoint-inclusive symmetric POST padding; LA uses replicate POST padding
and the accepted log/exp math. Complex buffers use column-major FFT storage,
with2H x2W padding. The second inverse axis uses the exact30720 normalizer.
All seven dispatches execute in one command buffer with no host intermediate
computation. CPU work generates filter coefficients and uploads original data;
output/optional trace downloads follow fence completion. Calls are synchronous
and require exclusive use of the instance command buffer.

`tests/verify_mixed_filter_chain.cpp` now invokes this component. Native
stage/model files supply expected values only. All five stages require both
numerical and bit equality. Three80x96 MTF reference cases additionally call
runtime again without tracing, requiring identical output bits and empty
optional diagnostics on the same Vulkan instance.

```powershell
cmake -S tests/hdrvdp3_dimensions -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target mixed-filter-chain-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_mixed_filter_runtime.ps1 -Executable <fresh-build>/Release/mixed-filter-chain-test.exe -OutputDirectory <fresh-results>
```

The24-case matrix covers asymmetric,impulse,HDR ramp; both orientations;
MTF/LA; reference/test. This is6,082,560 stage values and737,280 model words.
`-ResultsOnly` audits an existing completed output directory without rerunning
GPU work. Input/native/model scopes are inherited from each content's frozen
`assets_cpu_filters.json`, and executable/shader/manifest hashes are recorded.

Checkpoint root: `validation/mixed_filter_runtime_1792912954173`.
Source snapshots and old checker source are preserved there. The standalone
checker and bounded evaluation library compile with strict floating point.
The selected device is Intel Arc Pro Graphics. This extraction does not enable
80x96/96x80 in `evaluate_quality`: mixed SPYR/residual/pool/map runtime integration
and broader viewing parameter/device coverage remain open.

Final result: all24 component cases PASS with zero numeric/bit errors; no
Vulkan validation errors. Full HDR CPU-LUT replay in `full_hdr_replay/PASS.txt`
also passes original image ->filters/JND/SPYR/pool/masking ->Q/Q_JOD with both
orientations' final score words identical to native. This replay uses the
extracted filter runtime,latest production JND,production frequency helper.
It remains file/host composed between downstream components. Source snapshots,
source/binary/shader/manifest hashes and completed pointer are preserved.
