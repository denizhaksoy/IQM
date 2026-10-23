# Explicit HDR-VDP-3 luminance CLI

The normal application argument parser and HDR-VDP-3 entry accept an explicit
test/reference PFM pair and runtime directory. This path reads no legacy quality
environment variables. The existing synthetic and environment-based file paths
remain available when no explicit luminance CLI request is present.

```powershell
cmake -S tests/hdrvdp3_evaluation -B build-hdrvdp3 -DIQM_HDRVDP3_BUILD_APPLICATION=ON
cmake --build build-hdrvdp3 --config Release --target IQM-hdrvdp3 IQM-hdrvdp3-args-test
& ./build-hdrvdp3/Release/IQM-hdrvdp3.exe --method HDRVDP3 --input test.pfm --ref reference.pfm --hdrvdp3-runtime runtime --enable-validation
```

PFMs contain positive finite Float32 absolute luminance in cd/m^2. The test file
is `--input`; the reference file is `--ref`. Their dimensions must match. Select
the bounded32/64, square128 or rectangular runtime package described in
[the application entry](APPLICATION_LUMINANCE_VALIDATION.md).

| Option | Behavior |
| --- | --- |
| `--hdrvdp3-runtime DIR` | Required matching pinned FFT/map/base/math package |
| `--hdrvdp3-ppd NUMBER` | Pixels per degree, default30; validated30/60 |
| `--hdrvdp3-age NUMBER` | Observer age, default24; validated24/60/60.001 |
| `--hdrvdp3-spectral-data DIR` | Explicit D65 spectral tables; required at nondefault age |
| `--hdrvdp3-export DIR` | Optional fresh output directory; default creates a new directory under validation |

Square128 and64x128/128x64 require30ppd/age24. Square32/64 retain their earlier
viewing scope. At age24, omitted spectral data uses the established D65 constants.
CLI values take their own defaults and do not inherit environmental settings.
The global `--output` image option is rejected for luminance quality; diagnostic
output uses `--hdrvdp3-export`. Existing directories are never overwritten.

Argument parsing rejects missing pairs/runtime, malformed/nonfinite/unsupported
numeric values, duplicate or unknown HDR-VDP-3 flags and HDR flags on another
method. These checks run before Vulkan setup. `--help` documents the options.
The parser test covers five valid configurations and22 invalid configurations,
including legacy synthetic mode and unchanged PSNR option parsing.

The numerical checkpoint is `validation/quality_cli_639283326163455781`:
five supported shapes preserve524 numerical export hashes against previously
strict native-validated baselines, with legacy environment values deliberately
poisoned. `validate_application_quality.ps1 -CLI -RepresentativeOnly` reproduces
this scope with a fresh output directory. A separate32 environment-path call
preserves96 hashes. No numerical engine/shader changes were made.
The minimal CLI with no optional flags also passes95 numeric hashes;only the
`spectral_source.txt` provenance tag differs (`legacy_age24` versus `raw_d65`).
It creates a fresh automatic directory and ignores poisoned spectral/viewing
environment settings. Four executable parser rejection checks and two API
rejections also pass,including preservation of an existing output sentinel.

A64square CLI call at60ppd/age60.001 passes all six exact native validators;
Q=9.7612190246582031,Q_JOD=9.9169960021972656. Its accepted logs use
`view60_age60p001_default_plans_*`. An initial attempt incorrectly used the
rectangular checker (which resets FFTW single wisdom) on this square case and
failed preprocessing; that log and computed reference maps are preserved.
The established square validation protocol passes exactly without changing the
GPU outputs or equality requirements. Native FFT planning conditions remain
part of validation scope.

All five representative CLI cases finished successfully, but the runner's final
PowerShell summary failed because its records were dictionaries. The runner now
uses objects; the accepted summary was finalized from the complete five-case
JSON after checking524 hashes. No numerical run was relabeled or relaxed.

The checkpoint adds application argument/configuration coverage. Historical
strict file-pair baselines remain separate from the fresh nondefault-view native
check. RGB, arbitrary dimensions and other GPU devices remain outside scope.
