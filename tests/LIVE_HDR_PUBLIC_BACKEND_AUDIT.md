# Public CPU and hybrid representative-frame audit

`audit_live_hdr_public_backends.cjs` follows the accepted-video checkpoint from
the main LIVE HDR cohort evaluation. For each completed video it first audits
the full-video acceptance, then runs frame0 independently through both public
named-option applications. It compares Q and Q_JOD with the clean official
v3.0.7 Single score already recorded for that input pair.

The audit copies both executables, the projection shader and validated model
files into a fresh directory. The FFTW DLL stays at its explicit external path;
its SHA256 is checked against the accepted bundle and between videos. Results,
source PNG/score hashes, backend settings, logs and per-case executable/shader
copies are retained by the CPU and hybrid wrappers. The audit checkpoint records
a video only after both exact comparisons pass and the hybrid wrapper confirms
four actual Vulkan matrix dispatches without reported validation errors.

It processes the two public backends sequentially, adding at most one image-score
evaluation at a time to the main cohort's work. When caught up, it waits for the
next accepted video. A failed comparison stops this audit without changing the
main evaluator's output. If the scheduler disappears before the cohort is
complete, the audit fails instead of waiting indefinitely.

Active run: `validation/live_hdr_public_backends_20261102_run01`.

```powershell
node tests/audit_live_hdr_public_backends.cjs validation/live_hdr_cohort_scores_20261102_run01/config.json build-vs2026/bin/Release/IQM-hdrvdp3-cpu.exe build-vs2026/bin/Release/IQM-hdrvdp3-hybrid.exe validation/live_hdr_gpu4k_20261101_run02/projection.spv <fresh-output>
```

This is **one representative frame per video**, not full-video hybrid evaluation
and not additional unique dataset cases. Only native-color/LMSR matrices run
on Vulkan; remaining processing is CPU. It does not establish full GPU parity,
every original HDR-VDP-3 configuration or original paper cohort reproduction.
The directory must be fresh; interrupted audit outputs are preserved and are
not overwritten. Current progress is in its `checkpoint.json`, separately from
the primary cohort checkpoint.
