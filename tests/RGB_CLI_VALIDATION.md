# RGB PF CLI and source runtime

2026-10-27: checkpoint pointer `validation/rgb_cli_completed_path.txt`.

Production `load_rgb_pfm` supports color PF with a strict three-line header,
bottom-up interleaved RGB raster, endian sign and once-applied Float32 scale.
It returns top-down row-major planes and accepts finite nonnegative values.
It rejects malformed dimensions/scales, wrong raster lengths, NaN/Inf and
negative samples. No gamma, resizing or color-space conversion is performed;
the original grayscale Pf loader is unchanged.

CLI requires `--hdrvdp3-encoding rgb-native`, an RGB runtime directory,
`--hdrvdp3-display led-lcd-srgb|led-lcd-wcg|oled` and an explicit spectral data
directory. RGB supports96×96,30ppd,age24 only. Display options on the default
luminance path are rejected. Inputs, model files and profile are checked before
creating exports; an existing export directory is never overwritten.

## Results

- Five PF decoding runs: two original96×96 inputs plus3×2 little-endian,
  big-endian and scale1.25 cases; exact Float32 bits/layout verified.
- Ten malformed PF cases rejected by CPU tests; six CLI argument/export cases
  rejected. Existing parser regression: five valid /22 invalid scenarios pass.
- Actual production CLI: three display spectra of the same synthetic RGB pair;
  **663,552 Float32 exported values +24 quality values bitwise exact** against
  official3.0.7/Octave11.3 captures. Inputs, optical RGB, LMSR, adaptation, P,
  band errors, accumulated error, Q and Q_JOD checked. Vulkan validation on;
  final logs contain no VUID/validation/error markers. Overlaps previous API
  cohorts; not three independent image-content pairs.
- Original v4 competing_axes luminance CLI: all seven exports, including
  quality text, byte-identical to the prior accepted files.
- The real application main/argument/wrapper sources were built in the isolated
  HDRVDP3 application target. Other metric dependencies are outside this build.

`acceptance/` retains the first harness failure: it used unsupported `--validate`
and stopped before numerical work. Corrected `acceptance_v2/` uses the actual
`--enable-validation` option and passes all checks. No numeric tolerance changed.

## Source runtime

`generate_rgb_runtime.ps1` regenerates the18 pinned v2 base payloads and four
pinned RGB shaders. `source_runtime_v3/` passes all22 hash checks; both manifests
and payloads are byte-identical to the accepted manual package, and the RGB
loader accepts it. This run reads repository source/dependencies, not native
MATLAB/data or previous numerical exports. It is a live-workspace source build,
not a new isolated clean-copy host application build.

The first source attempt could not discover the C++ compiler inside the sandbox.
The second got through the model build but PowerShell treated glslang's source
filename stderr output as an exception despite its success. Both attempts are
retained. Process-local escalated execution and exit-code-based compiler output
handling resolved these environment/harness failures for the fresh third run.

Raw cone/rod/display files remain explicit inputs; the generator does not need
them, but evaluation does. Runtime reproducibility does not broaden numerical
acceptance to other devices, viewing conditions or input encodings.

## Ready local command

Run from the IQM repository in PowerShell. It uses frozen accepted files and a
fresh export directory; the original spectral data installation must exist.

```powershell
$r = Get-Content validation/rgb_cli_completed_path.txt
& "$r/IQM-hdrvdp3-used.exe" --method HDRVDP3 --enable-validation --input "$r/acceptance_v2/test.pfm" --ref "$r/acceptance_v2/reference.pfm" --hdrvdp3-encoding rgb-native --hdrvdp3-display led-lcd-srgb --hdrvdp3-spectral-data C:/HDR-VDP-3_Ref/data --hdrvdp3-runtime "$r/source_runtime_v3/runtime" --hdrvdp3-export "validation/rgb_run_$([DateTime]::UtcNow.Ticks)"
```

For these sRGB-display *native linear* fixtures, expected Q is
8.650890350341797 and Q_JOD9.236832618713379. The display name does not imply
that encoded sRGB image values are accepted.

Next expand independent RGB content/real HDR color coverage and preserve old
luminance/profile rejection tests. Other sizes, viewing configurations and
prediction modes remain open; no universal metric completion claim.
