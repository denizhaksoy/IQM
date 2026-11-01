# HDR-VDP-3 demonstration checkpoint

This is a reproducible implementation demonstration for a supervisor, with
explicit validation boundaries. It is not a claim of complete HDR-VDP-3 API
coverage, all-input reference parity or a fully GPU implementation.

2026-10-31 accepted local package:
`validation/professor_demo_639290563942725365/package`.
`HDRVDP3-Demo.zip` and the independently checked `relocated-results/report.html`
are in the same checkpoint. Completed pointer:
`validation/professor_demo_completed_path.txt`.

All four cases passed twice, including replay from a copied package in a
different working directory with spaces in its path. Per run, 32 quality
values and 16 binary exports matched; 87 package files were integrity-checked.
These are repeated existing fixtures, not new metric validation coverage.
The first run's preview images were corrected after numerical execution:
PowerShell had selected an integer Math.Min/Max overload. Original previews
are preserved, and metric inputs/results were unchanged. The relocated replay
used the corrected package throughout. An earlier JSON rounding failure is
also preserved; expected scores now retain original 17-digit text strings.

## Build and run the local package

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/hdrvdp3/Build-Demo.ps1 -OutputDirectory validation/my_fresh_demo
powershell -NoProfile -ExecutionPolicy Bypass -File validation/my_fresh_demo/Run-Demo.ps1 -OpenReport
```

Building the package uses the completed local validation checkpoints and the
frozen tested executable. It does not recompile the implementation or download
images. If these checkpoints are absent on a different checkout, the builder
fails rather than substituting unverified fixtures. A prepared package runs
without the original repository, compiler, reference installation or internet.
Vulkan driver and Float64 GPU support are required; tested on RTX A500.

The four included pairs are:

| Example | Input interpretation | Expected Q | Expected Q_JOD |
|---|---|---:|---:|
| sRGB stress | Encoded sRGB, OLED display model | 5.1837547121689243 | 6.1033661559446406 |
| sRGB ramps | Encoded sRGB, OLED display model | 9.8450571153464637 | 9.9523073729608242 |
| sRGB identity | Same encoded input twice | 10 | 10 |
| HDR photograph crop | Linear native-display RGB, OLED spectrum | 9.7738552093505859 | 9.922581672668457 |

The report includes reference/test previews, measured scores and exact reference
checks. It compares all eight quality values and all expected binary exports,
then stops on a mismatch. Each run writes a fresh folder, logs and commands.
Elapsed time includes startup and exports and is not a GPU performance benchmark.

The HDR example comes from the previously validated Memorial Church photograph
crop with local color changes. It is scaled to virtual display units and
interpreted in native display primaries. It is not absolute capture calibration.
Preview tone mapping is separate from metric inputs. Source/provenance details
remain in [RGB content validation](RGB_CONTENT_VALIDATION.md).

## What can be demonstrated

The normal IQM executable evaluates actual PFM image files and produces Q and
Q_JOD. The sRGB path uses the validated v10 runtime with GPU JND logarithms;
the native HDR example uses the earlier validated native RGB runtime. Both are
hybrid CPU/GPU paths. Runtime payload integrity, explicit encoding and viewing
parameters prevent silently treating unsupported inputs as validated profiles.

The demonstration is limited to 96x96, 30 ppd, age 24, quality/default sp0 and
OLED. Broader historical tests include three display spectra, but this package
intentionally presents a compact four-case demonstration at one display.
Its reruns reuse existing reference data and do not expand numerical coverage.

Remaining completion work is separate from this demonstrable checkpoint:
arbitrary dimensions and viewing conditions, wider independent image coverage,
other prediction outputs, hardware portability and the remaining CPU operations.
These must not be represented as already validated. CPU processing alone does
not imply a numerical mismatch.

See [v10 validation](DOUBLE96_GPU_LOG_VALIDATION.md) for stage counts and
[source-order logarithms](WINLIBM_DOUBLE_LOG_VALIDATION.md) for arithmetic scope.
