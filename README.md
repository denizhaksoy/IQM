# Image Quality Metrics
This library provides Vulkan implementations of SSIM, FSIM, FLIP, PSNR, LPIPS
image quality metrics.

For the validated HDR-VDP-3 professor demonstration (included image pairs,
reference checks and a local results report), see
[the demonstration guide](tests/HDRVDP3_DEMONSTRATION.md).

Dataset parity work is tracked in [UPIQ validation setup](tests/upiq/README.md).
The new [generic Double CPU path](tests/GENERAL_DOUBLE_QUALITY_VALIDATION.md)
passes original-size engineering comparisons, including 1024x683; it is separate
from the existing fixed-profile GPU CLI and is not UPIQ benchmark acceptance.

## Prerequisites
- C++ 20
- Vulkan 1.2+
- VkFFT (only for FSIM)

## Binary Usage
Main executable is `IQM`: 
```bash
IQM --method METHOD --input INPUT --ref REF [--output OUTPUT]
```

### Arguments:
- `--method <METHOD>` : selects SSIM, FSIM, FLIP, PSNR, LPIPS or HDRVDP3 (validated profiles described below)
- `--input <INPUT>` : path to tested image
- `--ref <REF>` : path to reference image
- `--output <OUTPUT>` : path to output image, optional
- `-v, --verbose` : enables more detailed output
- `-c, --colorize `: colorize final output
- `-h, --help` : prints help

### Method specific arguments:
#### PSNR:
- `--psnr-variant <VAR>` : One of `rgb`, `luma` or `yuv`
#### FLIP:
- `--flip-width <WIDTH>` : Width of display in meters
- `--flip-res <RES>` : Resolution of display in pixels
- `--flip-distance <DISTANCE>` : Distance to display in meters

## Library Usage
Example library usage can be found in `/bin/shared/wrappers` folder for each implemented method.
Simplified it can look like this:

```c++
IQM::FLIP flip(device); // init method with `vk::raii::Device`

// populate input structure with Vulkan objects and arguments to method
auto flipInput = IQM::FLIPInput {
    .args = flipArgs,
    ....
};

commandBuffer->begin();
flip.computeMetric(flipInput);
commandBuffer->end();
```

## Compilation
- needs properly configured Vulkan SDK (Tested on Ubuntu 25.04), or packages from system (Arch Linux)
  - in case of bad setup, link errors or missing includes will appear
- for FSIM, git submodule with `VkFFT` must be fetched
- before C++ compilation compile shaders by `./compile_shaders.sh`
- after compilation copy `lpips.dat` next to executable
## HDR-VDP-3 development status

The experimental `evaluate_pathway_quality` API evaluates positive Float32
luminance pairs at80x96/96x80,30ppd,age24,D65 using explicit shader/model assets.
Six synthetic pairs pass487 stage checks/1966412 compared words bitwise against
official HDR-VDP-3 v3.0.7,including final Q/Q_JOD and a repeated score-only call.
CPU model/LUT preparation,double NCSF and synchronous host transfers remain.
The public API/CLI can use this profile with an explicit version1 runtime package;
30ppd/age24/fixed D65 only,no spectral overrides.

See [CLI/runtime packaging](tests/PATHWAY_CLI_VALIDATION.md),
[API validation](tests/PATHWAY_QUALITY_API_VALIDATION.md) and
[component validation](tests/ACTUAL_GPU_COMPOSITION_VALIDATION.md) for build
commands,scope and remaining integration work. Numerical exports,local builds
and compiled runtime assets are excluded from Git;validation scripts reference
the documented locally generated fixture tree and external official reference.

Generate this runtime directly from source (no existing validation exports or
native reference installation required for generation):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_pathway_runtime.ps1 -OutputDirectory validation/my_source_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

The ready package is `validation/my_source_runtime/runtime`. See
[source reproduction](tests/SOURCE_RUNTIME_REPRODUCTION.md) and
[identity checks](tests/PATHWAY_IDENTITY_VALIDATION.md). Generation is pinned
to the validated payload hashes;other compiler outputs require investigation.

[Small-contrast controls](tests/PATHWAY_CONTRAST_VALIDATION.md) additionally
pass ten pairs/690 stage checks bitwise for localized tiny gains,uniform gains
andconstant luminance. This is in-memory API validation within the same profile;
it does not represent new viewing conditions or real HDR dataset acceptance.

[Luminance and HDR-derived grayscale checks](tests/PATHWAY_LUMINANCE_AND_HDR_VALIDATION.md)
extend this profile with twelve low/high luminance and decreasing-contrast pairs,
plus photograph-derived grayscale crops with an explicit display mapping.
These checks retain strict intermediate-value comparisons; RGB and arbitrary
dimensions remain outside this acceptance scope.

The [96x96 public runtime](tests/SQUARE_PUBLIC_RUNTIME_VALIDATION.md) matches
official Q/Q_JOD and intermediate values bitwise on six image pairs through
the production API and CLI. Its version2 package is separate from the version1
80x96/96x80 package; both retain the fixed viewing conditions above.

