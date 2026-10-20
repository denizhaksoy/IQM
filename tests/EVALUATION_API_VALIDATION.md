# Bounded luminance quality API

The bounded runtime source set is now committed in7c7f473 and independently
validated from a clean worktree. See [clean-source checkpoint](CLEAN_CHECKOUT_VALIDATION.md).
Earlier source-snapshot evidence below retains its original scope.

The local runtime now exposes `IQM::HDRVDP3::evaluate_quality` from
`include/IQM/hdrvdp3/evaluation.h`, implemented in
`src/hdrvdp3/evaluation.cpp` and compiled into `IQM-HDRVDP3`.
The quality arithmetic was extracted from the previously validated wrapper.
The library implementation does not parse process environment variables or
generate/transform image inputs. The wrapper prepares inputs/configuration
and calls this API. Historical corrDn/SPYR probe modes remain in the wrapper.

## Contract

- Two `LuminanceImage` objects: positive finite absolute cd/m², top-down
  row-major binary32 pixels, equal square32/64 dimensions and exact pixel count.
- `EvaluationOptions`: explicit30/60 pixels per degree, observer24/60/60.001,
  column-first reference FFT shader, native twiddles, reference math shader
  directory, ABI13 map shader and base FFT shader. The FFT shader must match
  the selected image size (64-point FFT for32,128-point FFT for64).
- Spectral configuration: original D65/cone/rod data directory, or already
  loaded `SpectralSamples`, mutually exclusive. If omitted, only historical
  age24 spectral constants are available. The API does not infer encodings,
  display primaries or absolute photometric calibration from an image file.
- Required `EvaluationTrace`: a fresh directory, plus optional provenance tags.
  Tags are written to diagnostics and do not control the calculation.
- Returns `EvaluationResult { QualityScores scores; vector<float> bandErrors; }`.
  Errors propagate as exceptions; callers must not treat failed calls as scores.
- Synchronous execution reuses the caller's `IQM::VulkanInstance` command buffers;
  concurrent use of that same instance is unsupported.

This first API deliberately requires diagnostic export. It still uses host
preparation for some coefficients/LUTs and existing shared Vulkan resources.
General dimensions, RGB-native input, zero luminance policy, full original
frames and a clean-checkout package remain separate unfinished work.
The test harness now rejects unvalidated non-FFT/CPU quality configurations
instead of routing them through the old preliminary quality calculation.
The old optional constant-image comparison was removed from the quality path;
its source snapshot and original reference files are preserved locally.

## Call shape

```cpp
#include <IQM/hdrvdp3/evaluation.h>

IQM::HDRVDP3::EvaluationOptions options;
options.fft.referenceShaderPath = fft_shader_for_image_size;
options.fft.referenceMathDirectory = reference_math_directory;
options.mapShader = abi13_map_shader;
options.baseFFTShader = base_fft_shader;
options.spectralDataDirectory = original_data_directory;
auto result = IQM::HDRVDP3::evaluate_quality(
    instance, reference_luminance, test_luminance, options,
    IQM::HDRVDP3::EvaluationTrace{fresh_export_directory});
// result.scores.Q, result.scores.Q_JOD, result.bandErrors
```

## Validation evidence

Checkpoint directory: `validation/evaluation_api_639281145664415302`.
The before directory preserves the previous wrapper/header, CMake list and
verified executable. The initial build failure (missing extracted transpose
helper) is retained in `build_initial_failed.log`; the corrected build passes.

The full regression driver is
`validation/run_evaluation_api_regression.ps1`. It uses fresh native runs,
strict preprocessing, native JND traces, raw and centered SPYR, CSF, masking,
difference maps, official Q/Q_JOD and independent GPU intermediate comparisons.
It compares text export hashes against the preceding luminance checkpoint.
Only three asset path provenance files are excluded from these hash checks.
Completed suite paths are taken from per-run `Suite PASS:` lines.

`IQM-quality-api-test` is a separate caller linked to the library without
`bin/shared/wrappers/hdrvdp3.cpp`. It poisons sixteen legacy environment
variables, then supplies actual inputs/assets explicitly. Its direct32/64
cases compare returned error/Q/Q_JOD bits and every API-generated numerical
text export against the pinned baseline. Native reference files added later
by the full suite are not API-generated output. An initial comparison script
incorrectly expected those extra files; the preserved first direct32 run had
already returned exact scores. A fresh attempt corrects the comparison scope.

Its negative-input mode rejects14 cases: shape, pixel count, zero/negative/
nonfinite luminance, unvalidated viewing/age/FFT settings, missing map asset
and empty/existing trace directory. Invalid cases do not dispatch computation
or overwrite existing output directories.

All ten fresh full regressions pass: six photo-derived luminance pairs and
two synthetic controls at30ppd/age24, plus two HDR ramp/strong-localized
controls at60ppd/age60.001. Independent GPU comparisons cover **1,425,505
float values and59,520 binary64 mask values**, with zero differences.
Official Q/Q_JOD, native JND, raw and centered SPYR and other strict stage
checks pass. **1,218 full-suite text export hashes** remain identical to
the pinned baselines. Direct API32/64 tests additionally preserve **203
API-generated numerical text exports** and bitwise returned scores under
the poisoned environment. The14 invalid API calls are rejected.

`COMPLETED.json`, `direct_results.json`, `extended_results.json` and source/
binary hashes pin this scope. Local raw exports and binaries remain local
evidence, not a claim that the remote checkout includes all dependencies.
The subsequent CLI change propagates quality exceptions to `main`, where
they produce a nonzero exit. A32-input/64-selected mismatch now returns-1;
`cli_reject_dimension.json` records this separately from numerical results.
The API checkpoint preserves its pre-CLI-change executable and wrapper source.
An additional fresh32/asymmetric-localized full strict success regression
passes after the CLI change (57,046 floats,2,368 binary64 masks,zero differences;
116 unchanged text exports). `CLI_COMPLETED.json` and `cli_status_hashes.json`
pin the separate final build. These extra counts are not folded into the ten
API regression totals above.

## Independent build continuation

`tests/hdrvdp3_evaluation` configures and builds from a fresh directory without
the main cache, other metrics, dependency downloads or workspace shaders_out.
All seven embedded headers are regenerated from GLSL and match the previous
headers' SHA256s. The independently built direct caller passes32/64 scores
bitwise and preserves203 numerical export hashes under the poisoned environment.
See `ISOLATED_BUILD.json`, source/binary hashes and `isolated_direct_results.json`.
This verifies the current source snapshot. The full source set is not yet a
clean committed runtime package, and external FFT/math/map/baseFFT shader assets
remain explicit inputs. VkFFT tracked headers are unchanged;its dirty CMake
file and untracked glslang directory are preserved and unused by this build.
