# HDR-VDP-3 scope-frozen delivery review

Status: numerical acceptance and benchmark completed; delivery awaits user review.
This is a hybrid CPU/Vulkan quality metric in the main IQM CLI, not a fully GPU
implementation, not a universal equivalence proof, and not reproduction of all
experiments or subjective correlations in the paper. No commit or push authorized.

## Supported profile

Official numerical target: HDR-VDP-3 v3.0.7. Delivery profile `live-hybrid`, quality
task, sp0 pyramid, Single image arithmetic with the reference-required Double
operations retained. Inputs are three-channel 16-bit PNG PQ BT.2020 code values.
Reference geometry is preserved; distorted images are bicubic-resized in PQ,
clamped, and processed through the same inverse PQ/display model as the reference.
Fixed OLED/age24/ambient200lux/reflectivity0.005/peak2000cd/m2 before ambient;
LIVE PPD62.729130063099447. Float32 FFTW uses three threads and estimate planning.
The parser accepts PPD26..256 and minimum16x16 inputs, but this is NOT a claim
that every geometry, parameter, image domain, device, or driver has been validated.
Other tasks, ages/displays, image visualization, GPU FFT, and general performance
optimization are outside the delivered scope.

|Work|Execution in the delivered pipeline|
|---|---|
|Native/LMSR projections|Vulkan, 4 dispatches per accepted 4K pair|
|Comparison mask|Vulkan, 1 dispatch|
|Mutual activity|Vulkan, 6 dispatches|
|Comparison-mask resizing|Vulkan, 6 calls / 8 dispatches|
|Adaptation L+M input|Vulkan, 2 dispatches|
|Optical/adaptation padding|Vulkan, 8 dispatches|
|PNG decoding, normalization, PQ resize/clamp, display EOTF|CPU|
|Model coefficients, spectral preparation, JND/LUT|CPU|
|Optical/local-adaptation FFT and inverse transforms|CPU FFTW|
|Pyramid and remaining CSF/masking/pooling/Q/Q_JOD|CPU|
|Transfers and scheduling|Host-visible coherent buffers, synchronous map/copy, submit/fence wait/readback|

The GPU stages really dispatch, rather than being isolated prototypes. No failed
GPU stage is silently substituted with CPU output. Float64-capable Vulkan hardware
is required. Dispatch counts depend on geometry/bands outside this 4K profile.

## Numerical acceptance

Frozen before scoring: Bonfire, CargoBoat, Daylight1, football1, each at archive
variants 540p_2.2M, 1080p_1M, 4k_3M, paired with its own 4k_ref. Every30th decoded
frame starting at0. Both implementations receive identical preserved RGB16 PNG
bytes, preprocessing and viewing settings. Fresh official reference and final IQM
executions were used, not cached scores. Full selection and exact stream/bitrate
details: [selection protocol](HDRVDP3_LIVE_ACCEPTANCE_SELECTION_2026-11-12.md).

|Video|Frames exact / sampled|Reference = IQM mean Q_JOD Float32 bits|
|---|---:|---|
|540p_2.2M_Bonfire|18/18|40f86667|
|1080p_1M_Bonfire|18/18|40a27a1e|
|4k_3M_Bonfire|18/18|40b29474|
|540p_2.2M_CargoBoat|22/22|40d37ab0|
|1080p_1M_CargoBoat|22/22|40b76034|
|4k_3M_CargoBoat|22/22|40d29adc|
|540p_2.2M_Daylight1|16/16|41088d6c|
|1080p_1M_Daylight1|16/16|4100b1a3|
|4k_3M_Daylight1|16/16|4107b9c3|
|540p_2.2M_football1|17/17|41035d7e|
|1080p_1M_football1|17/17|40f94a91|
|4k_3M_football1|17/17|41049f0a|