Generate the square96 package from source into a fresh output directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square_runtime.ps1 -OutputDirectory validation/my_square_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

The ready package is `validation/my_square_runtime/runtime`. Source-only
regeneration reproduced all18 pinned payloads and the manifest exactly.
See the public runtime report for executable lineage, preserved legacy
regressions and verified CLI rejection routing. RGB, arbitrary
dimensions, other viewing conditions and fully GPU-resident execution remain
outside the accepted scope.

The [variable-size base mean/DC stage](tests/GENERAL_BASE_MEAN_VALIDATION.md)
passes strict isolated GPU comparisons at12 band lengths and a six-pair
full-chain diagnostic regression. This prepares additional dimensions;
production runtime profiles and their payload hashes remain unchanged.

The [general SPYR/base diagnostic](tests/GENERAL_BASE_PIPELINE_VALIDATION.md)
also validates80x80 from native P maps, alongside the three existing shapes:
220 checks /1,144,720 Float32 words match exactly. The subsequent
[public80x80 profile](tests/SQUARE80_PUBLIC_RUNTIME_VALIDATION.md) now completes
optical filtering through Q/Q_JOD for that fixed luminance/viewing profile:
five pairs pass345 checks /990,565 values exactly through the public API,
and their CLI exports match the reference and API bits.

Generate its separate version3 package from source:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square80_runtime.ps1 -OutputDirectory validation/my_square80_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use `validation/my_square80_runtime/runtime` with `--hdrvdp3-runtime`.
The version1/version2 profiles retain their original payloads. These fixed
profiles do not establish RGB, arbitrary-size or general viewing-condition
support; CPU model/LUT/NCSF processing and host transfers remain.

The separate [modern128square profile](tests/SQUARE128_PUBLIC_RUNTIME_VALIDATION.md)
passes414 checks/3042894 values bitwise on six pairs, including a one-ULP
dominant-frequency stress case. Its version4 runtime uses FFT256 and map14;
the v1-v3 payloads remain unchanged.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square128_runtime.ps1 -OutputDirectory validation/my_square128_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use `validation/my_square128_runtime/runtime` with `--hdrvdp3-runtime`.

