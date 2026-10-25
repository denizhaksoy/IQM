# Comparison-mask runtime validation

`run_pathway_comparison` takes original in-memory Float32 reference/test
luminance,source/target dimensions,an abstract Vulkan instance and explicit
shader path. It returns binary mask words and a resized double mask. Native
fixture reads remain in the standalone checker. Reference must be positive
finite;test finite. Sources80x96/96x80 and uniform1/2/4/8 downsampling only.

The runtime preserves the binary ->Q20 horizontal ->Q40 double conversion
sequence in three GPU dispatches. CPU computes only axis plans. Plan checks
require unit normalization,finite exactly representable Q20 tap weights,
absolute tap magnitude<=1 and sum of absolute weights<=2. Buffers derive from
source/target geometry,not expected files. Explicit shader compatibility is
required. The call is synchronous and requires exclusive command-buffer use.

`validate_comparison_runtime.ps1` validates90cases/content across asymmetric,
impulse,HDR ramp:270cases,2073600 binary words and965520 double words match
native bitwise. These include overlapping resize/mask variants,not270 unique
image pairs. Same-instance repeated calls are exercised within each90case run.
No Vulkan validation errors were found. Old checker/native artifacts preserved.

```powershell
cmake -S tests/hdrvdp3_dimensions -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target comparison-runtime-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_comparison_runtime.ps1 -Executable <fresh-build>/Release/comparison-runtime-test.exe -OutputDirectory <fresh-results>
```

Checkpoint: `validation/comparison_runtime_1792913994936`,Intel Arc Pro Graphics.
The bounded evaluation library builds successfully with strict floating point.
Source/asset hashes and snapshots are recorded. Public evaluate_quality guards
are unchanged. Remaining work includes masking/base-noise runtime composition,
full in-memory API acceptance and broader content/viewing parameter/devices.
Cross-stage GPU residency remains separate work.

Fresh full HDR replay also passes native final Q/Q_JOD bit identity in both
orientations,using all extracted upstream components plus comparison runtime.
Artifacts: full_hdr_replay/PASS.txt. Subsequent CMake changes add masking runtime;
comparison source snapshots preserve the exact earlier build inputs.