219/219 frame Q and Q_JOD bitwise matches; 12/12 video means bitwise matches;
zero mismatches and zero reported Vulkan validation errors. Video aggregation is
ordered Float32 summation followed by Float32 division. This explicit local
protocol does not imply the paper documents aggregation arithmetic precision.
Acceptance wall23205.699s is an execution record, NOT a benchmark. Earlier clean
build regression:14settings/12distinct synthetic pairs exact, plus a fresh4K smoke.

Evidence directory (generated, excluded from Git):
`validation/hdrvdp3_live_acceptance_20261112_run01/` contains checkpoint,
frames.json, per-frame PNGs/scores/logs/dispatch counters, per-video scores and
DETAILED_ACCEPTANCE_REPORT.md. Frozen manifest SHA256:
`65bde31ffe3aee7840ad5b15f5cf464a2b58fbebf43fa494c330e979ad7d58db`.
Final IQM.exe SHA256:
`bcf6c0316ac607a56d890243ab11f8df19a666a091d4c6112eae6cd4f5f0c62d`.

Read-only evidence re-audit (run separately from timing so hashing does not
contaminate the benchmark):

```powershell
node tests/audit_hdrvdp3_delivery.cjs $FrozenManifest $AcceptanceDirectory $SourceSnapshot fresh-audit.json
```

This verifies frozen files/reference tree, all snapshot sources, all219saved
frame score pairs,438PNG hashes and decode-source provenance, recorded GPU
dispatches/validation logs, current source media hashes and independently
recomputes the12ordered Float32 means. It does not rerun the metric.

Final audit PASSED after timing:29frozen file hashes,155snapshot files,16current
source MP4 hashes,438PNG hashes,219score pairs and12recomputed means verified;
6351recorded GPU dispatches, zero reported validation errors. The official tree
fingerprint remains unchanged. Receipt: benchmark output/final_evidence_audit.json.

## Reproducible source build and runtime

See [clean build instructions](HDRVDP3_CLEAN_DELIVERY_2026-11-11.md) for exact
commands, versions, dependencies and preserved receipts. In a developer PowerShell,
from the repo root, choose NEW destination paths:

```powershell
node tools/snapshot_hdrvdp3_delivery.cjs . fresh-source
cmake -S fresh-source -B fresh-build -G "Visual Studio 18 2026" -A x64 -DIQM_HDRVDP3_DELIVERY_ONLY=ON
cmake --build fresh-build --config Release --target IQM
node tools/package_hdrvdp3_delivery.cjs fresh-build $Official $Fftw fresh-package
```

`$Official` is the unmodified official3.0.7 root; `$Fftw` is the pinned Single
DLL, not an undocumented validation file. Package tool verifies model hashes.
Build requires Windows x64, CMake>=3.29, MSVC C++20, Vulkan SDK/glslc/spirv-val,
Node and repo stb headers. Tested MSVC19.51.36260, CMake4.4.4, SDK1.4.363.0.
Six shaders are built with glslc Vulkan1.2/-O0 then spirv-val; never use the
destructive compile_shaders.sh for this delivery.

Runtime package: IQM.exe, six SPIR-V shaders, four pinned spectral/model files,
Single FFTW DLL plus libwinpthread, notices and relative-path launcher. External
requirements: Windows x64/UCRT/MSVC runtime, Vulkan loader/driver, Float64 GPU and
installed Khronos validation layer for the validated runs. Octave11.3.0 and the
official source are reference-only dependencies; FFmpeg9.0.2 is video-extraction
only. No Octave/FFmpeg/source tree is needed to evaluate prepared PNG inputs.
FFTW SHA256: `2dda2a429ad7a9088260c21e0e2e5fd13ff66771832f82d4da3e836b2875b55e`.
Do not redistribute datasets/reference assets/DLLs without retaining applicable
license terms; the package already contains runtime notices.

To regenerate synthetic fixtures/goldens from sources instead of relying on a
historical local `$Cases` manifest, use absolute NEW fixture/golden paths and
the existing unmodified official scorer. This is a reproduction recipe, not
another test sweep launched during benchmarking:

