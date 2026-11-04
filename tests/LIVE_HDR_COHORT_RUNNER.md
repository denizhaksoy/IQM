# Resumable local LIVE HDR cohort evaluation

2026-11-04 investigation update: the original cohort stopped at21/27 after an
unresolved reference discrepancy. Older descriptions of active jobs below are
historical; consult SESSION_HANDOFF.md for live processes.

## Repeated-reference diagnostic

The per-video positional CLI now accepts `reference-runs` (1..5, default1)
after the optional seed root, followed by an optional priority frame. Use `-`
for no seed. A priority frame must belong to the unchanged phase0/stride30
sample list; it changes processing order only. Aggregation still follows the
original sorted frame order.

```powershell
node tests/run_live_hdr_video_pair.cjs <dataset> <official> <bundle> <fresh-output> 540p_2.2M_football1 4k_ref_football1 - 3 150
```

Multiple-reference mode freshly decodes and computes every frame; it disables
score reuse. Each independent reference process writes its own score/log.
`reference_consistency.json` preserves all score file hashes. Every observation
must have the same profile/input hashes and identical finite Single Q/Q_JOD
bits. Any outlier fails, even when a majority agrees with C++. The result must
also match independent C++; neither numerical tolerance nor vote counting is
used. Completed-video audits recheck the repeated score files and hashes.

`verify_live_hdr_repeated_video.cjs <root> <dataset> <runner-PID>` waits for this
diagnostic, then checks metadata/PNG provenance, reference repetitions and
independent Octave aggregation. It does not merge the run into a cohort or
resolve historical discrepancies. Original failed records remain preserved.
Nine guards, including the actual historical outlier and changed settings,
passed in `validation/live_hdr_reference_consistency_checks_20261104_run01`.

## Conditional continuation in a fresh cohort directory

`continue_live_hdr_verified_cohort.cjs original-config repeated-video-root
fresh-root` waits for the independently audited diagnostic marker. It audits
the repeated records again before creating any new cohort configuration.
After success, it imports the original accepted videos and the newly verified
retry into a fresh directory, applies three reference runs to every remaining
new video, then runs analysis, independent Octave statistics, evidence auditing
and representative public CPU/hybrid checks. The original checkpoint is copied
and hash-checked, never overwritten. Failed diagnostics stop before a new
configuration is created.

The pending historical discrepancy is retained explicitly in the prediction
manifest and analysis. Reference-run counts are reported per video: original
single-reference results are not relabeled triplicate. Completion of this
current-run local validation does not resolve the historical outlier, establish
full GPU processing or verify the original paper submission's cohort/settings.

`run_live_hdr_cohort.cjs` schedules videos sequentially using the existing
per-video reference/C++ comparison, acceptance audit, and independent Octave
Single-precision aggregation. It does not change the evaluation protocol.

Configuration paths are relative to the configuration file:

```json
{
  "dataset": "../staged-dataset",
  "official": "../official_clean/hdrvdp-3.0.7",
  "bundle": "../accepted-single-bundle",
  "cohort": "../candidate/candidate_cohort.json",
  "output": "results",
  "accepted": [{"video": "720p_2.6M_Bonfire", "root": "../accepted-video"}]
}
```

Run from the repository root:

```powershell
node tests/run_live_hdr_cohort.cjs validation/live_hdr_cohort_scores_20261102_run01/config.json 27
```

The last argument limits newly evaluated videos per invocation; the default is
one. Accepted videos are rechecked against source MP4/metadata hashes, decoded
PNG hashes, exact Q/Q_JOD comparisons, the declared phase/stride/PPD, and the
independent Single aggregation before reuse. Failed attempts remain in fresh
directories. Resume currently operates at video boundaries: an interrupted
video always gets a new directory, never overwriting its prior attempt.
New-format runs can reuse previously exact frames after fresh decoding and
matching all fingerprints described below; legacy runs are recomputed.

For a read-only snapshot of accepted videos, partial exact frames, latest
failure and scheduler PID:

```powershell
node tests/status_live_hdr_cohort.cjs validation/live_hdr_cohort_scores_20261102_run01/config.json
```

This snapshot is a progress aid, not an acceptance audit or accuracy report.

