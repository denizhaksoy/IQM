# Additional Single quality profile validation — 2026-11-06

Following the 513-frame local LIVE sweep, **14 additional synthetic input/setting cases passed
fresh official v3.0.7 / independent CPU / independent combined-GPU-profile
comparison**. Both Q and Q_JOD were bitwise identical, with no reported Vulkan
validation errors. No tolerance threshold was used.
These cases contain 12 distinct PNG input pairs; three PPD settings deliberately
reuse the same 128x128 pair. They are not counted as14 unique image pairs.

| Coverage | Tested dimensions / settings |
| --- | --- |
| Landscape and portrait | 96x80, 80x96 |
| Odd dimensions | 127x95, 95x127 |
| Small dimensions | 17x33, 33x17 |
| PPD changes | 26, 32, 62.729130063099447, 128, 256 |
| PQ-domain up/down resizing | 17x29 to 31x47; 59x41 to 47x31 |
| Identity | Same RGB16 PQ image on both inputs |
| Deeper pyramids | 256x256 and 512x512, PPD256 |

The combined GPU backend executed four matrix dispatches and one comparison-mask
dispatch, with 3–8 mutual-activity dispatches according to image geometry/PPD.
The two deeper cases explicitly verified seven and eight mutual dispatches.
Remaining pipeline stages still run on CPU.

These are deterministic RGB16 PQ stress inputs, not an extension of the paper's
original LIVE cohort or its subjective statistics. PPD is explicitly varied as
an API parameter; no original display geometry is inferred for these fixtures.
The other model settings remain quality/sp0, age24, OLED, 200lux ambient,
reflectivity0.005, peak2000 before reflected ambient, and Single FFTW with
three threads/estimate/cleared wisdom. Reference and application preprocessing
independently perform PQ-domain bicubic resize/clamp and inverse PQ.

The official tree is not modified. Input PNGs are generated deterministically,
read back to assert RGB16 byte values, and hashed. Reference preprocessing uses
planar row-major little-endian Single exports; those tensors are supplied only
to the official score script, never to either application. Both applications
read the original PNGs. Existing pipeline padding and base-band DC handling
remain unchanged and are recorded in provenance.

## Artifacts and reproduction

- `validation/hdrvdp3_profile_matrix_20261106_run01`: first12 cases, all exact.
- `validation/hdrvdp3_profile_matrix_20261106_run02`: deep7/deep8, both exact.
- Each root retains `checkpoint.json`, `completion_audit.json`, fresh official
  scores, CPU/GPU scores/logs, frozen runtime/model, fixture hashes and source
  snapshots matching the recorded script SHA256 values.

```powershell
node tests/run_hdrvdp3_profile_matrix.cjs <clean-reference> <accepted-bundle> <cpu-exe> <frozen-mutual-runtime> <fresh-output> [case-names-comma-separated]
```

Generation: `generate_hdrvdp3_profile_fixtures.m`; explicit-PPD preprocessing:
`prepare_hdrvdp3_quality_profile.m`. The original LIVE preprocessing/scoring
files and old numerical exports were preserved. Failure records are retained
and the matrix cannot report a pass when any case differs.

## Additional real-content validation completed

The new backend passed all16 selected frames and the Single video mean of the
previously accepted `4k_3M_Daylight1` / `4k_ref_Daylight1` video. This content is
outside the completed 27-video/three-content backend sweep. The helper
`verify_additional_live_hdr_backend.cjs` verifies the saved acceptance,
metadata/MP4 identities, timing, profile, original PNG hashes and each fresh
application result before comparing the native Single video mean with the
previous independent Octave aggregation. Legacy plan IDs are verified from
metadata and hashed video paths; old plan files are not rewritten.

Result: `validation/live_hdr_daylight_mutual_20261106_run01/checkpoint.json`.
Independent saved-result audit:
`validation/live_hdr_daylight_mutual_20261106_run01/completion_audit_20261106_run01.json`.
Q_JOD mean is8.4828519821166992, bits4107b9c3. The combined new-backend real
coverage is529 unique PNG pairs across28 videos/four contents, verified by
source-pair hashes against the earlier513 records; the16 additions are distinct.
This evaluation uses preserved official records; the synthetic matrix above
uses fresh official evaluations. Neither result resolves the historical
football150 discrepancy or proves original-paper/full-GPU completion.
