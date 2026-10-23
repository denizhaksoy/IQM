# Application luminance-file quality entry

Accepted2026-10-23 checkpoint:
`validation/rectangular_app_639283313608789740`;
completed pointer `validation/rectangular_app_completed_path.txt`.
All11 PFM pairs preserve1,166 numerical export hashes and exact quality files.
Seven invalid application requests reject without quality exports.

Synthetic32/64 asymmetric/localized regressions pass strict preprocessing,
CSF,masking,D,official scores and all13 native JND stages. The independent
ABI13 GPU checker compares57,046/228,055 Float32 values exactly and
2,368/9,536 binary64 values bitwise,respectively. The initial32 driver selected
an obsolete ABI12 checker and failed after its native checks;that failed log
is preserved. The same completed32 export passes the ABI13 checker in
`synthetic32_checker13.log`. The64 suite uses that checker explicitly and has
its own complete PASS marker. No tolerance or algorithm was changed.

The `IQM --method HDRVDP3` application entry now accepts matching positive
Float32 luminance PFM pairs with square32/64/128 or rectangular64x128/128x64
dimensions. File dimensions select the public API path; `IQM_SPYR_SIZE` only
controls the historical synthetic32/64 path. There is no implicit resize,
normalization or RGB conversion.

```powershell
$env:IQM_LUMINANCE_REFERENCE='<absolute-reference.pfm>'
$env:IQM_LUMINANCE_TEST='<absolute-test.pfm>'
$env:IQM_QUALITY_RUNTIME='<matching-runtime-directory>'
$env:IQM_SPECTRAL_DATA='<D65-spectral-directory>'
& ./build-vs2026/bin/Release/IQM.exe --method HDRVDP3 --enable-validation
```

`IQM_QUALITY_RUNTIME` selects an explicit source-generated package. For square32/64
use the bounded package (`generate_assets.ps1`, `fft32.spv`/`fft64.spv`);
for square128 use `generate_assets128.ps1`; for rectangles use
`generate_assets_rectangular.ps1`. Each also provides `map.spv`, `base.spv` and
`math/`. Extended sizes require30ppd/age24. ABI and numerical domain guards remain
in the public API. A package for a different profile is not interchangeable.

The package sets the reference column-first/native FFT configuration and explicit
shader paths, replacing individual legacy backend/path environment settings for
this quality call. If `IQM_QUALITY_RUNTIME` is omitted, the existing individual
settings remain supported. Spectral/viewing settings keep their existing rules.
Both PFM paths are required together.

File requests go directly to the quality entry, avoiding unrelated historical
component tests. Synthetic requests still execute their old component tests and
then the same quality helper. Each successful call prints an absolute
`Quality export directory:` and creates a fresh export directory under validation.

`validate_application_quality.ps1` tests all eight previously accepted rectangular
pairs and three square file pairs. It deliberately sets invalid stale synthetic
size/backend values to establish that explicit file dimensions/runtime are used,
then compares quality and all non-path/non-tag exports by SHA256. The established
native acceptance is inherited through unchanged exports, not counted as new
independent native pairs. Synthetic regressions are checked separately.

This is an explicit luminance-file application entry. Arbitrary dimensions,
RGB images, automatic runtime installation and other GPU devices remain outside
this checkpoint.
