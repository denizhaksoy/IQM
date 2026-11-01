# BT2020 and XYZ content validation

Run date: 2026-10-28. Artifact root is recorded in
`validation/linear_content_active_path.txt`. Only a completed pointer and
`final_audit_PASS.txt` establish completion; individual PASS files record
completed subsets while the batch is running.

This batch exercises the existing production API through the frozen main
`IQM-used.exe` from `validation/linear_color_completed_path.txt`. No production
code, shaders or runtime payloads were changed for these tests.

Five original pairs (identity, constant, pure primaries, impulse, real HDR
crop) are each interpreted as `rgb-bt.2020` and `xyz`, under `led-lcd-srgb`,
`led-lcd-wcg` and `oled`: 30 configurations, not 30 independent image pairs.
The real pair uses the previously documented RGBE crop and explicit virtual
display scaling; interpreting its channels under these encodings does not
recover capture colorimetry. Original input buffers are byte-identical to
the prior BT709 content fixtures (60 reference/test buffers).

Official HDR-VDP-3 v3.0.7, Octave 11.3, original Single inputs, 96x96,
30 ppd, age 24 and default sp0 are fixed. Native FFTW uses three threads,
estimate planning and cleared Single wisdom. Native padding, layout and DC
processing are preserved. Each fresh encoding-specific native export verifies
stock/instrumented Q and Q_JOD bits; scores are not reused across encodings.
The native generator retains historical `linear709` wording in its PASS
text; `metadata.json.encoding` records the actual encoding and is checked
by the final audit. Existing native artifacts are preserved as generated.
Column-major native buffers are reordered to planar row-major before binary
comparison. CLI input is three-channel bottom-up little-endian PF.

Each completed pair/encoding compares 608,256 Float32 export words and
24 quality scalars across three spectra: original inputs, raw conversion,
clamped native RGB, perceptual response, adaptation, five band errors,
Q_err, Q and Q_JOD. The full batch target is 6,082,560 export words and
240 quality scalars, with Vulkan validation enabled. Comparisons are binary,
including signed zero; no tolerance is used. Actual counts, scores and
executable hash are recorded in `results.json` by:

```powershell
$r = Get-Content validation/linear_content_active_path.txt
node tests/hdrvdp3_evaluation/audit_linear_content.cjs $r
```

## Separate sRGB precision investigation

`tests/probe_srgb_precision.m` runs official `srgb-display` on a separate
96x96 encoded Single pair, including values around the EOTF threshold.
For all three spectra the display model output, color transformation output,
visual pathway input and final scores are Double. Stock and instrumented
native score bits match. Rounding the visual inputs through Float32 changes
55,296 of 55,296 Double values per spectrum. This is a precision boundary
measurement, not a production sRGB implementation or GPU acceptance.
The probe stores metadata and Float64 EOTF/pathway inputs in `srgb_probe/`.

Full `srgb-display` requires preserving its native Double processing path;
the current Float32 linear path cannot be declared exact by adding an EOTF.
Broader dimensions, viewing settings, prediction modes, device coverage and
remaining CPU processing also remain outside this acceptance scope.
