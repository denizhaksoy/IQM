# Image Quality Metrics
This library provides Vulkan implementations of SSIM, FSIM, FLIP, PSNR, LPIPS
image quality metrics.

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
- `--method <METHOD>` : selects method to compute, one of SSIM, FSIM, FLIP, PSNR, LPIPS
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
The public CLI geometry support has not been extended to this profile yet.

See [API validation](tests/PATHWAY_QUALITY_API_VALIDATION.md) and
[component validation](tests/ACTUAL_GPU_COMPOSITION_VALIDATION.md) for build
commands,scope and remaining integration work. Numerical exports,local builds
and compiled runtime assets are excluded from Git;validation scripts reference
the documented locally generated fixture tree and external official reference.