```powershell
$Fixtures = [IO.Path]::GetFullPath('fresh-fixtures')
$Goldens = [IO.Path]::GetFullPath('fresh-goldens')
& $Octave --quiet tests/generate_hdrvdp3_profile_fixtures.m $Fixtures
if ($LASTEXITCODE) { throw 'Fixture generation failed' }
New-Item -ItemType Directory -Path $Goldens
$fixtureManifest = Get-Content (Join-Path $Fixtures 'fixtures.json') -Raw | ConvertFrom-Json
foreach ($case in $fixtureManifest.cases) {
  $destination = Join-Path $Goldens $case.name
  $ppd = $case.ppd.ToString('G17', [Globalization.CultureInfo]::InvariantCulture)
  & $Octave --quiet tests/score_hdrvdp3_compact_reference.m $Official $case.reference $case.test $destination $ppd
  if ($LASTEXITCODE) { throw "Official scoring failed: $($case.name)" }
  $scoreFile = Join-Path $destination 'score.json'
  $case | Add-Member -NotePropertyName stock -NotePropertyValue $scoreFile
  $case | Add-Member -NotePropertyName stock_sha256 -NotePropertyValue ((Get-FileHash $scoreFile -Algorithm SHA256).Hash.ToLowerInvariant())
}
$Cases = Join-Path $Goldens 'cases.json'
[IO.File]::WriteAllText($Cases, ($fixtureManifest | ConvertTo-Json -Depth 20), [Text.UTF8Encoding]::new($false))
node tools/package_hdrvdp3_regression.cjs $Cases fresh-regression
node tests/verify_hdrvdp3_clean_delivery.cjs fresh-package/IQM.exe fresh-package/runtime fresh-package/fftw/libfftw3f-3.dll fresh-regression/cases.json $Official $Octave fresh-synthetic-results
```

```powershell
.\fresh-package\IQM.exe --method HDRVDP3 --hdrvdp3-profile live-hybrid --hdrvdp3-encoding pq-bt2020-rgb16 --input test.png --ref reference.png --hdrvdp3-runtime fresh-package/runtime --hdrvdp3-fftw fresh-package/fftw/libfftw3f-3.dll --hdrvdp3-ppd 62.729130063099447 --hdrvdp3-export fresh-output --enable-validation
```

### Regenerate the daylight0 smoke manifest

The smoke helper needs its own `daylight0` manifest; the synthetic manifest above
does not contain LIVE media. Set `$LiveSamples` to the directory holding the
public LIVE MP4 files, `$Official` to the unmodified v3.0.7 root, and `$Octave`
to octave-cli.exe. Put the pinned FFmpeg9.0.2 ffmpeg/ffprobe on PATH. Run from the
repository root with NEW output paths. No historical local manifest is needed:

```powershell
$Smoke = [IO.Path]::GetFullPath('fresh-daylight0')
New-Item -ItemType Directory -Path $Smoke -ErrorAction Stop
$Decoded = Join-Path $Smoke 'decoded'
node tests/decode_live_hdr_frame.cjs (Join-Path $LiveSamples '4k_ref_Daylight1.mp4') (Join-Path $LiveSamples '4k_3M_Daylight1.mp4') 0 $Decoded
if ($LASTEXITCODE) { throw 'LIVE frame decode failed' }
$Reference = Join-Path $Decoded 'reference.png'
$Test = Join-Path $Decoded 'test.png'
$Stock = Join-Path $Smoke 'official'
& $Octave --quiet tests/score_hdrvdp3_compact_reference.m $Official $Reference $Test $Stock '62.729130063099447'
if ($LASTEXITCODE) { throw 'Official smoke scoring failed' }
$Score = Join-Path $Stock 'score.json'
$case = [ordered]@{
  name = 'daylight0'; ppd = 62.729130063099447
  reference = $Reference; test = $Test; stock = $Score
  reference_sha256 = (Get-FileHash $Reference -Algorithm SHA256).Hash.ToLowerInvariant()
  test_sha256 = (Get-FileHash $Test -Algorithm SHA256).Hash.ToLowerInvariant()
  stock_sha256 = (Get-FileHash $Score -Algorithm SHA256).Hash.ToLowerInvariant()
}
$SmokeCases = Join-Path $Smoke 'cases.json'
$manifest = @{ cases = @($case) } | ConvertTo-Json -Depth 8
[IO.File]::WriteAllText($SmokeCases, $manifest, [Text.UTF8Encoding]::new($false))
node tests/verify_hdrvdp3_delivery_smoke.cjs fresh-package/IQM.exe fresh-package/runtime fresh-package/fftw/libfftw3f-3.dll $SmokeCases $Official $Octave (Join-Path $Smoke 'iqm-smoke')
if ($LASTEXITCODE) { throw 'Smoke comparison failed' }
```

