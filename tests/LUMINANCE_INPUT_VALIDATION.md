# File-based luminance quality validation

Updated2026-10-20. The local HDR-VDP-3 wrapper accepts a pair of positive
absolute-luminance PFM files and bypasses its synthetic generator/transforms.
The input pixels are neither resized nor tone mapped. The existing8-bit
RGBA reader used by other metrics is unchanged. The full metric path still
requires equal square32x32 or64x64 inputs; the independent reader is also
tested on a nonsquare3x5 image.

## Input contract and independent checks

The [standalone input project](hdrvdp3_input/README.md) verifies18 valid fixtures
covering little/big endian,three scales andthree shapes:30,810 binary32 values
match bitwise. Seven malformed/out-of-domain files are rejected. Six prepared
photo-content PFM files add15,360 bitwise values with zero differences. A
separate Octave PFM decoder verifies30,720 input/export values bitwise across
six full image pairs. These are transport checks, not additional distinct
image-case coverage.

The application rejects mismatched file dimensions rather than resizing.
Its legacy test wrapper can report errors while returning exit0, so the
validation runner additionally rejects its explicit error logs. The negative
dimension test and its expected failure directory remain preserved.

Runtime environment variables: `IQM_LUMINANCE_REFERENCE`, `IQM_LUMINANCE_TEST`.
Both must be supplied. Runner options: `-LuminanceReference`, `-LuminanceTest`,
`-InputSize 32/64`. It hashes both files, separates `_file` suite prefixes and
clears stale file-input environment variables for synthetic runs. Each export
records `input_source.txt=pfm_absolute_luminance` or `synthetic`.

## Real-content scope

The external official example `examples/nancy_church.hdr` is768x1024.
The validation preparer uses stb HDR float decoding, then explicit binary32
linear Rec.709 numeric Y conversion. It assumes1 cd/m² per derived Y unit;
source primaries and absolute photometric calibration are not independently
verified. The metric and Octave both consume the same resulting single-channel
luminance. Native `rgb-native` color processing is not covered.

Crop coordinates are zero-based, with no resizing:

| Size | x | y | Minimum cd/m² under declared assumption | Maximum |
| ---: | ---: | ---: | ---: | ---: |
|32|368|496|0.8130117654800415|3.6711874008178711|
|64|352|480|0.7602265477180481|35.260353088378906|

Reference crops,localized1.5 gain on an8x8 patch,global1.05 gain andidentical
pairs are tested. Raw spectral table loading,corrected ABI13 shader,
30ppd,age24 andD65 calibration are used consistently.

| Size | Distortion | Q | Q_JOD |
| ---: | --- | ---: | ---: |
|32|localized|8.0709753036499023|8.7933588027954102|
|64|localized|9.2685623168945312|9.6516733169555664|
|32|brighter|9.9970340728759766|9.999699592590332|
|64|brighter|9.9958648681640625|9.9995403289794922|
|32|identical|10|10|
|64|identical|10|10|

Every score matches the direct official v3.0.7 call exactly. Strict native
preprocessing, CPU JND traces,raw andcentered SPYR,andindependent GPU stages
also match. The six photo pairs compare855,303 floats and35,712 binary64
mask words. Two synthetic regressions preserve earlier scores exactly and
confirm stale file-input flags are cleared. Across all eight fresh runs:
1,140,404 float values and47,616 double mask bit patterns,zero differences.
Float-stage equality is numeric;input transport anddouble masks check bits.

## Artifacts and limitations

`validation/luminance_input_completed_path.txt` selects
`validation/luminance_input_639280968498275868/COMPLETED.json`. It contains
PFM fixtures,source/crop metadata andhashes,loader tests,build logs,all native
andGPU logs,verified executable andthe expected dimension failure. The local
`finalize_luminance_input_checkpoint.ps1` checks original-file bit parity,
strict stages/scores,counts andbinary/input identities before issuing PASS.

`validated_suites.json` is authoritative:paths come from completed per-run
`Suite PASS` lines. A concurrent negative test overwrote a mutable global
latest pointer;the resulting provisional64-identical metadata was preserved
as `suites64_from_mutable_latest.json` andcorrected from its completed log.
No numerical result changed. Do not use global latest pointers as evidence
when multiple runs share a suite prefix.

Padding remains MTF symmetric post andlocal-adaptation replicate post.
PFM input/export layout is top-down row-major;FFT buffers/reductions retain
column-major order. Raw bands are checked before residual DC removal andthe
centered residual is compared separately. Numerical precision is unchanged.

The whole768x1024 source frame,RGB-native metric,zero luminance/general image
formats,other dimensions andnormal public metric API remain unfinished.
The reader andindependent tests are publishable;main pipeline integration
remains local andmixed with pre-existing changes.
