# Integrated Single hybrid quality validation

2026-11-02. Scope: explicit Vulkan matrix stages within the full image-to-score
pipeline. FFT, adaptation, nonlinear/JND, pyramid and quality calculations remain
CPU operations; this is not full GPU quality or a speedup claim.

`SingleMatrixBackend` is an optional explicit provider in the general Single
front-end. It supplies native-color and LMSR projections. CPU model calibration
prepares the coefficients; Vulkan computes per-pixel FMA and clamps. Null provider
retains the CPU path. A failed explicit provider throws; it does not silently
fall back to CPU.

The main project builds `IQM-hdrvdp3-hybrid`, requiring the same explicit inputs
as the CPU application plus `--projection-shader`. Vulkan validation is enabled.
The output declares `hybrid-vulkan-matrices-cpu-fftw` and records four successful
matrix dispatches: two stages for each image.

| Case | Q, official = hybrid | Q_JOD, official = hybrid | Single hex Q / Q_JOD |
|---|---|---|---|
| Daylight1 4k_3M, frame30 | 7.4749040603637695 | 8.2962665557861328 | 40ef326a / 4104bd82 |
| Bonfire 720p_2.6M ->4K, frame0 | 7.088564395904541 | 7.9553570747375488 | 40e2d585 / 40fe9249 |
| Bonfire 1080p_1M ->4K, frame0 | 5.6833958625793457 | 6.6135272979736328 | 40b5de61 / 40d3a204 |
| CargoBoat 1080p_1M ->4K, frame0 | 1.6976785659790039 | 2.171440601348877 | 3fd94d88 / 400af8e2 |
| football1 1080p_1M ->4K, frame0 | 4.5036582946777344 | 5.384850025177002 | 40901df8 / 40ac50b1 |

All five complete runs matched clean official v3.0.7 CPU Octave11.3.0 bitwise in
Q and Q_JOD. Each verified four actual Vulkan dispatches and no reported
validation errors. Used executables/shaders, logs and provenance are preserved:

- `validation/live_hdr_hybrid_frame30_20261102_run01`
- `validation/live_hdr_hybrid_bonfire720_20261102_run01`
- `validation/live_hdr_hybrid_bonfire1080_20261102_run01`
- `validation/live_hdr_hybrid_cargoboat1080_20261102_run01`
- `validation/live_hdr_hybrid_football1080_20261102_run01`

Input PNG hashes, ppd62.729130063099447 and Single reference precision are
checked by `tests/run_live_hdr_hybrid.cjs`. Other settings remain age24/OLED,
quality/sp0, ambient200lux, reflectivity0.005, peak2000cd/m² before ambient,
CPU Float FFTW three threads/estimate. Lower-resolution distorted RGB16 data
are bicubic-resized/clamped in PQ before inversePQ; reference geometry stays4K.
Decoder precision/phase remain the documented local reconstruction choices.

CPU's seven argument/output checks passed again. Three additional backend
contract checks passed: hybrid requires a shader; CPU rejects the GPU-only
option; a nonexistent GPU shader fails without producing a CPU fallback score.
The16x16 crop used for failure checks is not counted as a numerical dataset case.
Artifacts: `validation/live_hdr_hybrid_contract_20261102_run01/checks`.

```powershell
cmake --build build-vs2026 --config Release --target IQM-hdrvdp3-hybrid
& build-vs2026/bin/Release/IQM-hdrvdp3-hybrid.exe --input validation/live_hdr_bonfire720_frame0_20261101_run01/test.png --ref validation/live_hdr_bonfire720_frame0_20261101_run01/reference.png --encoding pq-bt2020-rgb16 --ppd 62.729130063099447 --model-data validation/live_hdr_single_demo_20261101_run03/data --fftw 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/libfftw3f-3.dll' --projection-shader validation/live_hdr_gpu4k_20261101_run02/projection.spv --output <fresh-output-directory>
```

This checks final scores on five frame pairs from four contents. These hybrid
reruns are not counted as extra distinct dataset cases. CargoBoat and football1
full videos also passed their independent CPU/reference evaluations. These
results do not prove that every possible GPU/driver or
intermediate value in the entire pipeline is identical. Broader hybrid video
coverage and remaining GPU operations are still required.
