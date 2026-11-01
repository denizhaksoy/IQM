# Double sRGB steerable pyramid validation

Validated2026-10-28 against official HDR-VDP-3 v3.0.7,Octave11.3.
Completed root: `validation/srgb_pyr_completed_path.txt`.

Added CPU `create_sp0_model_double` and `build_sp0_pyramid_double` for the
default96x96/30ppd/sp0 profile. The model uses original decimal coefficients
as Double literals, without rounding through the older Float32 model. All380
coefficients match the native model bits. Input and output API arrays are
row-major; native column-major captures are explicitly converted.

The implementation reproduces reflect1 correlation, three downsample stages,
the five native band sizes96x96/96x96/48x48/24x24/12x12, normalization by
1/2^(band-1), and base-band DC subtraction. The observed native backend is
`matlabPyrTools/corrDn.m`, which calls `rconv2`/`conv2`, not a MEX correlation.
Reverse column/row tap traversal uses FMA in16-row vector blocks and separate
multiply/add in scalar tail rows. Base-band mean uses compensated summation,
matching Octave11.3 `mean` with `sum(...,"extra")`. This is the measured
reference configuration, not a claim about every Octave/compiler backend.

Fresh native captures save actual normalized bands, pre-DC base band and its
mean alongside the earlier optical/JND traces. Native stock/instrumented
Q/Q_JOD bits agree for two original encoded Single pairs (stress and ramps)
under led-lcd-srgb/led-lcd-wcg/oled. Fixed96x96/30ppd/age24/defaultsp0;
Double FFTW3threads3/estimate/cleared wisdom; optical/adaptation padding and
DC processing remain native. Captures and failed candidates are preserved.

The initial separate-only correlation produced120,450 bit mismatches. Reverse
FMA everywhere reduced this to1,945. Mixed FMA/tail rows left725, attributable
to base-band mean/DC. Mixed arithmetic plus compensated mean passed exactly.
No failed/tolerance run is counted as exact acceptance.

Final isolated stage tests:129,986 Double words each,259,972 across both
pairs, including380 repeated model coefficients per run. Six rejection checks
passed (three cases repeated twice): empty, wrong length and NaN input.
These stage tests use captured P as their explicit boundary input.

Independent CPU diagnostics also passed from original Single sRGB inputs
through the normalized, DC-corrected pyramid. Each pair compares4,976,640
previous optical/JND words plus129,606 pyramid/DC words. Total10,212,492
Double words across two pairs/three spectra, all binary exact including
signed zero. Captures supply comparisons only; no captured intermediate,
filter, matrix, LUT or band supplies the calculation. Isolated stage counts
overlap this scope and should not be added as new image coverage.

This independent diagnostic explicitly uses the local Octave FFTW DLL for
CPU FFT. It is not a production/GPU FFT implementation, and no shaders,
runtime manifests or CLI routing changed. Main IQM Release and strict MSVC
test builds succeeded. Full independent sRGB Q/Q_JOD remains unfinished:
Double masking, CSF/pooling and production Double FFT/runtime integration
are still required before CLI sRGB can be enabled.

```powershell
$r = Get-Content validation/srgb_pyr_completed_path.txt
& "$r/srgb-pyramid-double-test-used.exe" "$r/native_ramps" "validation/srgb_pyr_recheck_$([DateTime]::UtcNow.Ticks)"
```

Next: capture actual per-band adaptation/CSF, masking and difference maps for
the same original pairs in fresh directories; implement and independently
validate Double masking/pooling before final score parity claims.
