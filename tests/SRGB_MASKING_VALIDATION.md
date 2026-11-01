# Double masking and quality stage validation

Validated2026-10-28. Completed root:
`validation/srgb_mask_completed_path.txt`. Final accepted runs are
`api_stress_final` and `api_ramps_final`; earlier diagnostics are preserved.

Added CPU Double APIs for default quality/sp0 masking: mutual masking,
neighbor masks, excitation/noise normalization, psychometric response,
comparison-masked differences, Minkowski band error and Q/Q_JOD mapping.
Parameters are independently generated from official default quality literals.
Inputs and outputs are row-major; native column-major exports are reordered.
Old Float32 implementations, shaders, runtime manifests and CLI routing
remain unchanged. Main IQM Release and strict MSVC builds succeeded.

Mutual masking uses the actual defaults `do_si_gauss=false`,3x3 box averaging,
zero border and masking_norm=0. It computes min(abs(test),abs(reference)).
The native conv2 `same` operation uses increasing horizontal and decreasing
vertical taps, with FMA in source-row16-block regions and separate scalar
tail operations. This differs from the earlier pyramid's conv2 `valid` path.
Neighbor masks independently resize these self masks with Double bicubic
antialiasing/symmetric extension and clamp negative interpolation to zero,
following native neighboring-band exclusions exactly.

Fresh official3.0.7 captures include log-adaptation/CSF/noise, self/neighbor
masks, masking noise, band differences, excitation, normalized/psychometric
differences, comparison masks, masked differences, parameters, band errors,
accumulated error and final scores. Stock and instrumented native Q/Q_JOD
bits agree for stress and ramp pairs under all three spectra. Fixed original
Single encoded96x96 inputs,30ppd,age24,defaultsp0;Double pathway,FFTW3threads3,
estimate/cleared Double wisdom;native padding, band normalization and DC.

Each final test compares450,705 Double words with zero bit differences;
total901,410 across two pairs/three spectra. This includes self and neighbor
masks independently calculated from native pyramid-band boundary inputs,
five numerical maps per band, parameters, band/cumulative errors and scores.
Band differences are independently subtracted from the two bands. The test
still supplies native neural-noise and resized comparison maps as explicit
stage inputs. **These score matches are not independent original-image to
Q/Q_JOD acceptance.** Earlier original-input-through-pyramid results remain
separate and must not be added as new full-metric coverage.

Eight invalid-input cases passed per run (16 repeated checks total), covering
empty/negative errors, dimensions, negative self mask, zero neural noise,
NaN comparison, mutual-mask size and missing neighbor bands. A separate
positive-infinite neural-noise limit check produces zero error and Q=JOD=10.
Positive infinity is therefore allowed for neural noise, matching the native
zero-frequency sensitivity limit; NaN/nonpositive noise is rejected.

The first optional-Gaussian attempt mismatched all64,368 mutual-mask values
because the official default disables that branch. The corrected box model
initially had33,111 bit differences from conv2 accumulation order; isolated
tap/FMA candidates and matching source-row scalar tails eliminated them.
Failed exports are retained and are not exact passes. Final diagnostic-only
candidate loops were removed from the accepted test source.

```powershell
$r = Get-Content validation/srgb_mask_completed_path.txt
& "$r/srgb-quality-double-test-used.exe" "$r/native_ramps" "validation/srgb_mask_recheck_$([DateTime]::UtcNow.Ticks)"
```

Next independently generate Double neural CSF noise and comparison masks:
mean/log adaptation, per-band resize/interpolation, base-band FFT dominant
frequency selection and native-relative RGB comparison. Connect these to the
already verified original-input CPU pyramid and new masking/pooling stages
before claiming independent full sRGB scores. Production Double FFT/runtime
and GPU validation remain additional work; CLI sRGB stays unsupported.
