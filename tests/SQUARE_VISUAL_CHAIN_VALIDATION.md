# Experimental96x96 connected visual stages

Checkpoint: `validation/square_filter_639286168390035328` (2026-10-26).
This connects the isolated192x192 FFT stages to original-image optical and
adaptation processing. Public96x96 quality support remains disabled.

The diagnostic compiles `mixed_filter.cpp` with
`IQM_HDRVDP3_SQUARE_FILTER_DIAGNOSTIC`. That build exposes a separately named
`run_square_filter_diagnostic`, accepts96x96 and uses explicit new192x192 FFT
assets. Normal builds retain `run_mixed_filter` and the old80x96/96x80 guard.
The production regression binary independently verifies that96x96 throws
before assets are loaded. No manifest, accepted shader payload or public
geometry guard was widened.

The chain starts from original positive Float32 reference/test images, computes
MTF preparation, forward FFT, model filter product, normalized inverse and
retinal crop, then computes L+M on the GPU and feeds that actual result into
GPU local adaptation. All model tables are generated from existing source,
not read from image-dependent goldens. Expected stages are loaded only after
the numerical calls finish. Host copies occur at synchronous API boundaries
between MTF, L+M and adaptation; image-dependent arithmetic remains on the GPU.
Each individual filter call shares buffers across its compute dispatches.
This does not claim a fully device-resident pipeline.

Native configuration remains official HDR-VDP-3 v3.0.7,30ppd,age24,D65/sp0,
Float32 images/transforms, native double-filter promotion, FFTW three threads
and estimate planning,192x192 padding, MTF symmetric post-padding and LA
replicate post-padding after log. DC is retained; inverse normalization is one
division by36864. Diagnostic prepared/filtered complex traces are column-major;
forward/inverse traces are row-major; image maps are row-major.

## Content and representation checks

The six pairs are asymmetric, impulse, synthetic HDR ramp, constant10cd/m²,
an asymmetric identity pair, and a photograph-derived grayscale pair. The
first four use native localized gain1.4 plus offset0.25; identity has no
distortion. The photograph uses localized gain1.01 and offset0. It is a96x96
center crop of the same locally retained Memorial Church RGBE source described
in `PATHWAY_LUMINANCE_AND_HDR_VALIDATION.md`, with the same arithmetic RGB mean,
whole-source peak assigned1000cd/m², floor0.0001 and one Float32 cast. Crop
position(208,336), source512x768, range0.00877256–1.29148cd/m², no clamped
pixels. This is assigned grayscale display luminance, not RGB or recovered
physical scene-luminance validation. Raw photograph data remains outside Git.

The first asymmetric connected run passed. The constant reference exposed
18,240 imaginary signed-zero differences in the raw forward FFT, with no
numeric differences. The native Octave result narrows entirely real complex
arrays to real matrices. A diagnostic-only IO mode now applies that rule after
forward FFT, filter product and inverse FFT: one invocation checks all imaginary
components, then writes +0 only when the complete result is real. This is an
in-place GPU representation operation with a barrier, not a tolerance change
or per-component unconditional zero normalization. Existing IO generator and
production runtime payloads remain untouched. The new filter has ten dispatches
per MTF/LA call, plus the separate GPU L+M operation between them.

`acceptance` contains six pairs ×26 checks =156 stage checks /8,294,400 words
bitwise, including signed zeros, without Vulkan errors. Each image checks both
model tables, prepared arrays, forward spectra, filtered spectra, inverse arrays,
retinal/adaptation outputs and the actual GPU L+M input. The first failure and
its pre-fix executable remain retained. The accepted26-stage executable was
frozen as `square-filter-chain-MTF-LA-accepted.exe` before the next extension.

## Pupil, P and log-adaptation extension

The optional visual mode uses those actual GPU adaptation and retinal outputs
for pupil reduction and JND response, then computes pairwise mean adaptation
and log adaptation on the GPU. The existing CPU JND lookup table is static,
image-independent model preparation. Host extraction of response fields copies
Float32 values without new arithmetic. `export_square_visual_reference.m`
requires native pupil/P/mean/log goldens to be Float32 and preserves their bits.
The reference P is also checked against the visual trace's own final P.