The helper verifies hashes, compares final IQM with these freshly generated
official scores and performs another fresh official comparison. This recipe
adds no algorithm/preprocessing changes and was reviewed against the helper's
required fields; it was not used to rerun or replace acceptance measurements.

This build compiles the actual main CLI but intentionally includes ONLY the
live-hybrid profile. The default multi-metric build path remains present; its
clean rebuild is not part of this validated delivery. Numerical reproducibility
was tested on this host, not a bare second machine or byte-identical PE rebuild.

The acceptance freezer is deliberately write-once: it refuses to overwrite the
dated selection document. For a future NEW acceptance run, create a new replay
workspace with a `tests/` directory and copy these existing helpers there:
`freeze_hdrvdp3_live_acceptance.cjs`, `run_hdrvdp3_delivery_acceptance.cjs`,
`decode_live_hdr_frame.cjs`, `score_hdrvdp3_compact_reference.m`,
`live_hdr_clock_grid.cjs`, `live_hdr_frame_reuse.cjs`, plus the historical
`LIVE_HDR_FOOTBALL150_INVESTIGATION.md`. Do not copy/overwrite the selection
document. Run the freezer from that workspace with explicit dataset/package/
official/Octave paths and `IQM_FFMPEG`/`IQM_FFPROBE` executable paths, then run
the acceptance script with its new manifest and fresh results path. This creates
a NEW selection document and machine-local manifest rather than changing the
accepted manifest. Do not launch another acceptance run during benchmarking.

## Benchmark protocol

CPU baseline is the SAME shared numerical source from the accepted source
snapshot with Vulkan backend macros absent, compiled Release with /fp:strict,
/EHsc using the same MSVC. It is NOT MATLAB/Octave timing. The final packaged
IQM.exe is unchanged. CPU reads the same package model/FFTW and PNGs.
Build baseline without touching existing outputs:

```powershell
cmake -S tools/hdrvdp3_cpu_baseline -B fresh-cpu-build -G "Visual Studio 18 2026" -A x64 "-DIQM_SOURCE=$SourceSnapshot"
cmake --build fresh-cpu-build --config Release --target IQM-hdrvdp3-cpu-baseline
node tests/benchmark_hdrvdp3_delivery.cjs $FrozenManifest fresh-cpu-build/Release/IQM-hdrvdp3-cpu-baseline.exe $AcceptedFrameDirectory fresh-benchmark
```

Preselected input: Daylight1,4k_3M,frame0 with4K reference (same accepted PNGs).
One warmup/backend, three measured/backend, all sequential. Order CPU,hybrid
warmups, then hybrid/CPU, CPU/hybrid, hybrid/CPU. All eight outputs must exactly
match accepted reference Q=40826c85 and Q_JOD=409d6d04. Warmups excluded from stats.
Validation is enabled in the shipped hybrid profile; CPU has no Vulkan setup.
This measures delivered configurations including validation overhead, not an
isolated GPU vs CPU kernel comparison. Host: Intel Core Ultra7 165H,16cores/22logical,
NVIDIA RTX A500 Laptop GPU, driver595.95, corporate Balanced plan unchanged.
Clocks, thermal conditions and unrelated OS activity are not locked; single-input
results with three repeats do not establish broad performance superiority.

