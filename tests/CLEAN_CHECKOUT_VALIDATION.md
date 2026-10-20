# Clean committed-source validation

Commit `7c7f4736c324ce2d648151b6f21003f519132f95` promotes the bounded
quality API and its minimal runtime sources into18 scoped files. Main wrapper,
root/source CMake integration experiments, staged test shader and unrelated
working changes were excluded. This commit is local; it has not been pushed.

A detached clean worktree was created from this commit. Its VkFFT submodule
was initialized from the local Git repository at the recorded commit
`539be29f30ef40384f3ccd24d7884166f82cbef8`, without copying dirty working files.
The main worktree's dirty submodule and staged shader remained unchanged.

The independent project `tests/hdrvdp3_evaluation` configured and built in an
external fresh build directory with Visual Studio18 2026, CMake4.4 and Vulkan
SDK1.4.363.0. No root cache, workspace shader headers, wrapper, other metrics
or dependency downloads were used. It regenerated seven embedded shader
headers; all hashes match the preceding verified build.

## Measured scope

- Positive absolute luminance,32/64 square,30ppd,age24,D65 raw spectral loading.
- Two photo-derived localized-distortion pairs; the grayscale crops use the
  earlier explicit Rec709 numeric Y and1cd/m² per Y unit assumption. This is
  not independent photometric calibration or RGB-native metric validation.
- Returned error/Q/Q_JOD bits match the pinned native results.32Q/Q_JOD:
  8.0709753036499023/8.7933588027954102;64:
  9.2685623168945312/9.6516733169555664.
- All203 API-generated numerical text exports match prior baseline hashes
  (96 at32,107 at64). Six provenance filenames are excluded. This is hash
  equivalence to the previously native-validated baseline, not a new Octave
  execution or a new GPU-intermediate checker run.
- Sixteen poisoned legacy environment variables do not affect the API.
  Fourteen invalid input/configuration cases are rejected.
- Parent checkout and submodule remain clean after execution. The native
  runs use Intel Arc Pro Graphics on the existing validation machine.

Evidence root: `validation/clean_api_639281156569491989`.
`COMPLETED.json` pins the tested commit, clean statuses and per-case paths;
`configure.log`, `build.log`, `run32.log`, `run64.log`, `rejected.log`,
`binary_hash.json` and `assets/manifest.json` retain the detailed evidence.
The local driver is `validation/validate_clean_api.ps1` and requires a fresh
work path. Source checkout and build directory are separate.

## Build and remaining asset work

```powershell
git submodule update --init lib/VkFFT
cmake -S tests/hdrvdp3_evaluation -B <fresh-build-directory>
cmake --build <fresh-build-directory> --config Release -j 2
```

The direct caller arguments are documented in `tests/evaluate_quality_api.cpp`.
An isolated runtime asset directory was prepared with14 copied and SHA256-pinned
files: two FFT shaders, map/base FFT shaders, three math shaders, three original
spectral tables and four input PFMs. Golden text files are comparison evidence,
not additional runtime dependencies. No original files were modified.

This checkpoint proves clean committed-source building and evaluation with
explicit pinned external assets. It does **not** regenerate the external
reference FFT/math/map/base FFT assets from committed inputs. Their generator
dependencies, source provenance and reproducible acquisition still need to be
packaged. It is not yet a complete self-contained distribution. General image
dimensions, RGB-native encodings and full-frame validation remain open.
