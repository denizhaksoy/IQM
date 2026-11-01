# Linear BT.2020 / XYZ production paths

2026-10-28 checkpoint: `validation/linear_color_active_path.txt`.

`LinearColorEncoding` selects BT709,BT2020,XYZ. Production
`linear_color_transform` shares the accepted pivot/reciprocal/FMA solve while
preserving `rgb709_transform` as a compatibility wrapper. Each encoding passes
8,901 Double comparisons against official3.0.7/Octave11.3: shared resampled
spectral data/integrals/normalization plus its own27 transform coefficients.
These counts include repeated common data, not independent new sample sets.
RGB spectral regression9 configurations/10,296 Double values still passes.

`evaluate_pathway_linear_color_quality` accepts original finite nonnegative
Float32 planes, explicit encoding, consistent XYZ/LMSR display samples and
conversion/downstream shaders. It applies the independently calculated matrix,
native1e-6 clamp and existing native quality processing. The709 entry remains
a wrapper with the same geometry/viewing contract and output.

## API acceptance

BT2020 andXYZ each pass **442,392 Float32 words bitwise exact**, including raw
conversion, clamped native RGB,P/adaptation,Q/Q_JOD and all band/accumulated
errors. Updated709 compatibility test passes442,374 words (the historical
checker scope omits the18 extra band-error scalars). No native matrix or
intermediate is injected. Test orchestration and production API use the same
accepted generic3×3 projection SPIR-V; shaders/runtime numerical pins unchanged.

Scope is one synthetic96×96 pair interpreted separately in each encoding,
three display spectra,30ppd,age24,default quality/sp0. Native FFTW3 threads3,
estimate/cleared single wisdom, symmetric MTF/replicate adaptation/DC unchanged.
Both native new paths retain Single projection/P; no sRGB display EOTF is added.

## Runtime and application

`linear.runtime`: `IQM_HDRVDP3_LINEAR 1 <encoding>-30ppd-age24-sp0-square96`.
It binds native RGB manifest and converter with SHA256/canonical confinement,
then checks all nested native/base payloads. Loading requires the expected
encoding, so BT2020 andXYZ packets cannot substitute for each other. The709
format/loader is retained with its own header. Each final package contains
the same23 pinned shader/model payloads; the profile manifest differs.

Main IQM builds with explicit `--hdrvdp3-encoding rgb-bt.2020|xyz`, matching
runtime, display and spectral-data arguments. Same nonnegative color PF input
contract and fixed96×96/30ppd/age24 scope. `generate_linear_color_runtimes.ps1`
regenerates the common23 payloads and both profile packages from repository
sources. Its source root is recorded in `validation/linear_color_source_active_path.txt`.
Both regenerated packages match accepted manifests/payloads and load correctly.
Raw CIE/cone/rod/emission data remain explicit evaluation inputs, not bundled.

Two valid profiles and12 malformed variants per profile are checked without
GPU; cross-profile load attempts reject. Nine actual CLI negative combinations
reject swapped native/709/2020/XYZ packages, unsupported viewing and encoded
sRGB, with expected errors and no exports. An introduced variable-shadow warning
in the common loader was corrected before final application testing.

Both main-CLI encodings pass **1,216,512 Float32 export words +48 quality
scalars bitwise exact** in total (608,256+24 per encoding). Original inputs,
raw/clamped conversion,P/adaptation,all five band errors,total error,Q/Q_JOD
are checked. Six spectral/encoding configurations share one content pair;
API/CLI cohorts overlap. Vulkan validation enabled, no errors reported.
The old709 command retains all16 output files byte-identically; native RGB
HDR-derived regression passes221,184 export words+8 quality scalars. All
previous v4 luminance exports also match their frozen files byte for byte.

## Reproduction

Native oracle accepts `generate_rgb709_reference.m <fresh> --encoding
rgb-bt.2020|xyz`; original709 default/external-input options remain. The named709
test targets are legacy filenames: `xyz-matrix-test` takes optional encoding,
and `rgb709-quality-api-test` takes encoding after its usual five arguments.
`validate_rgb709_cli.cjs` similarly takes optional final encoding.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_linear_color_runtimes.ps1 -OutputDirectory validation/linear_packages -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use `validation/linear_packages/rgb-bt.2020` or `/xyz` with its matching encoding;
do not pass the intermediate709 source package. Native source generation uses
no reference installation or numerical exports. Main application execution
requires original spectral tables including ciexyz31.csv.

Ready local invocation (from repository PowerShell, explicit original data):

```powershell
$r = Get-Content validation/linear_color_completed_path.txt
$s = Get-Content validation/linear_color_source_completed_path.txt
& "$r/IQM-used.exe" --method HDRVDP3 --input "$r/cli_rgb-bt.2020/oled/test.pfm" --ref "$r/cli_rgb-bt.2020/oled/reference.pfm" --hdrvdp3-encoding rgb-bt.2020 --hdrvdp3-display oled --hdrvdp3-spectral-data C:/HDR-VDP-3_Ref/data --hdrvdp3-runtime "$s/rgb-bt.2020" --hdrvdp3-export "validation/linear_run_$([DateTime]::UtcNow.Ticks)"
```

For that pair/spectrum expected Q8.733026504516602/Q_JOD9.295844078063965.
For XYZ use encoding`xyz`, package`$s/xyz` and its own validated input files.
Binary export names retain the historical `_rgb` suffix for the three input
planes; `profile.txt` records the actual encoding and no implicit conversion
to that encoding is performed.

Next expand BT2020/XYZ contents before broader claims. sRGB-display Double
precision, more dimensions/viewing conditions/prediction modes/device coverage
and fully GPU-resident processing remain open. This is not whole-metric parity.

The automatic approval reviewer initially could not run the final test builds
because its usage quota was exhausted. The same approved build route succeeded
on retry; no approval check was bypassed. All recorded final acceptances were
run after successful builds. A new loader variable-shadow warning was fixed
before the final application binary was built and tested.
