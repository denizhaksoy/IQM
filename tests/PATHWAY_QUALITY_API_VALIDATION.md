# Single in-memory HDR-VDP-3 pathway quality API

`evaluate_pathway_quality` accepts the original row-major positive Float32
luminance pair,dimensions,an abstract Vulkan instance and explicit model/shader
assets. It returns `QualityScores` and optionally commits a diagnostic trace
only on success. The fixed experimental profile is80x96/96x80,30ppd,age24,D65,
sp0Filters. This isolated API does not change existing public CLI geometry guards.

One call executes MTF ->L+M adaptation preparation ->LA ->pupil/JND/P ->mean/log
adaptation ->SPYR/bands/DC/residual ->mutual pooling ->comparison masks ->CSF/
masking/base noise ->quality scores. Intermediate arrays pass directly between
functions in memory. Only explicit shader assets are read at runtime; original
images and fixed sp0 coefficients are supplied by the caller. There are no
intermediate image files or environment reads. CPU model/LUT preparation,
double NCSF and synchronous host transfers remain. This is not a GPU-resident
command graph or arbitrary-size/viewing-configuration support.

`IQM-pathway-quality-test` reads original images and fixed sp0 model before
calling the API. It reads independent native image-dependent expected values
after the call returns,checking81stages including retinal/adaptation/P/log,
all11SPYR stages,pooling,comparison,masking/base noise and Q/Q_JOD bits.
The first asymmetric80x96 case additionally repeats the entire call without
tracing on the same Vulkan instance,requiring identical score bits.
`IQM-pathway-quality-contract-test` checks13 malformed inputs using an instance
that throws on any GPU access,requiring rejection and unchanged output trace.

```powershell
cmake -S tests/hdrvdp3_evaluation -B <fresh-build> -G 'Visual Studio 18 2026' -A x64
cmake --build <fresh-build> --config Release --target IQM-pathway-quality-test IQM-pathway-quality-contract-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/validate_pathway_quality.ps1 -Executable <fresh-build>/Release/IQM-pathway-quality-test.exe -OutputDirectory <fresh-results>
& <fresh-build>/Release/IQM-pathway-quality-contract-test.exe
```

Checkpoint: `validation/pathway_quality_1792914761769`,Intel Arc Pro Graphics.
Original/fixed-model/config/shader/source/binary hashes and source snapshots are
recorded. Fixed sp0 files are identical across all three contents;they are model
data,not image-dependent intermediates. Native reference installations and old
artifacts remain unchanged. A first PowerShell hash-list parse error was fixed
before any matrix execution;it was not a numerical failure.

Final acceptance: six original image pairs pass487 stage comparisons and
1966412 compared words bitwise,including the additional same-instance score-
only repeat. All13 input contract checks pass;no Vulkan validation errors.
Evidence: matrix/PASS.txt,matrix/results.json andcontract.log. Counts include
repeated binary masks and the two repeated score words;these are not unique
pixels or extra image pairs. Final Q/Q_JOD matches independent native results
for asymmetric,impulse,HDR ramp in both orientations. This is now a validated
single in-memory API,not merely the earlier file-composed experimental replay.
Public application/CLI runtime packaging and broader support remain unfinished.

Subsequent public integration: evaluate_quality andCLI now route the validated
80x96/96x80 profile through an explicit version1 package. SixPFM CLI nativebit
checks,three unsupported settings,12 malformed packages andlegacy11pair/1166
export regression pass. See PATHWAY_CLI_VALIDATION.md. Earlier acceptance above
remains the independent81stage in-memory API scope;do not merge these counts.
