# Pathway pooling runtime validation

`run_pathway_pool` accepts an abstract Vulkan instance,two in-memory Float32
bands,dimensions and explicit FMA/separate shader paths. It returns raw mutual
mask and3x3 pooled vectors. Native fixtures are read only in the independent
checker. No resizing,normalization or DC subtraction is performed by this API:
callers supply the accepted normalized/DC-processed SPYR bands.

The implementation preserves `min(abs(reference),abs(test))`,zero-padded taps,
longer-axis traversal,and per-source-axis FMA selection within32-element blocks.
Two shader variants retain the original disjoint write partition. A single
synchronous submission completes before output download; exclusive instance
command-buffer use is required. Finite inputs with matching sizes and only the
eight validated80x96/96x80 pyramid band geometries are accepted.

The30-band matrix uses frozen GPU-serialized SPYR outputs for asymmetric,
impulse andHDR ramp,both orientations,five bands each. All214560 raw/pooled
words match native expectations bitwise. Each ten-band run reuses the same
Vulkan instance. No Vulkan validation errors were found. These are observed
fixture results,not a claim of arbitrary geometry/content/device parity.

Resize/CSF were already available through production MapGPU and are exercised
in downstream masking validation; this extraction adds pooling only. Public
`evaluate_quality` geometry guards remain unchanged. Cross-stage vectors/files
are still exchanged; a GPU-resident full pipeline remains future work.

```powershell
cmake -S tests/hdrvdp3_dimensions -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target pool-runtime-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_pool_runtime.ps1 -Executable <fresh-build>/Release/pool-runtime-test.exe -OutputDirectory <fresh-results>
```

Checkpoint: `validation/pool_runtime_1792913701963`,Intel Arc Pro Graphics.
Old checker source/binaries and native reference files are preserved.
`-ResultsOnly` audits existing results without repeating GPU computation.

The bounded evaluation library build passes. A fresh full HDR CPU-LUT replay
with extracted filters/SPYR/pooling also passes independent native Q/Q_JOD bit
identity for both orientations (`full_hdr_replay/PASS.txt`). All ten new pool
rows are bitwise exact; existing resize/CSF/masking checks pass126comparisons/
393384values with zero numerical errors. No Vulkan validation errors occurred.
These overlapping replay checks do not add new unique content coverage.