`checkpoint.json` records attempts and accepted videos. `video-predictions.json`
is suitable for `analyze_live_hdr_cohort.cjs` only once the entire declared cohort
is complete; that analyzer rejects incomplete cohorts. No correlation fitting,
recalibration, or original-author-cohort equivalence is implied.

An exclusive `scheduler.lock` prevents simultaneous writers. After an abnormal
termination, inspect the recorded PID and its child processes before removing a
stale lock. Do not remove a live scheduler's lock.

To wait for the current scheduler, process all remaining videos, and invoke
analysis only after successful completion:

```powershell
node tests/run_live_hdr_cohort_pipeline.cjs validation/live_hdr_cohort_scores_20261102_run01/config.json validation/live_hdr_cohort_analysis_20261102_run01
```

The analysis root must be fresh. If the prior scheduler's latest attempt was
not accepted, the continuation stops for inspection. It never removes a lock
or silently skips a numerical failure. Checkpoint and prediction-manifest
updates use temporary files followed by rename. The continuation process must
remain running; it is not a Windows service and does not survive shutdown.

For an already-running pipeline that predates the Octave statistics step,
`watch_live_hdr_analysis.cjs config.json analysis-root` attaches a separate
verifier without restarting the evaluation. It waits for the final analysis,
checks the exact cohort manifest/IDs, invokes the independent Octave verifier,
and records a status JSON next to the analysis root. Its exclusive lock and
preserve-existing-output checks prevent duplicate verification writers.
It stops if the scheduler disappears before analysis is produced. The current
run has this watcher attached; see SESSION_HANDOFF.md for its session ID.

Integrity verification additionally recomputes the video mean from accepted
frame hex values, checks all decoded timestamps and reference-input provenance,
and rejects changed score indices. Engineering fixture checks passed in
`validation/live_hdr_scheduler_checks_20261102_run01` (eight checks); these are
integrity tests, not additional dataset accuracy cases. The existing real
18-frame Bonfire720 result also passed the strengthened audit.

## Frame recovery for interrupted new-format attempts

The per-video runner now accepts an optional final `seed-root` argument after
the test/reference IDs. A restarted scheduler selects the previous attempt for
that same video as its seed. Every selected frame is freshly decoded. Reuse
requires identical source MP4/CSV hashes, selected indices/timestamps, PPD,
phase/stride, RGB48 layout, executable/FFTW hashes and a calculation fingerprint.
The latter covers the official reference tree, frozen bundle manifest and
decode/preparation/scoring/runner/reuse source files. Score bits must be finite
Single values and Q/Q_JOD exact; official Single/CPU/v3.0.7 settings and both
old/new PNG hashes are checked. The new comparison records its source JSON
path and SHA256. No reference tensors are injected into C++.

An incompatible or legacy fingerprint triggers recomputation. Corruption in a
claimed exact source fails explicitly. Existing files are preserved. Eleven
engineering integrity checks passed in
`validation/live_hdr_frame_reuse_checks_20261102_run01`. A complete real-video
recovery rehearsal also passed as described below. Already running Node processes retain
their loaded code: the active scheduler was not restarted to install this change.
Subsequent per-video processes record the new fingerprints; automatic seed
selection applies on a future scheduler invocation.

### Real-data recovery rehearsal

`tests/verify_live_hdr_video_recovery.cjs` waits for an accepted seed, invokes a
fresh full per-video run with that seed, requires every frame to be recovered
after fresh decoding, runs acceptance and independent Octave aggregation, and
compares the final Single mean hex against the seed.

2026-11-02: `1080p_1M_football1`, all17selectedframes freshly decoded and reused
with matching source/calculation fingerprints. Both Q and Q_JOD remained exact;
video mean7.7903523445129395, Single40f94a91; independent Octave passed.
Artifact: `validation/live_hdr_football_recovery_20261102_run01/recovery_acceptance.json`.
The original seed and active cohort pipeline were not interrupted. This is a
recovery-path rehearsal, not proof of OS-crash handling and not17new metric cases.

2026-11-02: syntax check passed; concurrent invocation was rejected with EEXIST
while the original lock remained intact. Existing Bonfire720 acceptance was
successfully audited and imported. The first new attempt is
`1080p_1M_Bonfire`; its numerical result is pending.
