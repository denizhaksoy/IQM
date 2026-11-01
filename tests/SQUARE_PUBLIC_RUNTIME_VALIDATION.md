# Public square96 runtime acceptance

Checkpoint: 2026-10-27, Europe/Prague. These results promote the previously
private [square quality diagnostic](SQUARE_QUALITY_VALIDATION.md) into the
production API and CLI. They do not establish general HDR-VDP-3 support.

Evidence root: `validation/square_public_639286351443539038` (local artifacts,
not distributed in Git). Official reference: HDR-VDP-3 v3.0.7.

## Accepted profiles

| Manifest version | Profile | Supported image geometry | Payloads |
| --- | --- | --- | --- |
| 1 | luminance-30ppd-age24-D65-sp0-map13 | 80x96 and 96x80 | 20 |
| 2 | luminance-30ppd-age24-D65-sp0-map13-square96 | 96x96 | 18 |

Both require positive Float32 luminance inputs, 30 pixels/degree, observer age
24, fixed D65, and no spectral overrides. Version 2 contains 14 shaders and
four model tables; its 192x192 padded FFT, whole-real filter narrowing and
144-element base processing must not be replaced with version 1 assets.
The runtime loader binds geometry to the manifest version/profile.

## Measured results

- Public quality API: six pairs (asymmetric, impulse, HDR ramp, constant,
  identity and photograph-derived grayscale), 69 checks and 285,277 values per
  pair. Total: 414 checks / 1,711,662 Float32/Float64 values, bitwise exact.
- Public visual/SPYR chain: the same six pairs, 43 checks per pair, 258 checks
  / 8,924,322 Float32 words, bitwise exact. This overlaps the quality cohort;
  it is not another six independent image pairs.
- CLI: six PFM pairs, seven exports each, matching original input, native and
  accepted API bits. PFM header/orientation readback and quality text values
  are checked. No reported Vulkan validation errors.
- Version 1 API: both old asymmetric_tiny orientations, 69 checks / 237,733
  values each, bitwise exact. Version 1 CLI: six accepted pairs preserved.
- Legacy CLI: eleven earlier pairs preserve 1,166 numerical exports and
  exact quality files, including square32/64/128 and rectangular64x128/128x64.
- Loader contracts: valid packages accepted and twelve malformed packages
  rejected for each version. Wrong profile/geometry and unsupported viewing
  settings rejected in the recorded API/CLI checks.

See `api_acceptance`, `public_visual`, `cli_acceptance`, `v1_api_h80_w96`,
`v1_api_h96_w80`, `v1_cli`, `legacy_cli`, `contracts_v1`, `contracts_v2`,
`profile_geometry_PASS.txt` and `viewing_rejection_PASS.txt` under the root.

The full-suite executable cohort is preserved in `full_suite_binaries`.
Header-comment changes subsequently caused a relink; the resulting executable
at `build/Release/IQM-hdrvdp3.exe` passed a final seven-export real96 smoke
comparison (`final_cli_smoke`). Do not attribute the entire earlier suite to
that later binary hash.

## Source reproduction

From the repository root, with the documented Windows CMake/MSVC/Vulkan SDK:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/generate_square_runtime.ps1 -OutputDirectory validation/my_square_runtime -VulkanBin C:/VulkanSDK/1.4.363.0/Bin
```

Use the resulting `validation/my_square_runtime/runtime` directory with
`--hdrvdp3-runtime`. The output directory must be fresh. Accepted SHA256 pins
are in `tests/hdrvdp3_evaluation/REFERENCE_SQUARE_PATHWAY_HASHES.json`; a changed
hash requires investigation, not a pin update to make a failure pass.

Both the live tree (`source_build`) and an isolated 598-file source-only tree
(`isolated_verified`) reproduced all 18 pinned payloads. Neither regeneration
uses existing numerical exports or an installed native reference. Both manifest
files match the accepted manifest byte for byte, SHA256
`14517088BBAE266EDB71A249CDF393B3DE6F7A91BDF2F3C6714DB336FC692042`.
Two earlier incomplete isolated copies failed on omitted shader-source
dependencies; their logs remain in `isolated_build` and `isolated_complete`.

## Final CLI routing and provenance

Follow-up: `validation/square_routing_639286693989771436`. Approval review
became available again; the earlier usage-limit blocker is resolved. Any
explicit modern manifest now routes through the modern API. A v2 package with
128x128 inputs fails at the geometry guard before creating an export directory.
The square96 provenance file now correctly reports `v2`, while rectangular
version1 exports retain `v1`.

`final_acceptance/PASS.txt` records the latest production executable preserving
all seven numerical/quality exports for real96 and the old asymmetric96x80
pair, plus correct profile labels and modern128 rejection. Earlier broad-suite
binary lineage remains separate. Reproduce this targeted regression with:

```powershell
node tests/hdrvdp3_evaluation/validate_pathway_routing.cjs <IQM-hdrvdp3.exe> validation/square_public_639286351443539038 <version1-runtime> <fresh-output>
```

## Remaining scope

RGB/color processing, arbitrary dimensions, other viewing conditions, broader
device coverage, and fully GPU-resident processing remain unvalidated or
unfinished. CPU model/LUT preparation, double NCSF and host transfers remain.
The photograph-derived input is a mapped grayscale crop, not RGB acceptance
or a claim of measured scene luminance. Historical private diagnostic results
and older negative square guards retain their historical scope.

The next dimension-preparation component is documented in
[general mean/DC validation](GENERAL_BASE_MEAN_VALIDATION.md). It is an
isolated alternate shader; production runtime v1/v2 payload pins are preserved.
