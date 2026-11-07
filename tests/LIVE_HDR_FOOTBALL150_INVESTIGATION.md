# football540p frame150: unresolved original reference discrepancy

2026-11-04 update: the eight-run clean-reference probe completed. All8runs gave
40ec7071/41038aea, equal to C++, and the complete reference tree, wrapper,
Octave binary and FFTW DLL fingerprints remained unchanged. Output argument
lengths matched the original run. Artifacts:
`validation/live_hdr_reference_repeatability_20261102_run01`.
This still does not explain the first40ec4f50/41037c75 record.

Additional Nov4 localization on the **later matching persisted tensors**:

- `probe_live_hdr_base_fft.m`:32offset/slice construction scenarios, each with
  an asserted identical Single input vector, produced the same full spectrum
  SHA256 and selected frequency0.029041263918101639 (Double3f9dbcfe3b0d0eb1).
  The next physically different frequency peak is about10.297% lower, giving
  no near-tie evidence for these saved base bands. Scope:135x240 column-major
  Single band difference, no FFT padding, DCcomplexbin zeroed, FFTW3threads /
  estimate / cleared wisdom. Spectrum files are interleaved real/imag Single.
  Offsets are construction scenarios; physical addresses were not measured.
  Artifact: `validation/live_hdr_base_fft_offsets_20261104_run01`.
- A fresh quality capture preserved the **later clean-repeat** score and
  recorded each D_p and scalar per-band error. The original clean tree remained
  unchanged. `probe_live_hdr_pooling.m` evaluated the exact reference Minkowski
  expression across8offset/slice constructions of each of6bands; Single input
  bit equality was asserted before every sum. Input layout is the saved Octave
  column-major D_p flattened into a vector, with no additional image padding.
  Every per-band error hex matched the capture and every reconstructed score
  matched40ec7071/41038aea. No acceptance tolerance was introduced.
  Artifacts: `validation/live_hdr_quality_capture_20261104_run01` and
  `validation/live_hdr_pooling_offsets_20261104_run01`.

### Update 2026-11-05: fresh diagnostic and local continuation completed

The fresh football video diagnostic passed all 17 selected frames, each with
three independent clean official evaluations and the C++ evaluator, including
frame150. Independent aggregation passed. The separate continuation accepted
all 27 declared local videos (513 selected video-frame pairs); its statistics
were independently verified by Octave. Public CPU/hybrid representative checks
are still running. These results belong to the fresh continuation, not the
original failed checkpoint. The initial discrepant score is preserved and its
cause is still unresolved. Older pending statements below describe the earlier
investigation state and must not be read as the current local-cohort count.

These probes do not recover the first failed run's unsaved intermediates and
do not prove that FFT/pooling behavior could never differ in another context.
They narrow hypotheses on the available data; the historical outlier stays open.

A new full-video diagnostic in
`validation/live_hdr_football_triplicate_20261104_run01` requires three clean
reference processes per frame, all identical and equal to C++. Frame150 is
processed first; all17samples and original aggregation order remain unchanged.
No previously accepted scores are reused. Postprocessing will independently
verify aggregation and every reference record's hash. The original cohort
remains21/27; this diagnostic is not automatically promoted into it.

2026-11-02. Failed cohort case:
`540p_2.2M_football1` / `4k_ref_football1`, zero-based150.
Original result is preserved under
`validation/live_hdr_cohort_scores_20261102_run01/results/540p_2.2M_football1_1793653626058/frame0150`.

| Execution | Q hex | Q_JOD hex |
|---|---|---|
| Original clean official3.0.7 run | 40ec4f50 | 41037c75 |
| Original frozen C++ diagnostic | 40ec7071 | 41038aea |
| Public CPU reproduction | 40ec7071 | 41038aea |
| Fresh traced C++ | 40ec7071 | 41038aea |
| Clean official repeat1 | 40ec7071 | 41038aea |
| Clean official repeat2 | 40ec7071 | 41038aea |
| Clean official, BLAS/OpenMP1, FFTW3 | 40ec7071 | 41038aea |
| Clean official, BLAS/OpenMP2, FFTW3 | 40ec7071 | 41038aea |

Both clean repeats used the original persisted linear inputs, unchanged clean
official tree, Octave11.3.0 CPU Single, FFTW3threads/estimate and the same PPD.
Input source/linear SHA256 values match the first reference record. Their roots:
`validation/live_hdr_failure_stock_repeat_20261102_run01` and `run02`.
This demonstrates inconsistent reference records across these executions;
the cause of the first differing result remains unproven. It is not sufficient
to select matching repeats and declare the cohort case accepted.
The two additional controlled BLAS/OpenMP thread experiments also match C++;
changing those thread counts did not reproduce the first recorded result.
These experiments have separate roots `live_hdr_failure_stock_blas1_20261102_run01`
and `live_hdr_failure_stock_blas2_20261102_run01`; they are diagnostics with
explicitly changed environment settings, not undisclosed replacements for the
original profile.

## Stage evidence and its limits

`live-frame-trace` is a separate diagnostic target with binary stage exports.
Its two linear tensors match the original preprocessing byte-for-byte.
A fresh instrumented official copy exports native RGB, optical RGB, LMSR,
adaptation, JND response and all six pyramid bands. All exported stages match
the C++ trace bitwise after explicit row/column layout conversion.
The comparator checked287,517,600Single values across24tensors, with zero bit
differences for this capture pair. This count does not establish equality to the
first failed reference execution whose intermediate tensors were not saved.

However, the instrumented capture's original-score preservation assertion
FAILED: it produced40ec7071/41038aea, matching C++ and the later clean repeats,
not the first recorded reference result. It cannot establish which stage first
differed during that initial execution. Do not report this as an accepted
instrumented reproduction of the failing original run. Clean source files and
old exports remain unchanged.

Artifacts:

- `validation/live_hdr_failure_capture_20261102_run01` (failed preservation assertion).
- `validation/live_hdr_failure_cpptrace_20261102_run01`.
- `validation/live_hdr_failure_stages_20261102_run01.json`.
- `validation/live_hdr_mismatch_football150_cpu_20261102_run01` (public reproduction failed original target).

## Harness correction

The per-video runner previously used Promise.all: one failed worker rejected,
but the other kept scheduling the remaining frames. It now stops scheduling
new frames after failure and drains already running tasks before reporting
failure. Numerical acceptance is unchanged. Already-running old processes are
not changed retroactively. Syntax check passed; no metric algorithm fix is
claimed from this scheduling correction.

Next: investigate clean-reference repeatability and capture the discrepant
execution, including FFT/quality pooling behavior and the execution environment.
Retain the first failure, every repeat and the frozen binaries. Keep this video
quarantined; do not weaken exact comparison, skip frame150 or silently replace
its original reference score. No complete-cohort correlation is valid yet.