Whole-process timing includes startup/model/PNG processing/algorithm, synchronous
transfers and waits, output and teardown. MP4 extraction and official scoring are
excluded. Internal elapsed starts AFTER Vulkan/FFTW setup/model loading and ends
before score export. Their difference is NOT transfer overhead; transfer/waits
occur inside the internal timer. No Vulkan timestamp queries exist in the validated
binary, so kernel, transfer and synchronization durations are unavailable separately.

Benchmark completed, three measured runs/backend (warmups excluded):

|Measurement (seconds)|CPU-only baseline|Final IQM live-hybrid|
|---|---:|---:|
|Whole-process measured runs|76.383 / 78.360 / 66.335|99.803 / 103.499 / 104.966|
|Whole-process median|76.383|103.499|
|Whole-process minimum..maximum|66.335..78.360|99.803..104.966|
|Internal elapsed measured runs|76.184 / 78.233 / 66.195|98.210 / 100.069 / 102.870|
|Internal elapsed median|76.184|100.069|
|Process minus internal, per run|0.199 / 0.127 / 0.140|1.593 / 3.430 / 2.096|
|GPU kernel / transfer / synchronization separately|Not applicable / unavailable|Unavailable|

CPU/hybrid median wall ratio0.7380. Hybrid median took35.5% LONGER than CPU;
there is no demonstrated GPU speedup. This is a descriptive ratio on one input,
not a statistical significance claim or dataset-wide slowdown estimate. Variation,
especially the last CPU run, remains visible; no slow/fast sample was discarded.
Separate backend medians are not a paired-kernel speed measurement. Do not
interpret process-minus-internal as upload/download cost or subtract these
values to invent a GPU kernel time.

All8runs (2warmups+6timed) EXACT Q=40826c85/Q_JOD=409d6d04. All4hybrid runs
record29GPU dispatches each and zero reported Vulkan validation errors; all4CPU
runs record zero dispatches. CPU baseline SHA256:
`430e8e97f01d3014d97c488c3ec15b40182208fa37f0c8ffe333824613b60e5c`.
All17package hashes and input hashes were verified before/between/after runs.
All155working-tree delivery source files also match the accepted source snapshot.

Generated evidence (not for Git):
`validation/hdrvdp3_delivery_benchmark_20261112_run01/` contains protocol.json
(all actual commands and hashes), results.json, per-run score/dispatch/logs,
hardware_conditions.txt, compiler_settings.json, baseline_build_receipt.json,
source_comparison.json and the final independent acceptance evidence audit.
No concurrent metric/reference jobs were launched during timing; OS/background
load, AC status and dynamic clocks were not controlled. The unchanged power plan
and sampled hardware state are recorded. A validation-disabled speed comparison
would require a separately reviewed configuration; it was not fabricated here.

## Preserved limitations and stopping point

- Light3 alignment remains OPEN:367header vs362default decoded frames; ignoring
  edit lists adds leading frames and is not an established alignment fix.
- Original football540p frame150 official40ec4f50/41037c75 conflicts with later
  official40ec7071/41038aea. Frame150 was INCLUDED in this accepted run; fresh
  parity does not explain or resolve the original reference inconsistency.
- No subjective correlation study, all279distorted videos, private competition
  reproduction, universal domain coverage, full GPU or cross-vendor parity claim.
- CPU FFT/JND/pyramid/remaining quality and synchronous host transfers remain by
  approved scope. No new FFT kernels or unrelated GPU work.

The stopping point is reviewed hybrid delivery with bounded exact acceptance,
honest benchmark and reproducible build/runtime, leaving the above issues explicit.
Commit candidates and exclusions: [commit review](HDRVDP3_DELIVERY_COMMIT_REVIEW_2026-11-12.md).
