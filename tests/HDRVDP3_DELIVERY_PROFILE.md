# Scope-frozen HDR-VDP-3 delivery profile

Target: validated hybrid CPU/Vulkan delivery by 2026-11-20; second metric starts
2026-11-23. No new manual FFTW kernels, unrelated GPU ports or performance claims.

## Main IQM CLI contract (integration increment)

Profile: live-hybrid. Task: quality/sp0, RGB16 PQ BT.2020 PNGs. Parameters preserved
from the existing standalone Single pipeline: OLED, age24, ambient200lux,
reflectivity0.005, peak2000cd/m2 before ambient. LIVE evaluation uses explicit
PPD62.729130063099447. The CLI requires an explicit PPD; existing [26,256]
parameter support is retained for synthetic regressions, not a claim that all
settings have been validated.

Input processing is unchanged: normalize uint16 to Single, bicubic-resize the
distorted image in PQ to reference dimensions, clamp encoded values to [0,1],
inversePQ and ambient/peak processing. No image resizing is introduced for the
reference. CPU FFTW Single provider uses three threads and estimate planning.
Both inputs must decode as three-channel 16-bit images of at least16x16.

The profile enables all six existing validated GPU backends. Matrices execute
4dispatches/imagepair; comparison1; L+M2; padding8. Mutual dispatches and comparison
resize counts depend on pyramid bands and are recorded in gpu_stages.json.
Projection coefficients/model preparation, RGB normalization/PQ resize/EOTF,
FFTW, adaptation nonlinear math, JND, pyramid, remaining CSF/masking/pooling and
final Q/Q_JOD calculations remain CPU work. Buffers use synchronous transfers.
The profile is hybrid; full_gpu=false. No speedup claim.

Main CLI usage (runtime and provider are explicit, not inferred from Downloads):

~~~powershell
.\IQM.exe --method HDRVDP3 --input test.png --ref reference.png --hdrvdp3-profile live-hybrid --hdrvdp3-encoding pq-bt2020-rgb16 --hdrvdp3-runtime runtime --hdrvdp3-fftw "C:\path\libfftw3f-3.dll" --hdrvdp3-ppd 62.729130063099447 --hdrvdp3-export fresh-output --enable-validation
~~~

Runtime layout required by the adapter:

~~~text
runtime/
  projection.spv
  mask.spv
  mutual.spv
  resize.spv
  adaptation.spv
  padding.spv
  model/
    ciexyz31.csv
    cie_scotopic_lum.txt
    emission_spectra_oled.csv
    log_cone_smith_pokorny_1975.csv
~~~

Missing shaders fail explicitly; no numerical CPU fallback is substituted.
Model overrides, device overrides, other ages/displays and colorized image output
are rejected in this profile. Existing legacy CLI profiles remain separate.
Vulkan Float64 support is required by the existing mask/mutual/resize backends.
The current implementation selects the existing Float64-capable device profile.

Outputs: score.json (Q/Q_JOD bits, settings, processing elapsed time),
gpu_stages.json (actual dispatch counters, full_gpu=false), device.txt and
progress.log. Validation-layer output is captured by the launcher. Processing
time excludes some startup/model-loading work; the integration launcher also
records whole-process wall time. These measurements alone do not establish speedup.

## Integration approach and current boundary

The standalone CLI's numerical body is moved unchanged to
bin/shared/hdrvdp3_single_cli.cpp and reused by the main IQM executable. The old
CPU/hybrid/mask/mutual executables retain thin entry points and their original
compile-time profiles. No reference source or arithmetic stage is changed.

This increment validates CLI routing and preserved calculations. Existing build
cache/runtime assets are used for its tests. A clean-build/runtime preparation
procedure and the predefined LIVE subset are NEXT review-gated tasks, not claimed
complete here. Runtime files must later be generated/copied with documented
commands, hashes and redistribution requirements. No dependency on a historical
validation directory is acceptable for final delivery.

Unresolved Light3 alignment and historical football150 reference discrepancies
remain open; this CLI connection does not resolve them. The synthetic-only gate
contains14settings/12distinct input pairs. Its omission of the two real-HDR cases
in the earlier manifest is an explicitly limited integration scope, not a failed
case being removed from the final LIVE evaluation.

## Clean-build checkpoint completed (2026-11-11)

The later clean-build/runtime step passed; see [HDRVDP3_CLEAN_DELIVERY_2026-11-11.md](HDRVDP3_CLEAN_DELIVERY_2026-11-11.md) for the opt-in same-main delivery build, all runtime dependencies, portable synthetic inputs and the single4Ksmoke result. Earlier integration-only boundaries above are historical. Full12-pair acceptance remains pending review.

## Delivery review update (2026-11-12)

The subsequently approved12-pair acceptance completed:219/219frame Q/Q_JOD bits
and12/12ordered Single video means exact; zero reported Vulkan validation errors.
Controlled sequential CPU/hybrid timing on one accepted4Kpair completed with
three timed runs/backend; hybrid was slower, not a demonstrated GPU speedup.
See [final delivery report](HDRVDP3_FINAL_DELIVERY_2026-11-12.md) for measured
times, reproducible commands, limitations and the proposed commit review. Earlier
pending statements in this document describe their dated checkpoints. No commit
or push has been performed; Light3 and original football150 remain OPEN.
