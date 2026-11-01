# Linear BT.709 runtime and main application CLI

2026-10-28 checkpoint: `validation/rgb709_cli_active_path.txt`.

Main IQM now accepts explicit `--hdrvdp3-encoding rgb-bt.709` for finite
nonnegative96×96 color PF inputs at30ppd/age24. It loads original CIE1931,
cone/rod and chosen display emissions, computes the independent709 transform,
performs GPU conversion/native1e-6 gamut clamp and calls the production709 API.
No captured matrix/intermediate is read. This is linear709, not encoded sRGB;
sRGB-display remains rejected.

## Package and reproducibility

Distinct manifest `rgb709.runtime`:
`IQM_HDRVDP3_709 1 rgb-bt.709-30ppd-age24-sp0-square96`.
It binds `native/rgb.runtime` and `assets/conversion.spv` by SHA256. The nested
native loader verifies its own manifest, base models and shader payloads.
Canonical path confinement, unique required keys, profile/geometry, digests
and SPIR-V headers are checked. A native-only package cannot silently serve
as a709 package, nor can709 substitute for a native package.

`generate_rgb709_runtime.ps1` regenerates23 pinned payloads:22 native RGB
shader/model files plus the conversion shader. Conversion SHA256:
8806e521f71f5834adf6be8627f1130c3837d5bc4338fe8113e28bf39dacb2e9.
The successful source build is pointed to by `validation/rgb709_source_active_path.txt`.
Its payloads/manifests match the accepted manual package, and the loader accepts
it. This is a live-workspace source build; no native installation/data or old
numerical exports are used for generation. Evaluation still requires original
spectral data supplied explicitly by the caller.

An initial deeply nested source build failed in MSBuild's temporary compiler
test with DirectoryNotFoundException for its tlog directory. The same command
completed in a fresh shorter `validation/g709_*` path. The failed attempt is
retained; no persistent system setting or numerical code was changed.

## Measured results

- Normal main IQM Release build succeeds with the new loader/CLI/API.
- Three709 display configurations of one synthetic PF pair pass **608,256
  Float32 export words +24 quality values bitwise exact** against official
  HDR-VDP-3 v3.0.7/Octave11.3. Original709, raw conversion, clamped native RGB,
  P/adaptation, five band errors, accumulated error,Q/Q_JOD are checked.
- Encoding provenance is rgb-bt.709. Vulkan validation enabled, no errors
  reported. Same96×96/30ppd/age24/defaultsp0 profile and native padding/DC
  rules as the preceding API acceptance; cohorts overlap.
- Twelve malformed overlay/nested packages rejected by CPU-only tests.
- Seven CLI cases reject swapped709/native packages, encoded sRGB, nondefault
  ppd/age, wrong shape and missing CIE input, with expected errors/no exports.
- Native RGB HDR-derived PF regression:221,184 export words+8 quality values
  bitwise exact. Luminance v4 regression:seven exports byte-identical to its
  frozen cohort. Neither old path was broadened by fallback.

First `cli/` stopped after successful numeric checks because the harness expected
LF in Windows CRLF provenance text. `cli_v2/` normalizes line endings only and
passes. First rejection harness reused its no-CIE input directory as its output
name; `rejections_v2/` separates those paths and checks the intended missing-CIE
error. Both attempts are preserved; no numeric tolerance was added.

## Ready local command

From the IQM repository in PowerShell, using frozen binary and actual validated
PF files. The C:/HDR-VDP-3_Ref/data installation must be present.

```powershell
$r = Get-Content validation/rgb709_cli_completed_path.txt
$s = Get-Content validation/rgb709_source_completed_path.txt
& "$r/IQM-used.exe" --method HDRVDP3 --enable-validation --input "$r/cli_v2/led-lcd-srgb/test.pfm" --ref "$r/cli_v2/led-lcd-srgb/reference.pfm" --hdrvdp3-encoding rgb-bt.709 --hdrvdp3-display led-lcd-srgb --hdrvdp3-spectral-data C:/HDR-VDP-3_Ref/data --hdrvdp3-runtime "$s/runtime" --hdrvdp3-export "validation/run709_$([DateTime]::UtcNow.Ticks)"
```

Expected Q8.5618896484375/Q_JOD9.17173957824707 for that pair/spectrum.
For a new source package use a fresh short output directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_rgb709_runtime.ps1 -OutputDirectory validation/my709 -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Next broaden709 content (identity/primary/HDR-derived RGB pairs) before adding
other encodings/viewing configurations. This is not universal metric parity,
all-image-dimension support, sRGB EOTF support or fully GPU-resident execution.