This adds four pupil values and one P map per image plus mean/log maps per pair.
The resulting contract is32 stages /1,419,272 words per pair. This extends the
same six pairs; its MTF/LA values overlap the26-stage cohort. It is not six
additional independent image pairs and does not include SPYR, masking or final
quality scores.

Final `visual_acceptance` result: all six pairs pass192 stage checks /
8,515,632 Float32 words bitwise, including signed zeros. Pupil, P, mean and
log-adaptation are checked against native values, and Vulkan reports no errors.

Build target `square-filter-chain-test` in `tests/hdrvdp3_residual144`. The
new IO shader is produced by `compile_square_filter_io.ps1` under
`tests/hdrvdp3_dimensions`; forward/inverse use the accepted square FFT assets
and MapGPU uses the pinned ABI13 map shader. The durable validator reads six
named native roots from a JSON cases file:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_evaluation/validate_square_filter.ps1 -Executable <build>/Release/square-filter-chain-test.exe -ForwardShader <forward192square.spv> -InverseShader <inverse192square.spv> -IOShader <square_filter_io.spv> -MapShader <map.spv> -CasesFile <cases_visual.json> -OutputDirectory <fresh-results> -Visual
```

Without `-Visual`, the validator audits the26-stage filter contract and needs
only native filter roots. `-Visual` additionally requires a `visual` path per
case with the exported P/pupil/mean/log goldens. `-ResultsOnly` audits retained
outputs. The numerical execution script initially omitted names in progress
lines because of PowerShell variable/colon interpolation; only that message
was fixed. Its original execution snapshot and hashes are preserved separately.

Regressions: production80x96 and96x80 MTF/LA retain exact stage bits, including
the prior80x96 repeated/nontrace check. The production96x96 negative guard
passes. The HDR preparation helper's new `--square96` flag leaves all four
original80x96/96x80 preparation artifacts byte-identical. Old references,
staged shader changes and dirty VkFFT state remain preserved.

## Connected steerable pyramid extension

The optional SPYR mode feeds both actual GPU P maps to a separately named
`run_square_spyr_diagnostic`. Its build definition selects96x96/144-element
base geometry; normal compilation retains the existing80x96/96x80 function
and120-element base. Buffer lengths are290(mean/DC),288(complex difference)
and433(raw FFT/magnitude/index). Difference packing dispatches three64-wide
groups so all144 elements are covered; the old120-element mode still uses two.
The actual GPU base is inspected for the proven mean exponent domain before
mean processing, preserving the existing runtime range check.

All filter coefficients come from `create_sp0_model`, not native files. The
existing generic FMA/separate SPYR kernels are reused, followed by accepted
mean144/difference144/residual144 assets. There are no CPU image-dependent
arithmetic stages. Native replay goldens from `export_square_spyr_reference.m`
independently assert all four raw lowpass maps, high/orientation bands, centered
base band, FFT/DC and first native maximum against the full official trace.
Complex exporters extract whole real/imaginary arrays before indexing, retaining
the signed-zero correction established in the preceding FFT work.

The extension checks11 additional output buffers,68,115 words per pair. The
complete diagnostic contract becomes43 stages /1,487,387 words per pair. Use
the durable validator's `-SPYR` option, explicit five SPYR shader arguments
(`-SPYRFMA`, `-SPYRSeparate`, `-MeanShader`, `-DifferenceShader`, `-ResidualShader`)
and a `spyr` golden-directory path per case. The option includes visual mode.
This extends the same six original image pairs; earlier stage counts overlap.
Masking, CSF/base-noise integration, comparison resizing and final Q/Q_JOD
remain outside this connected acceptance.

Final `spyr_acceptance` result: all six pairs pass258 stage checks /
8,924,322 Float32 words bitwise, including signed zeros, without Vulkan errors.
The original SPYR caller also passes its three-content/two-orientation matrix
with historical acceptance criteria unchanged (`production_spyr_regression`).
The downstream image-to-score checkpoint is documented in
[SQUARE_QUALITY_VALIDATION.md](SQUARE_QUALITY_VALIDATION.md).