RGB-native96×96 quality is available through the production API and CLI at
30ppd/age24. Inputs must be finite nonnegative linear native display primaries
in color `PF` files; no sRGB EOTF or implicit color conversion is performed.
See [RGB CLI validation](tests/RGB_CLI_VALIDATION.md) for the measured scope.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_rgb_runtime.ps1 -OutputDirectory validation/my_rgb_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
IQM-hdrvdp3 --method HDRVDP3 --input test.pfm --ref reference.pfm --hdrvdp3-encoding rgb-native --hdrvdp3-display led-lcd-srgb --hdrvdp3-spectral-data C:/HDR-VDP-3_Ref/data --hdrvdp3-runtime validation/my_rgb_runtime/runtime
```

The source generator checks18 base payload pins and4 RGB shader pins. Original
cone/rod/display spectral tables are explicit CLI inputs and are not bundled.

Additional fixed-profile RGB validation covers identity, constant HDR colors,
pure-primary regions, HDR impulses and an HDR-derived three-channel color
crop. See [RGB content validation](tests/RGB_CONTENT_VALIDATION.md) for exact
counts, source mapping and the remaining limits.

Linear `rgb-bt.709` is also available at96×96/30ppd/age24 through a distinct
709 runtime (not the native RGB package). See
[BT.709 CLI validation](tests/RGB709_CLI_VALIDATION.md) for a ready local command.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_rgb709_runtime.ps1 -OutputDirectory validation/my709 -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use `validation/my709/runtime` with `--hdrvdp3-encoding rgb-bt.709` and explicit
display/spectral-data options. Encoded sRGB uses the separate Double96 runtime described below.

Additional709 validation covers five further content pairs across all three
display spectra, including the HDR-derived color crop. See
[BT.709 content validation](tests/RGB709_CONTENT_VALIDATION.md).

Linear BT.2020 and XYZ are available at96×96/30ppd/age24 with matching strict
profiles. Generate both packages with
`tests/hdrvdp3_evaluation/generate_linear_color_runtimes.ps1` and select
`--hdrvdp3-encoding rgb-bt.2020` or `xyz`. See
[linear color validation](tests/LINEAR_COLOR_VALIDATION.md) for exact scope
and a ready local command. Encoded sRGB is available through the separate fixed Double96 CLI profile below.
Separate CPU sRGB work now validates [display preprocessing](tests/SRGB_DISPLAY_VALIDATION.md)
and [Double optical/adaptation stages](tests/SRGB_OPTICAL_VALIDATION.md);
the latter's independent FFT diagnostic uses local Octave FFTW and does not
establish production GPU or full sRGB quality support.
The [Double pupil/JND stage](tests/SRGB_JND_VALIDATION.md) also matches the
reference through combined perceptual response P on two encoded pairs;
The [Double pyramid](tests/SRGB_PYRAMID_VALIDATION.md) now also matches on
these two pairs. [Double masking and score stages](tests/SRGB_MASKING_VALIDATION.md)
also match at explicit stage boundaries. [Independent sRGB CPU scores](tests/SRGB_FULL_CPU_VALIDATION.md)
now match Q/Q_JOD on two distorted pairs plus an identity control across three
spectra. These CPU results are the historical baseline for the Double96 hybrid CLI below.
Initial [Double GPU FFT stages](tests/DOUBLE_GPU_FFT_VALIDATION.md) now match
on an explicitly selected Float64-capable NVIDIA device.
The [complete Double192x192 forward FFT](tests/DOUBLE_GPU_FORWARD192_VALIDATION.md)
now matches72 padded matrices; inverse transforms/filtering and the complete
Double GPU quality path remain unfinished.


Encoded sRGB is now available through `--hdrvdp3-encoding srgb-display` with
its distinct `double96.runtime` package:96x96 color PFM in[0,1],30ppd,age24,
quality/defaultsp0. Model data is inside the package;no spectral override.
All FFT/convolution/crop operations run on a Float64-capable GPU;remaining
visual/quality calculations use CPU. See [public sRGB CLI validation](tests/SRGB_PUBLIC_CLI_VALIDATION.md)
for package generation,commands,exact tested scope and remaining limitations.

Double96 runtime v2 adds GPU optical clamp,spectral RGB-to-LMSR projection,
LMSR clamp and adaptation-input sum. Add `-GPUProjection` to the package
builder to select it;v1 remains supported. See
[Double GPU projection validation](tests/DOUBLE96_GPU_PROJECTION_VALIDATION.md).

Double96 runtime v3 adds bitwise-validated GPU3x3 mutual masking.
Build with `-GPUMasking` (includes GPU projection);v1/v2 remain supported.
See [Double GPU mutual masking](tests/DOUBLE96_GPU_MUTUAL_VALIDATION.md).

Double96 runtime v4 adds GPU bicubic neighbor masking with source-generated
Double axis weights. Build with `-GPUNeighbor` (includes mutual/projection).
See [GPU neighbor masking validation](tests/DOUBLE96_GPU_NEIGHBOR_VALIDATION.md).

Double96 runtime v5 adds the complete GPU SP0 pyramid,band normalization,
compensated base mean andDC removal. Build with `-GPUPyramid`;v1-v4 remain
supported. See [GPU pyramid validation](tests/DOUBLE96_GPU_PYRAMID_VALIDATION.md).

Double96 runtime v6 adds GPU relative-RGB comparison decisions and bicubic
comparison maps. Build with `-GPUComparison` (includes priorGPU stages).
See [GPU comparison validation](tests/DOUBLE96_GPU_COMPARISON_VALIDATION.md).

Double96 runtime v7 adds GPU pupil multiplication,JND LUT lookup/interpolation
and P addition. CPU log/exp query preparation remains explicit. Build with
`-GPUJND`;see [hybrid JND validation](tests/DOUBLE96_GPU_JND_VALIDATION.md).

Double96 runtime v8 adds GPU CSF resize/clamp/interpolation,neural-noise
reciprocal andbase-log mean;CPU model/base nonlinear evaluation stays explicit.
Build with `-GPUCSF`;see [CSF map validation](tests/DOUBLE96_GPU_CSF_VALIDATION.md).

GPU quality arithmetic and normalized pooling (hybrid runtime v9): see [validation scope](tests/DOUBLE96_GPU_QUALITY_VALIDATION.md). CPU powers and final scalar prediction remain.

Runtime v10 moves JND adaptation/query logarithms to GPU while retaining CPU exp/pow and model preparation: [validation scope](tests/DOUBLE96_GPU_LOG_VALIDATION.md).

### Single CPU quality for decoded LIVE HDR frames

On Windows, `IQM-hdrvdp3-cpu` is an explicit CPU/FFTW application built by the
main project and backed by the Single quality library API:

```powershell
cmake --build build-vs2026 --config Release --target IQM-hdrvdp3-cpu
& build-vs2026/bin/Release/IQM-hdrvdp3-cpu.exe --help
```

It requires explicit `--encoding pq-bt2020-rgb16`, `--ppd`, `--model-data`,
`--fftw`, `--input`, `--ref` and a fresh `--output` directory. Inputs are RGB16
PNG frames already decoded into PQ BT2020 RGB. The profile uses quality/sp0,
age24, OLED, ambient200lux, reflectivity0.005 and peak2000cd/m² before ambient.
Lower-resolution distorted frames are bicubic-resized in PQ before conversion
to absolute linear RGB. It does not initialize Vulkan or replace the existing
multi-metric IQM executable's GPU profiles.

The [daily validation record](tests/HDRVDP3_DAILY_PROGRESS_2026-11-01.md) documents
two complete selected-frame video comparisons and a second-content frame check.
See [CLI and 4K GPU stages](tests/PUBLIC_SINGLE_CPU_AND_GPU4K.md) for exact scope,
dependencies and commands. The 4K Vulkan matrix-stage results do not imply a
complete GPU quality pipeline or full LIVE HDR dataset acceptance.
