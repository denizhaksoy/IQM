# Public Single CPU tool and 4K Vulkan matrix stages

2026-11-01. These additions extend the previously validated CPU pipeline.
They do not establish full GPU image-to-score or full dataset equivalence.

## CPU library and executable

`general_single_pathway.cpp` adds `prepare_quality_side_single` and
`evaluate_general_quality_single` for calibrated Single BT2020 RGB. The model
profile is quality/sp0, age24, with explicitly supplied model samples and FFT
provider. It contains no reference scorer or reference tensor fallback.

The main CMake project now builds `IQM-hdrvdp3-cpu`. It uses the library API,
explicit Float FFTW DLL, original RGB16 samples, PQ-domain bicubic distortion
resize, inversePQ and the source-example bright display model. It does not
initialize Vulkan. The ordinary multi-metric `IQM` executable's existing GPU
profiles remain unchanged.

Actual verified CLI results:

| Pair / frame | Reference and CLI Q | Reference and CLI Q_JOD | Hex Q / Q_JOD |
|---|---|---|---|
| Daylight1 540p_2.2M ->4K, frame0 | 4.7725744247436523 | 5.6721343994140625 | 4098b8ee / 40b58220 |
| Bonfire 720p_2.6M ->4K, frame0 | 7.088564395904541 | 7.9553570747375488 | 40e2d585 / 40fe9249 |

Both were compared to unmodified official v3.0.7 CPU Octave11.3.0 on identical
hashed source PNGs and settings. The second check expands content coverage
beyond Daylight1 and verifies a different spatial scale. The reference canvas
is3840x2160 in both cases. These are frame results, not two new full videos.

Seven CLI checks passed: help, ambiguous encoding, nonfinite ppd, trailing ppd
text, missing encoding, duplicate ppd, and preservation of an existing output
directory/sentinel. Required encoding is explicit; unsupported inputs are not
silently interpreted as sRGB or linear HDR.

From the repository:

```powershell
cmake --build build-vs2026 --config Release --target IQM-hdrvdp3-cpu
& build-vs2026/bin/Release/IQM-hdrvdp3-cpu.exe --input validation/live_hdr_540p_frame0_20261101_run01/test.png --ref validation/live_hdr_540p_frame0_20261101_run01/reference.png --encoding pq-bt2020-rgb16 --ppd 62.729130063099447 --model-data validation/live_hdr_single_demo_20261101_run03/data --fftw 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll' --output <fresh-output-directory>
```

Profile constants: OLED, age24, ambient200lux, reflectivity0.005, peak2000cd/m²
before ambient. PPD must be specified and in[26,256]. Current image inputs are
RGB16 PNG already decoded into PQ BT2020 RGB. There is no implicit codec/color
policy for other file formats. Header-only Windows FFTW provider is explicit
in `fftw_single_provider.h` and requires a compatible external DLL.

Artifacts: `validation/public_cpu_cli_checks_20261101_run01`,
`public_cpu_540p_20261101_run01`, `public_cpu_bonfire720_20261101_run01`;
frozen used executable/source under `public_cpu_cli_acceptance_20261101_run01`.

## 4K Vulkan matrix acceptance

`projection_single_gpu.h/.cpp` adds an explicit library Vulkan operation for
native RGB conversion and LMSR projection. Input coefficients are prepared on
CPU; per-pixel multiplication/FMA and gamut/response clamps execute on GPU.
Optical inputs for the LMSR check come from the independently validated CPU FFT.
No full GPU optical/adaptation/JND/quality claim follows from this test.

The new `hdrvdp3_rgb_projection_2d.comp` uses a two-dimensional dispatch. A
one-dimensional dispatch would exceed the workgroup-count limit at full4K.
Storage-range and geometry are checked; accepted maximum is3840x2160 pixels.
Legacy shaders and previous binaries were preserved; only the new shader was
compiled into fresh validation directories.

| GPU stage, both images | Values | Bit differences |
|---|---:|---:|
| BT2020 -> native RGB | 49,766,400 | 0 |
| Optical native RGB -> LMSR | 66,355,200 | 0 |
| Total | 116,121,600 | 0 |

Device: Intel(R) Arc(TM) Pro Graphics. Vulkan validation was enabled and no
validation errors were reported. The library-backed test repeated the same
exact results, not only the standalone shader prototype.

Acceptance roots:

- `validation/live_hdr_gpu4k_acceptance_20261101_run01`: initial LMSR-only test.
- `validation/live_hdr_gpu4k_acceptance_20261101_run02`: both matrix modes.
- `validation/live_hdr_gpu4k_library_20261101_run01`: public library API repeat.

Shader binaries: `validation/live_hdr_gpu4k_20261101_run01` and `run02`.
`tests/run_live_hdr_gpu_projection.cjs` records executable/shader hashes and
retains stdout/stderr; it rejects reported validation errors and nonzero exits.
No GPU speedup is claimed: transfer and kernel times have not been isolated.
