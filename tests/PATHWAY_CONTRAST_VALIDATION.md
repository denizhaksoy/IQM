# Small-contrast,uniform-gain andconstant-luminance validation

Five explicit controls in both80x96/96x80 orientations produce ten pairs:

|Control|Reference|Transform|Gain|Offset|
|---|---|---|---:|---:|
|asymmetric_tiny|asymmetric10cd/m2 pattern|localized ROI|1.0001|0|
|hdr_small|HDR ramp1e-2..1e3|localized ROI|1.001|0|
|asymmetric_uniform|asymmetric10cd/m2 pattern|whole image|1.01|0|
|constant_small|constant10cd/m2|whole image|1.01|0|
|constant_visible|constant10cd/m2|whole image|1.4|0|

`generate_general_pathway_reference.m` now accepts explicit--transform,
--gain,--offset,--level andconstant pattern. Localized default1.4gain+.25offset
remains unchanged:206 regenerated default native text exports have identical
hashes. Official3.0.7 runs use Float32 luminance,30ppd,age24,D65,three FFTW
threads,estimate planning andfresh single wisdom per stock/traced evaluation.
Stock/traced Q/Q_JOD must agree for every pair;the installed reference is read-
only. All native/gpu exports use fresh directories,old references retained.

Positive infinity export is permitted only at the measured native zero-frequency
positive-infinite-noise limit,not merely based on anidentity flag. Other NaN/Inf
fields remain rejected. These ten nonidentity controls have finite base noise;
zero-frequency/+Inf acceptance remains the separate identity test scope.

`export_native_band_errors.m` applies the exact official local Minkowski
expression to saved native D_p. Every reconstructed cumulative error must equal
the captured official accumulated error before exporting scalar band errors.
Psychometric goldens use the native expression on saved native D andpf.
These derived goldens are validation only,never runtime inputs.

`IQM-pathway-contrast-test` calls the in-memory API from original images with
explicit source-generated runtime assets. It reads expected image-dependent
values only after the call. Each pair compares69stages: P/log,normalized bands,
raw/pooled/comparison masks,neighbor/denominator/difference/CSF,D/masked/
psychometric maps,band/total errors,scores,base selection/frequency/noise and
base log/adaptation. Float32 andFloat64 words must match bitwise;numeric/bit
errors andfirst mismatch are preserved,with actual andexpected binary dumps.

All ten pairs pass690stage checks/2377330values bitwise,no Vulkan errors.
Some uniform/constant controls have native scores rounding to10;this is observed
reference behavior,not anassumed monotonicity or a claim of zero intermediate
signal. The intermediate/error comparisons protect against a score-only pass.

```powershell
cmake --build <bounded-build> --config Release --target IQM-pathway-contrast-test -j4
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/validate_pathway_contrast.ps1 -Executable <bounded-build>/Release/IQM-pathway-contrast-test.exe -Runtime <source-runtime>/runtime -NativeDirectory <native-cases-root> -OutputDirectory <fresh-results>
```

The native root needs cases.json (the table above) andper-case outputs from
the native generator followed byexport_native_band_errors.m. See retained
checkpoint validation/pathway_contrast_639285467553162992 for exact arguments,
source/binary/native hashes andresults. Device:Intel Arc Pro Graphics. Results
extend content/transform coverage,not dimensions,viewing conditions,RGB/real
HDR data ordevices. No production numerical changes were required.
