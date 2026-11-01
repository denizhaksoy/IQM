# Validated mixed-dimension quality runtime and CLI

The public `evaluate_quality` API accepts80x96/96x80 positive Float32 luminance
at30ppd/age24 when `EvaluationOptions.pathwayRuntimeDirectory` names a version1
package. The CLI selects that route for these dimensions through the existing
`--hdrvdp3-runtime` option. The profile uses fixed D65/sp0/ABI13 and intentionally
rejects spectral overrides. Existing32/64/128/64x128/128x64 routes retain their
previous assets and viewing-condition contracts.

`package_pathway_runtime.ps1` creates a fresh package from an explicitly supplied
validated asset manifest. It copies16 SPIR-V payloads (including both mixed FFT
orientations),four fixed model tables,licenses and provenance. It does not
compile shaders or copy images/native image-dependent intermediates.

`load_pathway_runtime` requires all20 unique known asset keys,the version1
fixed profile,relative paths confined to the canonical package root,SHA256
content agreement,valid SPIR-V headers and finite sp0 model words of the exact
required sizes. SHA256 agrees with independent PowerShell digests on package
payloads. Twelve mutation checks reject malformed versions/profiles/missing or
duplicate keys,unknown keys,path escapes,absolute paths,changed hashes/bytes,
invalid SPIR-V,wrong model shapes and nonfinite model words,without creating a Vulkan instance.
The supplied asset manifest describes compatible previously validated kernels;
content checks do not independently prove arbitrary replacement shader parity.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/package_pathway_runtime.ps1 -AssetsManifest <validated-assets.json> -OutputDirectory <fresh-runtime>
cmake -S tests/hdrvdp3_evaluation -B <fresh-build> -G 'Visual Studio 18 2026' -A x64 -DIQM_HDRVDP3_BUILD_APPLICATION=ON
cmake --build <fresh-build> --config Release --target IQM-hdrvdp3 IQM-pathway-runtime-test -j4
& <fresh-build>/Release/IQM-hdrvdp3.exe --method HDRVDP3 --input <test.pfm> --ref <reference.pfm> --hdrvdp3-runtime <fresh-runtime> --hdrvdp3-export <fresh-export>
```

Checkpoint: `validation/pathway_cli_639285285317479567`,Intel Arc Pro Graphics.
`runtime_contract_final/PASS.txt` verifies loading/contracts. `cli/PASS.txt` verifies
six PFM pairs against independent native final score bits and original input,
P andlog adaptation bits. PFM fixtures serialize existing native original
Float32 pixels bottom-to-top with scale-1;exported input bytes verify no scaling
or orientation change. Three viewing/spectral overrides fail before exports.
Fresh exports include `quality.txt`,`scores.f32`,original input buffers,P/log,
input provenance andprofile metadata. They differ from the historical legacy
107-export diagnostic layout;tests explicitly check the applicable route.

The API still uses CPU model/LUT preparation,double NCSF and synchronous host
transfers. General dimensions,RGB/real HDR datasets,other viewing parameters,
other devices and additional perceptual outputs remain outside this acceptance.

Legacy regression also passes all11 existing PFM pairs,1166 numerical exports
and exact quality files (`legacy_regression/PASS.txt`). The original numerical
executable is preserved as `pre_help/IQM-hdrvdp3.exe`;the final build changes
only help text and is checked separately. Historical hash paths map to this
preserved executable in binary_lineage.json. No shader/model assets changed.

The final help-only build passes `--help` and an additional asymmetric80x96 PFM
smoke:original input,P/log,quality text andscore buffers match the fully tested
binary byte-for-byte. The copied validated package is ready to use locally:

```powershell
& './validation/pathway_cli_639285285317479567/build/Release/IQM-hdrvdp3.exe' --method HDRVDP3 --input './validation/pathway_cli_639285285317479567/cli/inputs/asymmetric_h80_w96_test.pfm' --ref './validation/pathway_cli_639285285317479567/cli/inputs/asymmetric_h80_w96_ref.pfm' --hdrvdp3-runtime './validation/pathway_cli_639285285317479567/runtime' --hdrvdp3-export './validation/my_pathway_cli_run'
```

Use a fresh export folder for each run. This example uses retained local test
images/package;generated data andbinaries are not included in Git. A source-only
asset generation/reproduction driver remains a separate improvement.

A source-only runtime driver is now available:
`generate_pathway_runtime.ps1 -OutputDirectory <fresh> -VulkanBin <SDK/Bin>`.
It regenerates all20payloads+manifest byte-for-byte without previous exports or
native installation. See SOURCE_RUNTIME_REPRODUCTION.md. Six additional same-
image pairs now also passCLI andactualAPI trace checks,including the native
zero-frequency/+Infnoise limit;see PATHWAY_IDENTITY_VALIDATION.md. These use
the same three contents/two orientations,not a broader viewing/device domain.
