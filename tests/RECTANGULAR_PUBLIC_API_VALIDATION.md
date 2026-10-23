# Rectangular public quality API

Accepted 2026-10-23 checkpoint:
`validation/rectangular_public_639282926432216545`;
completed pointer `validation/rectangular_public_completed_path.txt`.
All eight public pairs retain score bits and 856 numerical export hashes.
Current-source square32/64/128 regressions retain96/107/107 export hashes and
score bits;all35 rejection checks pass. Freshly regenerated runtime assets
retain all six pinned binary hashes.

The first replay process stopped after four completed64x128 pairs,leaving an
incomplete128x64 output. Completed first-run records are in
`public_pairs/results.json`;the four128x64 replays use fresh
`public_pairs_resumed/` exports and their own PASS/results. Root
`public_results.json` combines only these eight accepted cases. The interrupted
partial output remains preserved and is excluded from acceptance.

`evaluate_quality` now supports equal 64x128 or 128x64 reference/test images at
30 pixels per degree and observer age24. Inputs are positive finite Float32
absolute luminance, top-down row-major, with no implicit resize or conversion.
The existing square32/64/128 contracts are retained. Other rectangular sizes
and viewing parameters remain rejected.

The rectangular path requires explicit FFT ABI64256, MapGPU ABI16 and base FFT
ABI85 assets, plus the pinned prepare/finish/multiply math shaders. Generate
them from workspace sources with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_assets_rectangular.ps1 -OutputDirectory <fresh-runtime-dir> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Set `options.fft.referenceShaderPath` to `fft.spv`,
`options.fft.referenceMathDirectory` to `math/`, `options.mapShader` to
`map.spv` and `options.baseFFTShader` to `base.spv` in that directory. Supply
spectral samples or the established D65 spectral data directory as required by
the existing API. A fresh `EvaluationTrace::directory` is required. The caller
must use its Vulkan instance exclusively during this synchronous call.

This exposes the same numerical path previously validated by
[the first two full pairs](RECTANGULAR_MAPS_VALIDATION.md) and
[six stress pairs](RECTANGULAR_STRESS_VALIDATION.md). Only dimension admission
and API comments change in the engine; there are no numerical shader changes.
The diagnostic wrapper and public wrapper now call the same implementation.

Public replays use a fresh non-diagnostic build with both
`IQM_HDRVDP3_DIMENSION_DIAGNOSTIC` and `IQM_HDRVDP3_DIAGNOSTIC_128` set to OFF.
`validate_rectangular_public.ps1` compares returned score bits and every
non-path/non-tag text export against the accepted eight diagnostic pairs.
The earlier native six-stage acceptance remains separate from these public
replays. Full native validators need not be reinterpreted as new runs here.

Public rejection checks cover 18 rectangular dimension/viewing/missing-asset
cases,3 invalid square128 viewing configurations and14 existing invalid cases.
All reject before evaluation dispatch or diagnostic output overwrite.

This bounded support does not cover arbitrary dimensions, odd FFTs, RGB-native
inputs or other GPU devices. CLI/UI input selection and installation of default
runtime assets are separate from this explicit library API.
