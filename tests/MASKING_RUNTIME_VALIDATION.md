# Masking and quality runtime validation

`run_pathway_masking` takes five in-memory band pairs,pooled self masks,double
comparison masks,GPU log adaptation,integer residual selection,80x96/96x80
geometry and explicit map/product/base shader paths. Native image-dependent
intermediates are never read by runtime. The input bands are already normalized
and base-centered. The fixed profile is30ppd,age24,D65,sp0Filters.

The model factory computes p,pf,q,self andneighbor scales from the same pow10
constants as production evaluation. The checker verifies their binary64 values
against native scalars for every band/content/orientation:150 comparisons of
repeated fixed model values,not150 independent parameters. Neural/base noise
is not taken from native data. It uses the actual GPU log resize/clamp,accepted
base120 mean/power shader,GPU CSF parameters and residual selection,followed by
the existing CPU double NCSF/frequency helpers. Actual base range/exponent
checks are preserved before mean computation.

Existing production MapGPU performs neighbor resizing/masking,CSF sampling,
D,psychometric,band error and final scores. The explicit product shader exports
masked D diagnostics. CPU provides model/LUT data and double NCSF only; host
vector transfers remain synchronous. The caller must exclusively own the
instance command buffer. This is not a GPU-resident command graph.

`verify_masking_runtime.cpp` preserves the full GPU-input profile of the old
checker; its native reads are goldens only. It verifies126stage/input checks
and393384values per content,default model double bits,base noise double bits
and final score bits. Three contents/two orientations pass378checks/1180152
values with zero observed bit errors; `validate_masking_runtime.ps1` additionally
gates every recorded bit counter. These counts include repeated input checks,
not new unique pixels or viewing parameters. Original checker/artifacts remain.

```powershell
cmake -S tests/hdrvdp3_dimensions -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target masking-runtime-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_masking_runtime.ps1 -Executable <fresh-build>/Release/masking-runtime-test.exe -OutputDirectory <fresh-results>
```

Checkpoint: `validation/masking_runtime_1792914241203`,Intel Arc Pro Graphics.
The standalone checker and bounded evaluation library build successfully with
strict floating point. Matrix inputs come from frozen GPU band,pool,comparison,
log andSPYR exports,with exact paths in runtime_results.json. Public quality
geometry guards remain unchanged. Full in-memory API composition and broader
input/viewing/device coverage remain open.

A fresh full HDR original-image replay also passes using every extracted runtime
component. Both orientations' final Q/Q_JOD words equal independent native
results; all126 quality-stage rows have zero bit errors. No Vulkan validation
errors. Artifacts: full_hdr_replay/PASS.txt and validated/results.json. This is
still file/host composition between components; it is not the single in-memory
API or general public geometry support. Replay counts overlap matrix coverage.
