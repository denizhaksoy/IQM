# Production RGB API and runtime

2026-10-27 checkpoint: see `validation/rgb_api_active_path.txt`.

`evaluate_pathway_rgb_quality` now accepts original three-plane row-major
Float32 RGB96 data and explicit shared-grid spectral samples. Production
code performs optical filtering, spectral calibration, projection, adaptation,
pupil/JND, RGB masks and downstream quality. It has no test-header or native
intermediate dependency. All returned trace planes use row-major layout.
Scope is rgb-native,96×96,30ppd,age24,default quality/sp0. No conversion from
sRGB/BT.709/BT.2020 is implied. CPU models/LUTs/NCSF and host copies remain.

`rgb_quality.cpp` is registered in the main HDRVDP3 library CMake target with
strict floating-point options. The acceptance target compiles this production
source and dependencies directly. A full application/library build is not
claimed by this isolated-target verification.

Direct explicit-asset API acceptance passes **93 checks / 949,134 Float32
words bitwise**, including original-to-P traces, band masked differences,
band/accumulated errors and Q/Q_JOD. Packaged API acceptance repeats the same
93 checks /949,134 words with identical results. These are overlapping runs
of three display spectra of one synthetic RGB96 pair; native3.0.7/Octave11.3
parameters and scores are recorded in `RGB_QUALITY_VALIDATION.md`.

## Runtime contract

Separate header: `IQM_HDRVDP3_RGB 1 rgb-native-30ppd-age24-sp0-square96`.
`rgb.runtime` contains exactly five SHA256-bound entries: base manifest,
projection, adaptation, ABI17 JND map, RGB comparison. A confined nested v2
base retains its original manifest and18 payload files. The RGB overlay adds
four shader payloads. Old luminance packets and loader behavior are preserved.
Sensor/display spectral data remain explicit caller inputs, not bundled data.

`load_rgb_pathway_runtime` enforces geometry/profile, required/unique keys,
relative confined canonical paths, payload digests and SPIR-V headers. The
nested base loader independently checks its models and asset hashes. ABI17
is additionally checked when MapGPU initializes for computation. This is not
shader semantic verification or a source-reproduction proof.

Contract tests accept the valid package and reject twelve malformed variants
without GPU: version,profile,missing,duplicate,unknown,traversal,absolute path,
digest,corrupt payload,invalid SPIR-V,nested invalid model,nested wrong profile.
Nested cases update the parent hashes so validation reaches the inner failure.
No Vulkan validation errors were reported by either numerical acceptance run.
After the loader addition, the rebuilt original v4 luminance competing_axes
regression passes 69 checks /507,149 mixed values bitwise. The initial direct
API run predates the optional packaged checker arguments; the frozen final
binary corresponds to packaged acceptance. Its numerical algorithm is unchanged.

## Usage

```cpp
#include <IQM/hdrvdp3/pathway_runtime.h>
auto assets = IQM::HDRVDP3::load_rgb_pathway_runtime(runtimeDirectory, 96, 96);
auto spectra = IQM::HDRVDP3::load_rgb_spectral_samples(coneFile, rodFile, emissionFile);
auto scores = IQM::HDRVDP3::evaluate_pathway_rgb_quality(
    instance, referencePlanarRGB, testPlanarRGB, 96, 96, spectra, assets);
```

Package existing accepted assets with `tests/hdrvdp3_evaluation/package_rgb_runtime.ps1`:
arguments `-BaseRuntime`, `-Projection`, `-Adaptation`, `-JND`, `-Comparison`,
`-OutputDirectory` (fresh). Validate with `validate_rgb_runtime.ps1`, using
the `rgb-runtime-test` target from `tests/hdrvdp3_residual144`.
Run `rgb-quality-api-test <native_bands> <RGB-runtime> <raw-spectral-data>
<fresh-output>` for packaged numerical acceptance. Explicit asset arguments
remain supported for provenance comparison.

Next implement explicit PF RGB input and API/CLI options, plus source-only
generation/pinned reproduction for the RGB runtime. Expand independent RGB
contents and real HDR color before broader parity claims. This checkpoint
does not yet provide a ready RGB CLI command or all-condition metric parity.
