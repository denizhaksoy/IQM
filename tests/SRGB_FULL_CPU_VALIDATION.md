# Independent sRGB CPU score validation

Validated2026-10-28 against official HDR-VDP-3 v3.0.7/Octave11.3.
Completed root: `validation/srgb_csf_completed_path.txt`.

The independent CPU diagnostic now computes original encoded Single images
through Double Q/Q_JOD without using captured intermediates, native filters,
matrices, LUTs, base frequency or comparison masks as calculation inputs.
Raw spectral data remains explicit model input. Native captures supply only
expected comparisons. This closes the earlier masking-stage noise/comparison
dependencies within the tested CPU profile.

New source APIs generate mean log-adaptation, resized comparison maps and
per-band CSF/neural noise. Existing source-generated CSF LUTs and native-grid
interpolation match all tested entries. Comparison uses any relative RGB
channel difference greater than0.001 before native Double bicubic resize.
The actual12x12 Double base-band FFT ignores DC, selects the first maximum
in column-major order, generates its frequency and uses compensated mean
log-adaptation for base luminance/nCSF. No captured index/frequency is supplied
to the full diagnostic. All288 base spectrum components are compared.

Two distorted pairs (ramps and deterministic stress codes) plus an identity
control are tested under led-lcd-srgb/led-lcd-wcg/oled: **nine configurations**.
The identity control reuses the stress reference; it is not independent
photographic content. Fixed96x96,30ppd,age24,defaultsp0;original Single input,
Double pathway;FFTW3threads3/estimate/cleared wisdom/unaligned plans;native
symmetric optical/replicate adaptation padding, band normalization and DC.

All nine Q/Q_JOD predictions match native bits. Intermediate comparisons
also match binary representations, including signed zero. Per pair/control:
5,106,246 earlier frontend words plus712,890 full-quality words; total
**17,457,408 Double words** across the three runs. Scores are included in
these counts. Isolated stage tests also pass712,020 words each/2,136,060
total and36 repeated invalid-input checks (12 distinct cases), with positive
infinite-noise limits. These counts overlap the independent scope.

| Pair | Spectrum | Q | Q_JOD |
|---|---|---:|---:|
| Stress | led-lcd-srgb | 5.184004726299424 | 6.1036253107800604 |
| Stress | led-lcd-wcg | 5.1840068853923409 | 6.1036275487945639 |
| Stress | oled | 5.1837547121689243 | 6.1033661559446406 |
| Ramps | led-lcd-srgb | 9.8454421901640536 | 9.9524591792569996 |
| Ramps | led-lcd-wcg | 9.845440160992446 | 9.9524583795843746 |
| Ramps | oled | 9.8450571153464637 | 9.9523073729608242 |
| Identity | all three | 10 | 10 |

The first identity export stored a wholly real spectrum without its imaginary
plane. The capture format now always writes complex FFT fields as interleaved
real/imaginary words, even for zero spectra; a fresh export is `native_identity_v2`.
The subsequent comparison exposed60 conjugate imaginary -0 values where
Octave had simplified the wholly real spectrum to real storage and exported
+0. The diagnostic now reproduces that real-storage simplification. No
signed-zero difference is ignored or accepted with tolerance. Failed exports
and runs are preserved; final accepted runs are `accepted_stress`,
`accepted_ramps` and `accepted_identity`.

The diagnostic explicitly loads local Octave Double FFTW for CPU transforms.
It is not linked into the production metric. Normal IQM Release and strict
MSVC tests build successfully, but production Double FFT/runtime integration
and GPU equivalence remain unfinished. CLI `srgb-display` remains unsupported.
Nine tested configurations do not establish arbitrary dimensions, viewing
conditions, other prediction tasks, all images or cross-device exactness.

```powershell
$r = Get-Content validation/srgb_csf_completed_path.txt
$p = Get-Content validation/srgb_double_completed_path.txt
& "$r/srgb-fft-double-inspect-used.exe" "$r/native" "validation/srgb_full_recheck_$([DateTime]::UtcNow.Ticks)" unaligned C:/HDR-VDP-3_Ref/data "$p/native_stress"
node tests/hdrvdp3_evaluation/audit_srgb_full.cjs $r
```

Next: implement a production Double FFT/runtime path, preserve the verified
CPU numerical behavior during GPU transfer, then enable strict sRGB CLI
routing and perform public-CLI regression/negative tests. Retain this frozen
independent CPU result as the comparison checkpoint.
