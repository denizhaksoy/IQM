# CLI integration review ? 2026-11-11

Scope freeze: deliver a validated hybrid CPU/Vulkan profile by November20; begin
the second metric November23. Manual FFTW kernel development is stopped. Existing
experiments, staged shader, dirty VkFFT and validation artifacts remain untouched.
No commit or push. Stop after this CLI integration review before the next task.

## Modified source files

- bin/IQM/main.cpp: route live-hybrid before legacy FileMatcher/Vulkan setup; help.
- bin/IQM/args.h and args.cpp: explicit profile/FFTW argument and profile guards.
- bin/IQM/hdrvdp3_live_quality.h and .cpp: map main CLI options to the shared runner.
- bin/shared/hdrvdp3_single_cli.h and .cpp: existing standalone numerical body.
- bin/IQM-hdrvdp3-cpu/main.cpp: thin standalone entry, preserving four old profiles.
- bin/CMakeLists.txt: compile the runner for main and standalone targets with strict FP.

The numerical preparation/evaluation body is text-identical to its saved original.
No reference algorithm, preprocessing, kernel or model math was rewritten.
Profile/settings/runtime layout: [HDRVDP3_DELIVERY_PROFILE.md](HDRVDP3_DELIVERY_PROFILE.md).
Test launcher: tests/verify_hdrvdp3_live_cli.cjs (all local paths explicit arguments).

## Build and checks

All five targets built: IQM, IQM-hdrvdp3-cpu, IQM-hdrvdp3-hybrid,
IQM-hdrvdp3-mask-experimental and IQM-hdrvdp3-mutual-experimental. Existing cached
MSVC19.51/Vulkan1.4.363 build was used, NOT a clean-checkout build. Initial mask
build hit MSBuild FTK1011 on an old tracking log. Rebuilding the last two targets
with /p:TrackFileAccess=false succeeded; no old build files were deleted.
Existing GLFW-discovery and compiler warnings remain for clean-build follow-up.

Main executable results:14synthetic settings /12distinct input pairs exactly
match preserved independent official Q/Q_JOD bits. The portrait case also matches
a NEW official v3.0.7 Octave execution: Q411fadeb, Q_JOD411ff1c9. Includes portrait,
landscape, odd/small dimensions, resize, identity, PPD variation and deeper bands.
GPU counts:4matrix,1comparison,2adaptation-input,8padding; mutual/resize counts
match band geometry. full_gpu=false. No Vulkan validation errors reported.
Summed process wall time for the14settings:26.129seconds on this run, not a CPU
speedup comparison. Per-case processing and wall times are retained.

Nine CLI checks passed: help, missingFFTW, wrongmethod/encoding/age/PPD,
duplicateprofile, unknownoption and missingshader. All failure cases produce no
quality score. All four rebuilt legacy profiles retain exact portrait Q/Q_JOD
with new GPU options disabled. This is one-case compatibility, not a broad
legacy-profile revalidation. CJS syntax and git diff --check passed.

Evidence: validation/hdrvdp3_cli_integration_20261111_run01/review.json;
numeric run hdrvdp3_cli_numeric_20261111_run01; CLI contract run
hdrvdp3_cli_contract_20261111_run01; legacy run
hdrvdp3_cli_legacy_compatibility_20261111_run01. Source before/after snapshots,
binary hashes, input/runtime hashes, official logs, per-case Vulkan logs,
dispatch counts and times are preserved.

## Not yet accepted

No main-CLI LIVE HDR subset acceptance or clean-build/runtime packaging is claimed.
The current integration gate deliberately tests only synthetic cases; the two
real-HDR entries in the older manifest belong to the later real-data gate.
Light3 alignment and the old football150 reference discrepancy remain unresolved.
No fully-GPU, arbitrary-device or speedup claim. Next review-gated task is the
clean-build/runtime preparation procedure, followed by the predefined LIVE subset.
