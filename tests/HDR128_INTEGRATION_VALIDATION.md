# 128-square quality integration

The local implementation connects FFT256 forward graphs8/9 and inverse3/4
to LocalAdaptFFT, MapGPU ABI14 to256-sample DC/base adaptation and Q20/Q40
comparison-mask resizing, and base FFT ABI82 to16-square residual selection.
ABI13 and the bounded32/64 shader assets retain their separate contracts.
Both SPYR calls and exported band counts use the ppd-limited pyramid height.

Historical integration evidence is preserved in
`validation/map128_api_639281996762386676/`. Three128-square synthetic pairs
(`full128`, `full128_localized`, `full128_hdr_localized`) have six successful
native stage checks each: strict preprocessing, SPYR, CSF, masking, difference
and official quality. Their public API replays compare107 numerical text
exports per pair by hash and require bitwise Q/Q_JOD/pooled-error equality.
See each `*_stage_results.json` and `public_release/results128.json`.
These records predate the2026-10-22 continuation and are not fresh runs.

On2026-10-22 the rebuilt public API reran the localized pair in
`validation/continuation128_639282590525981729/`. All six native stage checks
pass with zero numerical mismatches;107 numerical exports match the previous
public replay by hash. Q=5.024019718170166,Q_JOD=5.937023639678955 and pooled
error=2.635451316833496 pass the API bit checks. No Vulkan errors were found.
This repeats one existing pair. Source/input/binary hashes, native logs and
COMPLETED.json/PASS.txt are preserved there; the completed pointer is
`validation/continuation128_completed_path.txt`.

Two new full-chain inputs on2026-10-22 pass twelve native checks in
`validation/quality128_stress_639282594471943896/`:

| Input | Q | Q_JOD |
| --- | ---: | ---: |
| Three impulses, including both image corners | 6.7128562927246094 | 7.6113557815551758 |
| Competing horizontal/vertical cosines | 0.98083019256591797 | 1.2951202392578125 |

The latter's largest distinct native residual magnitudes are5018.5791015625
and5018.57861328125, one Float32 ULP apart. Native and GPU first-maximum index
is33 (one-based); the impulse case selects32. The difference validator now
asserts the index itself as well as frequency, avoiding equal-radius aliases.
See ranking_audit.json and the per-case base_native_ranking.txt files.
Stage comparisons use exact numeric equality; final Q/Q_JOD use bit checks.
The initial suite driver was extended during execution; final_source_hashes
and the separate post-run ranking audit record these changes. All numerical
validator changes were in place before their corresponding checks executed.
The current reusable driver is tests/run_quality128_stress.ps1; it additionally
checks pinned runtime shaders and explicitly requires the one-ULP fixture.

The separate dimension_frontier subdirectory contains16 official SPYR band
fixtures covering eight rectangular/odd shapes at30/60ppd. It establishes
native geometry only, with no GPU/full-quality parity claim. Notably63x65
requires residual8x9 and80x96 requires10x12; existing square8/16 residual
contracts cannot simply be enabled for these inputs.

Supported128 viewing scope is30ppd/age24, positive finite single-channel
luminance, explicit spectral inputs and pinned reference shaders. The API
rejects other128 viewing parameters. This does not establish general-size,
RGB-native, other-device or universal floating-point parity. Float stage
equality does not imply signed-zero bit identity. Host coefficient/LUT and
spectral preparation remain part of the implementation.

Generate the separate six-shader runtime package using
`tests/hdrvdp3_evaluation/generate_assets128.ps1` in a fresh directory.
`REFERENCE_SHADER_HASHES128.json` pins these assets. The existing
`IQM-quality-api-test` accepts128 inputs; `IQM-quality128-diagnostic` remains
an optional isolated caller. Native stage checks can be reproduced with
`validation/validate_full128_stages.ps1 -Root <fresh-root> -CaseName <case>`
after a successful build and an API export. Official reference files and old
GPU exports must be preserved.
