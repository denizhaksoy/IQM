# Raw spectral loading and viewing/age continuation

Updated 2026-10-20. The C++ spectral component can now read the original
single-channel D65, three-channel log-cone and rod tables directly. It
implements nonuniform pchip interpolation, out-of-range zero fill, the exact
native420-point wavelength grid, zero-log sensitivity replacement and cone
exponentiation. Source data remain external inputs; no native data files are
modified or redistributed by this change.

`load_d65_spectral_samples` accepts the three original paths. It reproduces
2,520 double values (wavelength, emission and four sensitivities at420 nodes)
with zero numeric differences. Its outputs then reproduce all16,822 double
age-calibration trace values across13 ages. The independent scalar project
passes both `spectral_age_double` and `spectral_loading_double` CTests.
The scope is the pinned D65/cone/rod files, not arbitrary spectral formats
or color input. Numeric equality does not separately assert signed-zero bits.

Evidence: `validation/raw_spectral_completed_path.txt` selects
`validation/raw_spectral_639280955944105771`. It records hashes, fixture
comparison, CTest logs and subsequent full pipeline records. The test harness
adds `IQM_SPECTRAL_DATA` / `-SpectralDataDirectory`; it hashes all three raw
inputs and exports `spectral_source.txt`. Raw and pre-resampled input modes
are mutually exclusive. Nondefault viewing parameters require the verified
FFT/Vulkan map path. Historical pre-resampled checkpoints remain separate.

## A real power rounding failure found and corrected

The first64x64/60ppd/age60 HDR-ramp stress run failed one final D pixel by
one binary32 ULP. Raw D and all earlier stages matched. The isolated input
was abs(D)=`0x30473d44`, exponent=`0x3f7e96d5`: old output `0x305fbd82`,
native output `0x305fbd83`. Maximum absolute difference was
5.5511151231257827e-17. The failed directory and old binaries are preserved;
it is not a tolerance pass.

Increasing the log/exp polynomial degree fixed mathematical truncation but
broke four existing random native comparisons, so that candidate was rejected.
The accepted correction retains the original native-style cubic polynomials.
For ambiguous normal-output rounding it uses a three-float expansion and
additional constant residuals. The condition depends on rounding uncertainty,
not a pixel index, input lookup or case-specific constant. This remains a
tested native compatibility path, not a universal correctly-rounded pow claim.

Random65,536 + boundary65,536 + counterexample-neighborhood65,536 pairs all
pass (196,608 distinct input pairs; two repeated outputs per pair). The old
shader fails precisely the center of the neighborhood. The accepted map
shader replays all ten earlier full-age exports with zero GPU differences:
1,254,496 floats and52,352 binary64 mask words. This replay is not ten new
end-to-end application runs.

## Four fresh combined viewing/age pairs

With the corrected shader, HDR ramp/stronger localized distortion passes at
both32/64 and ages60/60.001, all at60ppd. Official Q/Q_JOD, CPU JND traces,
strict preprocessing, raw/centered SPYR and GPU intermediates match exactly.
These four fresh pairs compare570,202 floats and23,808 binary64 mask words,
with zero differences. They use the earlier pre-resampled spectral input
path; raw-loader application regressions are recorded separately.

| Size | Age | Q | Q_JOD |
| ---: | ---: | ---: | ---: |
|32|60|9.3602609634399414|9.70660400390625|
|64|60|9.9757671356201172|9.9955730438232422|
|32|60.001|9.3602581024169922|9.7066020965576172|
|64|60.001|9.9757680892944336|9.9955730438232422|

Evidence: `validation/age_ppd60_639280954526250977/COMPLETED.json`, selected
by `validation/age_ppd60_completed_path.txt`. Accepted/rejected shader sources,
counterexample/neighborhood logs, original failed directory pointer and
ten-case replay logs are retained. Map ABI remains13; its payload is unchanged.
The corrected shader SHA256 is
`0B023987B9A2C9267B8979A072574DCC04AD8F56EA56CB1B7AACF164EA041BFF`.
SPIR-V validation passes with Shader capability only, no Float64/Int64.
Earlier ABI13 and ABI12 checkpoints retain their original hashes and scope.

## Full pipeline with raw inputs

Two additional fresh application runs use raw spectral tables directly,
at32/64,60ppd,age60.001,HDR ramp/stronger localized distortion. Both match
official Q/Q_JOD and all strict native stages. Independent GPU comparisons:
285,101 floats and11,904 binary64 mask values,zero differences. They preserve
218 common text-export SHA256 hashes against the corresponding corrected
resampled-input runs (103 at32,115 at64). Q/Q_JOD are respectively
9.3602581024169922/9.7066020965576172 and
9.9757680892944336/9.9955730438232422. These two loader regressions repeat
the corresponding physical input cases;do not count them as new unique cases.

`validation/finalize_raw_spectral_checkpoint.ps1` verifies raw source metadata,
native scores/raw andcentered bands/GPU counts,input/binary hashes and common
text exports before finalizing. The raw root preserves COMPONENT_COMPLETED.json,
the new full records,old/new application binaries and all build logs.

## Reproduce the independent raw loader check

```powershell
cmake -S tests/hdrvdp3_scalar -B '<fresh build>' -DIQM_HDRVDP3_SPECTRAL_FIXTURE='<native fixture>' -DIQM_HDRVDP3_RAW_SPECTRAL_DATA='C:/HDR-VDP-3_Ref/data'
cmake --build '<fresh build>' --config Release --target spectral_age_test
ctest --test-dir '<fresh build>' -C Release -R '^spectral_' --output-on-failure
```

Public API integration, real HDR/color input, general full-pipeline dimensions,
other devices and broader observer parameters remain unfinished. Mixed GPU
integration and the precision-fallback shaders remain local and require their
pinned build/artifacts; independent raw loading sources can be built separately.
