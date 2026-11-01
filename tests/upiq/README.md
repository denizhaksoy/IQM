# Original-resolution UPIQ parity work

Acceptance target: our Q/Q_JOD predictions against unmodified official
HDR-VDP-3 v3.0.7 predictions on the same verified UPIQ images, with matching
precision, color/photometric representation, viewing conditions and task.
Dataset human JOD/MOS labels are separate statistical targets, not expected
to equal a model prediction exactly.

`dataset.lock.json` pins the April 2021 UPIQ release, DOI
[10.17863/CAM.68372](https://doi.org/10.17863/CAM.68372), including the corrected
`pix_per_deg` metadata. The official DSpace file record supplies archive length
2,520,028,189 bytes and MD5 `b3adf6f25c2b02b86fadca275ee6a517`.
The previous release's metadata must not be substituted silently.

Acquire into a fresh directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/hdrvdp3/Get-UPIQ.ps1 -OutputDirectory validation/my_upiq_download
```

An existing manually downloaded archive can be supplied with `-ArchivePath`;
it is accepted only if length and official MD5 match. SHA256 is also recorded.
HTTP error bodies remain `.partial`, with `downloaded:false,validated:false`.
They cannot become accepted archives or numerical validation evidence.

On 2026-10-31 the official metadata API worked, but the README and ZIP content
endpoints returned HTTP 500, `An internal read or write operation failed`.
No UPIQ archive or image was obtained, and no UPIQ score was computed in this
checkpoint. Metadata and failure responses are preserved in
`validation/upiq_639290576672243959` (also recorded in the active pointer).
This is an upstream content-access failure, not an approval rejection.

## Reference runner

`run_reference_manifest.m` calls the unmodified installed `hdrvdp3` function,
checks version 3.0.7 and function resolution, preserves original dimensions,
and records full-precision score text/hex and source/input SHA256. CPU Octave,
three FFTW threads, estimate planning and fresh wisdom are explicit.

It accepts a JSON manifest with format `IQM_REFERENCE_MANIFEST_1`, an explicit
`dataset_status`, and a `cases` array. Each case requires `id`, `reference`,
`test`, `precision` (`single` or `double`), `encoding`, `ppd`, `observer_age`,
and `rgb_display`. File paths are relative to the manifest. No ppd, display or
precision is inferred from a filename. PNG integer normalization and PFM scale
application are explicit in the runner. It never resizes, crops or tone maps.

EXR input now uses `decode_exr.cpp` with the installed OpenEXR library, built by
`scripts/hdrvdp3/Build-EXR-Decoder.ps1`. The decoder supports single-part,
non-deep FLOAT/HALF channels with explicit RGB or XYZ channel names, retaining
the original data window, signed zero and raw values. It rejects UINT and
subsampled/missing channels. Channel names do not establish color calibration.

`prepare_exr_inputs.cjs` attaches source/decoded SHA256 to a decoded cache and
adds its paths to the reference manifest. The reference runner verifies those
hashes before reading the original float samples, without scale or color
conversion. Differing EXR data/display windows require a further explicit policy
and are rejected by the reference runner. Build/runtime currently depends on
the installed Octave MinGW/OpenEXR prefix; this is not added to the main IQM CLI.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/hdrvdp3/Build-EXR-Decoder.ps1 -OutputDirectory validation/my_exr_build
node tests/upiq/prepare_exr_inputs.cjs manifest.json validation/my_exr_build/decode-exr.exe validation/my_prepared_inputs
```

Put the matching OpenEXR DLL directory on process PATH when running the decoder.
An EXR case requires explicit `source_channels` (`RGB` or `XYZ`). The actual UPIQ
manifest adapter remains unfinished: its README/CSV must be obtained before
selecting encoding and photometric conversion. Do not invent those values or
reinterpret encoded PNG as already-calibrated linear RGB.

Engineering checks passed: 420 decoded words across four RGB/XYZ FLOAT/HALF
controls and 18,915 words for a positive 97x65 control match their source values
exactly. The original-size EXR control runs through the unmodified official
metric with identity scores 10/10. A separate 1024x683 official example also runs
at its original size. These two runs and five comparator contract checks are
**not UPIQ tests or implementation parity acceptance**.

```powershell
octave-cli.exe --no-gui --quiet tests/upiq/run_reference_manifest.m C:/HDR-VDP-3_Ref manifest.json validation/fresh_reference_run
node tests/upiq/compare_predictions.cjs reference_results.json implementation_results.json validation/fresh_comparison
```

The comparison rejects missing/extra cases and mismatched image hashes,
dimensions, precision, encoding, ppd, age or display. It reports bit differences
and ULP differences separately and never upgrades a tolerance pass to exact.
Completing an engineering smoke test is not dataset acceptance.

## Remaining required work

1. Obtain and verify the original archive and corrected viewing metadata.
2. Apply verified EXR ingestion to actual files and add the UPIQ metadata adapter.
3. Generate official full-resolution predictions and trace the benchmark's
   precise settings before claiming agreement with a published report.
4. Apply the [generic Double C++ path](../GENERAL_DOUBLE_QUALITY_VALIDATION.md)
   to the dataset's actual precision/viewing contract, completing any remaining
   unsupported branches before comparing every selected pair without skips.
5. Report coverage, all numerical differences, and only then statistical
   agreement with subjective scores under the documented evaluation protocol.

The earlier demo and millions of small-fixture comparisons do not satisfy this
benchmark requirement. Further GPU-only optimizations are secondary to it.
