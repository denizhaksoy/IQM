# HDR-VDP-3 demonstration

From this folder, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Run-Demo.ps1 -OpenReport
```

Four examples run automatically and write a fresh `results/run_...` folder.
The local HTML report displays both inputs and Q/Q_JOD scores. Every run checks
all eight quality values and the binary exports against preserved official
HDR-VDP-3 v3.0.7 comparisons. A mismatch stops the demonstration.

Requirements: Windows x64, installed Vulkan GPU driver, and an enabled Float64
GPU for the sRGB examples (tested NVIDIA RTX A500). This uses the existing
MSVC runtime installation. No compiler, Octave, Node, external reference source,
internet connection or original repository paths are needed to run the package.
For an additional Vulkan validation run, add `-EnableValidation`; this requires
the Vulkan validation layer to be installed. Runs include startup and file
exports, so their durations are not GPU benchmark timings.

The package contains sRGB stress, ramp and identity fixtures, plus a real
HDR-derived color photograph crop. Original PFM files are the metric inputs;
PNG previews never enter the calculation. HDR previews use a separate display
mapping. The reference photograph is the Memorial Church HDR radiance map by
Paul Debevec/Jitendra Malik; see HDR_PROVENANCE.md for crop, scaling and source
hash. The crop is a numerical stimulus interpreted in native display primaries,
not proof of absolute capture luminance or original camera colorimetry.

Validated demonstration scope: 96x96, 30 pixels/degree, observer age 24, OLED
spectrum, quality/default sp0. The implementation is hybrid CPU/GPU. It does
not claim arbitrary image sizes, all viewing conditions, every prediction mode,
all hardware or a fully GPU implementation. See SCOPE.md.

To evaluate another 96x96 encoded sRGB PFM pair:

```powershell
.\IQM.exe --method HDRVDP3 --hdrvdp3-encoding srgb-display --hdrvdp3-display oled --hdrvdp3-runtime .\runtime\srgb --ref .\reference.pfm --input .\test.pfm --hdrvdp3-export .\my-new-results
```

Input values must be finite encoded RGB in [0,1]; choose an unused results
directory. Do not resize an arbitrary HDR image and interpret it as sRGB.
The score for an arbitrary new image is not independently reference-validated
merely because the included demonstration examples pass.
