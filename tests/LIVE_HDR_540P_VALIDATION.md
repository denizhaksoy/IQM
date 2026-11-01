# LIVE HDR 540p-to-4K Single quality check

2026-11-01. Pair: metadata-mapped `540p_2.2M_Daylight1` versus
`4k_ref_Daylight1`, zero-based frame0. This is one lower-resolution distorted
frame test, not a full lower-resolution video evaluation.

Both streams were probed as HDR10/PQ/BT2020 before explicit RGB48 PNG decoding.
The reference retains3840x2160 geometry; the distorted input is960x540.
`frame.cpp` now normalizes the original uint16 samples, bicubic-resizes each
Single PQ channel to reference dimensions, clamps to[0,1], then applies
inverse PQ, reflected ambient and peak clamping. It does not resize linear
light or reduce reference dimensions. The existing independent bicubic helper
preserves Single image arithmetic and Double coefficient preparation.

Unmodified official v3.0.7, Octave11.3.0 CPU, Single inputs and the same display/
viewing/FFTW settings as the 4K checkpoints gives:

| Output | Official and independent C++ | Single hex |
|---|---|---|
| Q | 4.7725744247436523 | 4098b8ee |
| Q_JOD | 5.6721343994140625 | 40b58220 |

`tests/compare_live_hdr_frame_outputs.cjs` verified exact Q/Q_JOD and checked
input hashes, tensor sizes, precision, backend, viewing/display settings and
provenance. The C++ run starts from the PNG files and does not use the official
preprocessed tensor. This is final-score equality; the resized Single tensors
have not been exhaustively compared in this checkpoint.

Artifacts:

- `validation/live_hdr_540p_frame0_20261101_run01`: PNGs, probes, exact decode
  commands, hashes and FFmpeg version.
- `validation/live_hdr_540p_preprocessing_20261101_run01`: official Single tensors.
- `validation/live_hdr_540p_score_20261101_run01`: official score/log.
- `validation/live_hdr_540p_cpp_20261101_run01`: independent complete score and
  `comparison.json` with `exact:true`.
- `validation/live_hdr_single_demo_20261101_run03`: executable/source snapshot
  with PQ resize and all three PNG cases, including`lowres540`.

Frozen-package repeat passed with `EXACT Q AND Q_JOD` in
`results_lowres540_1793564583103`. Its comparison records the executable,
dependency, model and source-image hashes. This validates the new resize branch;
the two equal-resolution package checks remain preserved in the earlier run02
bundle. Merely including three cases in a bundle is not a claim of three new runs.

Local reproduction from the repository:

```powershell
node validation/live_hdr_single_demo_20261101_run03/run_demo.cjs lowres540
```

The package wrapper verifies hashes and creates a new result directory. Original
decode precision and frame phase remain explicitly local choices, as described
in the protocol audit. Further resolutions, temporal samples and contents remain
to be validated. The 16-frame equal-resolution video batch uses the earlier
frozen run02 executable; adding resize did not alter that running batch.
