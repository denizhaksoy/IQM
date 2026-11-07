# HDR-VDP-3 acceptance and publication claims

The project has two independent acceptance targets. Passing one does not prove
the other. GPU coverage is recorded separately from both.

## Publication checkpoint: 2026-11-07

The comparison, mutual-activity and optional Double comparison-mask resize GPU
stages are implemented in separate experimental quality backends. The runtime
remains hybrid; full_gpu=false. New shaders require enabled Float64 support.

Completed evidence includes the combined non-resize backend on513+16distinct
real frame pairs,14synthetic setting cases (12distinct image pairs),16/16compact
reference workflow comparisons and529real-mask resize-stage checks covering
1,456,866,000Double values with exact bits and final preservation audits.
The new resize-enabled compact Daylight end-to-end pilot passed16/16frames and
its orderedSingle video mean matched preserved reference results.

All310supplied MP4s are ZIP-byte-verified and HDR stream headers checked.
The declared full archive has279distorted videos/31reference groups and4716
phase0/stride30samples. Its numerical evaluation is IN PROGRESS, not complete.
At this publication check6videos were complete; the count is a dated checkpoint,
not a claim that the full archive or original paper results have been reproduced.
The original football150 reference discrepancy and undocumented author settings
remain unresolved. Author questions are a local draft and have not been sent.
Earlier dated scope below is retained as historical context.

## Numerical implementation equivalence

Use clean official HDR-VDP-3 v3.0.7 on identical hashed image inputs, explicitly
matched precision, preprocessing, task, viewing/model settings and backend.
Q and Q_JOD require matching Single bits for the current Single quality profile.
A tolerance result must never be relabeled exact. Independently verify video
aggregation; keep full-video results distinct from incomplete selected frames.
Count repeated input pairs once when describing unique test coverage.

The active LIVE evaluation uses quality, age24, BT.2020/OLED defaults,
ppd62.729130063099447, RGB16 PQ PNG decoding, phase0/stride30, PQ-domain bicubic
resize/clamp, inversePQ, ambient200lux/reflectivity0.005 and peak2000 before
ambient. CPU Octave Single/FFTW3threads/estimate is the numerical reference.
Those choices are documented in `live_hdr_protocol.json`. This is a concrete
supported profile, not evidence for every task, age, spectrum, dimension,
numerical precision or GPU/driver combination accepted by the original metric.

Update 2026-11-05: the fresh declared 27-video local continuation completed,
including evidence auditing of 513 selected frame pairs and independent Octave
statistics verification. Official and C++ predictions have identical local
raw PLCC (approximately 0.901651) and SROCC (approximately 0.935287). The
historical football150 discrepancy remains open and is retained in the analysis.
Representative public CPU/hybrid checks are still running; this is not an
original-paper reproduction or a result for all 279 archive distortion pairs.
See the resume receipt under
`validation/live_hdr_verified_continuation_20261104_run01/resume_20261105_run01`.

Required next evidence: complete the public-backend audit, resolve or explicitly
bound the historical discrepancy, and independently validate any new GPU stages.
Record unsupported configurations and
remaining CPU stages explicitly. Full GPU completion requires the remaining
stages to execute on GPU and retain the required reference agreement; matrix
dispatches alone do not establish that.

## Original paper experiment reproduction

[Section6 of the paper](https://www.cl.cam.ac.uk/~rkm38/pdfs/mantiuk2023hdrvdp3.pdf)
identifies LIVE HDR, every30thframe, framewise quality with averaging, inversePQ
into absolute BT.2020 and reflected ambient. It retains the UPIQ-calibrated model.
It supplies neither a per-video reference prediction vector nor a numeric
correlation target in that section. The release example is supporting evidence,
not a complete execution manifest for the submitted experiment.

Still required for a claim of matching the original submitted results:

- Exact original train/public/private cohort IDs and reference inclusion rules.
- Frame-selection phase and actual FFmpeg color/PNG precision commands.
- Actual software/backend/precision and correction of the example's fixed frame filename.
- Author prediction vector or the precise reported result and its target MOS,
  split, correlation computation and any fitting/mapping procedure.

The current27video candidate is local and its author-confirmed flag remains
false. Do not infer missing settings by fitting predictions until they resemble
a published number. Equal C++/official predictions on the same documented
inputs establish implementation equivalence for that experiment; they do not
identify the unknown original cohort or decoding commands. Similarly, perfect
implementation agreement does not imply a correlation of1 with human scores.

Until those items are supplied by a primary source, report a documented local
reconstruction and its measured numerical coverage. Do not promise universal
bitwise identity or original-publication equality from finite local tests.
