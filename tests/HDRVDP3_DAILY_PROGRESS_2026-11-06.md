# HDR-VDP-3 validation result — 2026-11-06

The frozen experimental matrices/comparison/mutual backend completed the full
declared local sweep: **513/513 selected image pairs across 27 videos passed**.
Every frame's Q and Q_JOD bits matched the preserved accepted official v3.0.7
records, and all 27 native Single video means matched. Completed at
2026-11-06 12:59:34 UTC (13:59 Europe/Prague).

The independent saved-result audit also passed: 513 unique records, matching
application/result bits and verified GPU stage counts for every frame, with no
reported Vulkan validation errors. Each evaluation used four matrix, one
comparison-mask and six mutual-activity GPU dispatches. Remaining processing
still uses CPU; this is not a full GPU implementation.

Artifacts:

- `validation/live_hdr_mutual_all_frames_20261105_run01/checkpoint.json`
- `validation/live_hdr_mutual_all_frames_20261105_run01/accepted_frames.jsonl`
- `validation/live_hdr_mutual_all_frames_20261105_run01/video_means.json`
- `validation/live_hdr_mutual_all_frames_20261105_run01/completion_audit_20261106_run01.json`

All three representative 27-video backend audits had already passed. This
full-frame sweep extends coverage of the new backend, not the number of unique
input pairs. It reuses preserved official predictions; it does not recompute or
replace them. The earlier isolated football150 reference discrepancy remains
unresolved and preserved. Original paper cohort/settings remain unverified.

Supported statement: the documented Single quality profile matched accepted
official results on all 513 tested pairs and 27 video means in this local cohort.
Universal bitwise parity, original-paper reproduction and complete GPU coverage
have not been established. The current test chain is finished; no numerical
test process remained running at the status check.

## Subsequent validation extension

Fresh additional tests passed14/14: portrait/landscape, odd/small dimensions,
PPD26–256, PQ-domain resize in both directions, identical inputs and deeper
7/8-band pipelines at256x256/512x512. CPU and combined GPU application score bits
matched clean official evaluations for every pair. Source snapshots, fixture
hashes and score records were independently audited.

A new separate real-content run is now active on16 selected Daylight1 frames,
outside the completed three-content sweep. See
`HDRVDP3_PROFILE_MATRIX_VALIDATION.md` for scope, artifacts and commands.
The earlier statement that no test process remained running describes the
completion status check before these new tests were launched.

## Daylight completion and next GPU stage

Daylight1 completed16/16 frames and its video mean exactly (Q_JOD bits4107b9c3).
Source-pair hashes confirm529 distinct real pairs in the combined backend's
measured coverage. The14 synthetic setting cases contain12 distinct PNG pairs.

Implemented optional Double comparison-mask resizing on GPU. Standalone26cases/
11,068,767values, quality isolation, independent football150 PNG-to-score, and
all14 resize-enabled dimension/PPD cases passed bitwise checks. All four app
targets compiled. The old default CPU/hybrid paths remain available.

A stage-only comparison across the529 real input masks is now running; it
avoids recomputing unchanged FFT/JND stages and does not produce new full-score
predictions. See `COMPARISON_RESIZE_GPU_VALIDATION.md` for scope and live status.
