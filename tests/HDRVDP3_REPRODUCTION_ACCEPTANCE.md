# HDR-VDP-3 acceptance and publication claims

The project has two independent acceptance targets. Passing one does not prove
the other. GPU coverage is recorded separately from both.

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

Required next evidence: finish the declared27video candidate cohort, audit
each exact frame and mean, compute the complete-cohort statistics and independently
verify those statistics with Octave. Record unsupported configurations and
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
