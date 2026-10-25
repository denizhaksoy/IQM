# Pathway SPYR runtime validation

`run_pathway_spyr` in `src/hdrvdp3/pathway_spyr.cpp` accepts two in-memory
Float32 P maps,80x96/96x80 dimensions,explicit fixed sp0 filter words and
FMA/separate/mean/difference/residual shader paths. It reads only shaders.
Native expected files and manifests are exclusive to the checker.
Buffer lengths and dispatch geometry derive from dimensions,not expected data.

Returned stages0..7 contain paired lowpass/high/orientation outputs,stage8
mean/DC words,stage9 difference pack,andstage10 residual FFT/magnitude/selection.
The two FMA/separate dispatches retain the accepted per-stage shader contract.
Mean/DC/normalization and column-major selection ordering are unchanged.

Runtime validates actual GPU base data before dispatching mean/DC: finite
values,nonzero exponents[-90,100],within-pair-image exponent spread<=16.
This requires a fence/download/check between SPYR and residual submissions;
CPU performs validation only. Actual residual complex values are range checked
before returning. Rejected data yields an exception,not a tolerance fallback.
The runtime is synchronous and requires exclusive instance command-buffer use.
It does not claim one resident submission or arbitrary input-domain parity.

`tests/verify_spyr_runtime.cpp` preserves the historical --all-chain CLI and
exports eleven stages for each orientation. The standalone matrix covers
asymmetric,impulse,HDR ramp: six pairs,66 stage rows,340578 values. All observed
words match bitwise,including signedzero. The checker retains historical
component acceptance,while `tests/validate_spyr_runtime.ps1` additionally gates
zero bit errors on every row. Original checker/source/binaries are preserved.

```powershell
cmake -S tests/hdrvdp3_dimensions -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target spyr-runtime-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_spyr_runtime.ps1 -Executable <fresh-build>/Release/spyr-runtime-test.exe -OutputDirectory <fresh-results>
```

Checkpoint: `validation/spyr_runtime_1792913354201`,Intel Arc Pro Graphics.
The standalone executable and bounded evaluation library compile successfully
with strict floating point. Public evaluate_quality geometry guards remain
unchanged. Filters are caller-supplied fixed model data; this does not validate
arbitrary filter coefficients,PPD/age/RGB or devices. Pool/map runtime integration
and cross-stage GPU residency remain separate work.

The fresh full HDR CPU-LUT replay also passes all stages and independent native
Q/Q_JOD bit identity for both orientations (`full_hdr_replay/PASS.txt`). Its22
SPYR stage rows have zero bit errors. It uses the extracted mixed filter and
SPYR runtimes plus production JND/frequency helpers. Downstream pool/masking
stages still exchange files/host vectors. Source/asset hashes and completed
pointer are saved. Original native installations and reference files unchanged.
