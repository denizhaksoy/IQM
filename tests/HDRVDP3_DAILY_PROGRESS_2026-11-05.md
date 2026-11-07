# HDR-VDP-3 progress — 2026-11-05

## Completed today

- Verified that the football triple-reference diagnostic finished with all
  17 selected frames exact across three independent official runs and C++.
  Its independently verified video mean Q_JOD is 8.210325241088867.
- Identified an interrupted continuation rather than assuming stale status meant
  a live process. Preserved the old partial attempt and archived its stale lock
  only after confirming the recorded process was absent.
- Restarted the remaining local cohort evaluation with a recovery wrapper that
  checks historical evidence hashes, refuses live lock owners, and chains
  statistics/evidence/public-backend audits after successful video evaluation.
- Implemented a standalone three-buffer Vulkan comparison mask with explicit
  shaderFloat64 requirement and exact Float32 threshold semantics.
- Validated 999,995 near-threshold synthetic pixels plus identical/extreme tail
  cases and two different 4K native captures. Each real capture's 8,294,400 mask
  values matched the strict C++ expression bitwise. No Vulkan validation errors
  were reported. Repeated synthetic checks are not extra independent coverage.
- Added an explicit optional quality-stage mask provider. In a separate 4K
  isolation test, six band errors, base frequency and Q/Q_JOD bits matched the
  CPU path; Q=40ec7071 and Q_JOD=41038aea also matched the later saved reference
  capture. This diagnostic uses persisted reference stages and is not an
  independent end-to-end parity test.
- Rebuilt both public CPU and hybrid application targets successfully after
  adding the optional provider and library component.

## Running and remaining

Further GPU progress: implemented the mutual-activity stage while preserving
the CPU transpose, FMA order and row-tail behavior. Rejected native prototypes
exposed Intel FMA bit differences and NVIDIA subnormal flushing; their failed
records are preserved. The corrected FP64/explicit Float32-rounding kernel
passed 35 cases / 19,479,447 values, including subnormals, all finite exponent
ranges, extreme values and six real 4K pyramid bands. Quality-stage isolation
and an independent PNG-to-score football150 execution both retained exact
Q/Q_JOD bits. The new experimental profile executed 4 matrix + 1 comparison +
6 mutual GPU dispatches. Its separate 27-video representative regression is
running; this is still a mixed CPU/GPU implementation. See
`MUTUAL_SINGLE_GPU_VALIDATION.md` for artifacts and rejected-prototype details.
After the representative regression passes, an attached continuation will
recompute all 513 sampled frame pairs using the frozen new backend, compare
every Q/Q_JOD bit and verify all Single video means. Its current waiting status
is in `validation/live_hdr_mutual_all_frames_20261105_run01/checkpoint.json`.
The failure-blocking guard passed. This full-frame backend sweep is pending;
it must not be described as completed before its final acceptance appears.

The continuation completed 27/27 local videos and audited 513 unique selected
video-frame pairs. Official/C++ local bright-MOS statistics are identical:
SROCC approximately 0.935287 and raw PLCC approximately 0.901651. Independent
Octave statistics verification passed. Representative public CPU/hybrid checks
are running. Inspect the live status:

`validation/live_hdr_verified_continuation_20261104_run01/resume_20261105_run01/status.json`

The earlier single football150 reference discrepancy is still unresolved.
The original author's exact cohort/protocol omissions still prevent a claim of
identical paper results. CPU stages remain in both public backends. The new GPU
mask now has a separate explicit experimental application target. One independent
PNG-to-score football150 test passed against the later clean reference, with
four matrix plus one comparison-mask dispatch. A separate 27-video representative
frame0 regression is running at
`validation/live_hdr_mask_backend_audit_20261105_run01/checkpoint.json`.
The existing hybrid profile is unchanged. The new profile is still experimental.
Additional GPU stages and broader metric configurations remain outside today's
validated scope. No universal parity or metric-completion claim is made.

See `COMPARISON_SINGLE_GPU_VALIDATION.md` for the implementation, artifacts and
reproduction commands. Prior local changes and dirty VkFFT were preserved.
