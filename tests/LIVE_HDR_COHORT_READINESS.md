# Declared cohort inputs and analysis readiness

2026-11-02. The declared local candidate subset has27 distorted clips and three
reference contents. The original challenge training CSV has not been recovered;
this is not a certified reconstruction of the authors' exact cohort.

`stage_live_hdr_cohort.cjs` / `.ps1` materialized only those30 MP4s from the user's
existing ZIP. Every existing or newly extracted file was SHA256-compared with
the corresponding decompressed archive entry. Existing mismatches are preserved
and rejected rather than overwritten. Resolved destinations must stay within
the workspace. The ZIP is opened read-only and its fingerprint is recorded.

All30 stream headers agree with local metadata in dimensions and container frame
count, and report `yuv420p10le`, `smpte2084`, `bt2020` primaries, `bt2020nc` matrix
and limited (`tv`) range. Distribution:12 UHD,9 1080p,6 720p,3 540p, including
the three UHD references. Actual rational fps and nominal CSV fps are recorded
separately. Header counts are not equivalent to decoding/timestamp validation
of every frame; the complete video runner performs that later.

Artifacts: `validation/live_hdr_cohort_inputs_20261102_run01` with extraction
identity records, original ffprobe JSONs and `inventory.json`. The archive hash
is a local provenance fingerprint, not a verified author-published checksum.

## Analysis policy and checks

`analyze_live_hdr_cohort.cjs` requires the complete unique declared video list,
accepted video-level exact predictions, independent Octave Single aggregation,
matching reference/subjective keys and metadata hashes, and the explicit local
sampling/viewing/decode policy. It rejects incomplete cohorts rather than
computing a favorable partial result. References are excluded by the manifest.

Outputs are average-tie SROCC and raw, unfitted Pearson correlation against
bright MOS. No training, nonlinear fit or guessed JOD-to-MOS RMSE mapping is
introduced. RMSE remains null with a reason. The author-cohort verification
flag remains false when the input cohort is only a local candidate.

Tests compare engineering vectors with Octave `corr`/`spearman` at absolute
tolerance1e-14, check average ties, constant/nonfinite rejection, incomplete
cohort rejection and complete synthetic CLI record joins. This tolerance check
is statistical-tool verification, not metric bitwise acceptance. The negative
correlation values in those fixtures are NOT LIVE HDR accuracy results.

Artifacts: `validation/live_hdr_statistics_20261102_run01` and run03. The
statistics package is ready to consume enough accepted videos; no real27-video
correlation analysis has yet been performed.

```powershell
node tests/analyze_live_hdr_cohort.cjs <declared-cohort.json> <complete-video-predictions.json> <fresh-output> --local-cohort
```

Prediction manifest schema: `IQM_LIVE_HDR_VIDEO_PREDICTIONS_1` with a `videos`
array of `{video, root}` records. Root paths resolve relative to that manifest
and contain `plan.json`, `video_score.json`, `acceptance.json` and
`aggregation_octave.json`. Frame-level predictions are not accepted as video MOS.

## Active continuation, 2026-11-02

Latest checkpoint supersedes the history below:5/27videos accepted (93frames),
including Bonfire1080p6M18/18 exact, mean8.1062984466552734/Single4101b366.
CargoBoat1080p6M is evaluating. All completed evidence including Daylight1:
`validation/live_hdr_evidence_20261102_run05` ->7pairs/4contents/125distinctcases.

`verify_live_hdr_cohort_analysis.m` independently recomputes the final analysis
using Octave corr/spearman with statistics-only tolerance1e-14. Engineering
six-vector fixture passed and an intentionally wrong SROCC was rejected;
these are not LIVE correlations. New pipeline invocations execute it after
analysis. The already-running process predates this addition, so its output
must receive that verification separately once complete (see SESSION_HANDOFF).

Latest update: CargoBoat1080p1M completed22/22 exactframes, video mean
5.7304935455322266 (Single40b76034), independent Octave aggregation passed.
Now3/27 acceptedvideos,58cohortframes. Scheduler automatically advanced to
1080p_1M_football1. The partial progress below is historical; no complete-cohort
correlation yet. Cumulative evidence including two external Daylight1 tests:
validation/live_hdr_evidence_20261102_run03 ->5videos/3contents/90distinctcases.

The scheduler and continuation described in [LIVE_HDR_COHORT_RUNNER.md](LIVE_HDR_COHORT_RUNNER.md)
are running from `validation/live_hdr_cohort_scores_20261102_run01/config.json`.
One candidate video is accepted (Bonfire720); the next, Bonfire1080p1M, has
six exact selected frames so far and is not yet accepted as a full video.
The continuation will evaluate remaining videos sequentially and run analysis
only after successful complete-cohort acceptance. Any failed video stops that
sequence. No actual complete-cohort correlation is available yet.
