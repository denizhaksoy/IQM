# LIVE HDR selected-frame video equivalence

Scope: one `4k_3M_Daylight1` / `4k_ref_Daylight1` pair, under the documented
local Section6 reconstruction. Completion is established by the generated
`video_score.json`, `acceptance.json` and `aggregation_octave.json`, not this
document alone. This is not full-dataset or original challenge reproduction.

## Measured completion: 2026-11-01

All16 selected frame pairs passed exact Q **and** Q_JOD comparison. The ordered
Single video mean also matches:

| Output | Official | Independent C++ |
|---|---|---|
| Mean Q_JOD | 8.4828519821166992 | 8.4828519821166992 |
| Single hex | 4107b9c3 | 4107b9c3 |

The post-run hash/metadata acceptance and independent Octave aggregation checks
both exited0. Fourteen frame results were newly computed; frame0/30 were reused
only after new decoding produced exactly the accepted PNG hashes. The bright
MOS linked to this distorted video is55.33486906 and bright DMOS30.60362545;
these human labels are different quantities from JOD and no correlation is
claimed from one pair. All acceptance artifacts listed below are now present.

## Protocol

Both videos contain480 decoded frames. The runner compares every frame's
best-effort presentation timestamp string and rejects any count/timing
mismatch before evaluating quality. Selection is zero-based
`0,30,60,90,120,150,180,210,240,270,300,330,360,390,420,450`.
Phase0 is an explicit local interpretation of the example loop; the paper's
starting phase remains independently unconfirmed. RGB48 PNG decoding is also
a documented local choice, not a recovered author setting.

Per-frame settings: clean official v3.0.7 CPU Octave11.3.0 versus frozen
independent C++ Single PNG-to-score implementation; BT2020/OLED, age24,
ppd62.729130063099447, quality/sp0, 200lux*0.005/pi reflected ambient,
2000cd/m² peak before ambient, FFTW Float three threads/estimate.
All frames retain3840x2160 geometry. No temporal model or time-weighted pooling
is introduced. Each official and C++ frame score must match in Q and Q_JOD bits.

Video aggregation is the arithmetic mean of per-frame Q_JOD, using ordered
Single accumulation as in the source example. It is not meanQ followed by a
nonlinear conversion. The wrapper reproduces Single rounding explicitly;
`tests/verify_live_hdr_video_aggregation.m` independently checks the result
using Octave Single additions and division.

The freshly decoded frame0/30 PNG hashes must equal the previously accepted
bundle's inputs before those two prior exact scores can be reused. All other
frame scores are newly calculated. Reused cases are marked in their records.

## Artifacts and acceptance

Root: `validation/live_hdr_video_20261101_run01`.

- `plan.json`: source SHA256, all480 reference/test timestamps, sampling indices,
  executable/dependency hashes and explicit reconstruction choices.
- `frameNNNN/decoded`: PNGs, ffprobe outputs, decode commands/version/hashes.
- `frameNNNN/comparison.json`: official/C++ score bits, source hashes and reuse flag.
- New cases also retain preprocessing tensors and separate official/C++ logs.
- `video_score.json`: written only after all16 frame pairs are exact.
- `acceptance.json`: full output/image verification and subjective metadata join.
- `aggregation_octave.json`: independent Octave aggregation check.

`tests/run_live_hdr_video_pair.cjs` performs the evaluation with at most two
workers; two are enabled only when startup free RAM exceeds14GiB. Any numerical
mismatch prevents video acceptance. `tests/summarize_live_hdr_video.cjs` verifies
every selected case and binds the exact CSV `video`/`ref` keys to human labels.
Bright and dark MOS/CI95/DMOS are retained separately. Correlation is deliberately
not calculated from a single distorted video, and MOS is not treated as Q_JOD.

## Commands

```powershell
# The output path must be fresh; preserve the completed run.
node tests/run_live_hdr_video_pair.cjs validation/live_hdr_639290787607997134 validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7 validation/live_hdr_single_demo_20261101_run02 <fresh-video-root>
node tests/summarize_live_hdr_video.cjs <completed-video-root> validation/live_hdr_639290787607997134
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet tests/verify_live_hdr_video_aggregation.m <completed-video-root>
```

## Limits and next coverage

This proves equivalence only for the named pair and explicit local settings
when all acceptance files pass. It does not establish every possible image,
all310 public files, the reserved challenge cohort or subjective correlation.
The separate 540p frame checkpoint verifies one PQ-domain upsampling case;
it is not a complete lower-resolution video result. General Vulkan execution
and main IQM CLI integration remain outstanding.

Next coverage: additional contents and bitrate/resolution ladders, complete
lower-resolution video samples, declared evaluation cohort/reference exclusion,
and correlation analysis with enough videos. Preserve the protocol audit's
unresolved author PNG depth, frame phase and execution environment.
