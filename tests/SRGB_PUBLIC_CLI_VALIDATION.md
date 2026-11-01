# Public sRGB PFM CLI

Validated2026-10-29. Completed artifacts:
`validation/srgb_public_cli_completed_path.txt`.

The main IQM command now supports `--hdrvdp3-encoding srgb-display` with
96x96 encoded colorPFM inputs in[0,1],30ppd,age24 and quality/defaultsp0.
It loads the separate Double96 runtime and model data,selects a Float64
compute device,enables the logical feature,and uses the production GPU
backend and capture-free metric API. PFM row-major planes are transposed
without arithmetic to the API column-major layout;the API applies the
verified mixed-precision sRGB display preparation. There is no extra gamma,
resizing or conversion in the CLI adapter.

All9public CLI configurations(two distorted pairs+identity control,three
spectra) match official HDR-VDP-3v3.0.7 exactly for error,Q,Q_JOD and5band
errors:72Double scalars. All497,664 decoded Float32 input values also match
the original encoded inputs exactly. One configuration uses big-endian,
scale2 PFM for both images;its decoded values and scores are exact.
This reuses earlier images,not new image/geometry coverage. Prior production
backend acceptance of13,937,256 intermediate Double values is separate.

13negative CLI cases reject:unsupportedppd/age/display,spectral override,
unsupportedIntel/missingdevice,wrongruntime,existingexport,over-one/negative/
NaN/Inf pixels,and wrong geometry. Device override is allowed only for sRGB.
AllnewCLI Vulkan logs are clean. The prior linearBT.2020OLED CLI path was
rerun on its default Intel device;all16export files remain byte-identical.

Generate a package:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory validation/my_double96 -RawData C:/HDR-VDP-3_Ref/data
```

Run the main executable:

```powershell
& .\build-vs2026\bin\Release\IQM.exe --method HDRVDP3 --input test.pfm --ref reference.pfm --hdrvdp3-encoding srgb-display --hdrvdp3-display oled --hdrvdp3-runtime validation/my_double96/runtime --enable-validation
```

Optional:`--hdrvdp3-device NVIDIA` selects a matching Float64 device;the
default filters devices for Float64 automatically. `--hdrvdp3-export` must
name a fresh directory. Otherwise a fresh validation directory is generated.
Do not supply `--hdrvdp3-spectral-data`:the package owns the model data.
Outputs:quality.txt(5banderrors then error/Q/Q_JOD),profile.txt,and two
planar row-major Float32 decoded-input exports. A ready command with real
fixture paths is saved as run_example.ps1 in the validation root;execute
with process-local PowerShell ExecutionPolicy Bypass if needed.

The tested device is NVIDIA RTX A500;otherFloat64 hardware is not numerically
validated. IntelArcPro lacks Float64 and is rejected. All FFT/convolution/crop
operations are GPU;display preparation,spectral/JND/pyramid/masking/pooling
remain CPU. No native FFTW or native captures are required by the metric.
General geometry/viewing/other prediction tasks and completeGPU processing
remain open. This is a usable fixed-profile hybrid CLI,not fullGPU/all-input
HDR-VDP-3 completion.

MainRelease build passes. Existing unrelated compile warnings are preserved.
The validator is tests/hdrvdp3_evaluation/validate_srgb_public_cli.cjs,requiring
a fresh root with lineage.json. It writes fixtures,exact commands,logs and
results.json/CLI_PASS.txt. Old references/shaders_out/VkFFT changes preserved.
