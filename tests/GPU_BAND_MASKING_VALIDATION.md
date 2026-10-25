# GPU-derived bands, pooling and CSF in quality composition

Checkpoint `validation/gpu_band_masking_639284552580517314`,2026-10-24,
Intel Arc Pro Graphics. This is a file/host-composed experimental pipeline;
public support/guards and production algorithms remain unchanged.

`prepare_gpu_band_pool_inputs.m` reads the accepted all-chain GPU dumps from
`pathway_spyr_all_chain_639284285430220524/gpu_all_chain`. Highpass/orientation
bands are stages4..7, paired reference then test, already normalized. The base
uses stage8's centered words, skipping each mean without arithmetic. It writes
binary Float32 bands and a pooling input fixture. Native bands/raw masks/pooled
masks are validation goldens only: runtime reference/test fields come from GPU
words. Native reference/test band bit identity is asserted before serialization.
No host filtering, normalization, DC subtraction or runtime mask generation.

The unchanged experimental pool checker and accepted integer-FMA shaders from
`pathway_resize_csf_639284286981781158` compute raw/pooled masks from those GPU
bands. `gpu_pool/PASS.txt`:10bands,71520 outputs numerically exact; observed bit
differences zero. This repeats the same actual input coverage, not new images.

The extended masking checker loads all five GPU reference/test band pairs and
the pooled half of each GPU pooling dump. It validates all three against native
goldens, then uses the GPU values for band difference, self/neighbor masks and
N_mask. Adjacent masks also come from the GPU pool cache. GPU comparison masks
are retained from the previous general comparison checkpoint. Native band/self
data are not fallback runtime inputs in this profile.

`gpu_masking/PASS.txt`:112 checks,357614 Float32/double comparison values
numerically exact, observed bit differences zero; Q/Q_JOD match both native
pairs. The executable/source for that profile is preserved as
`band_self_only.exe`/`band_self_only_source.cpp`.

The final optional log-adaptation directory additionally replaces CSF runtime
maps with GPU resize ->clamp ->sample_csf for all five band sizes, using the
existing native log-adaptation input and CPU-generated standard LUT coefficients.
Native CSF files are only comparison goldens. `gpu_masking_CSF/PASS.txt`:
**122 checks,393374 values numerically exact**, observed bit differences zero.
The ten extra checks compare35760 GPU CSF values. The full run includes the
prior112 checks, so no unchanged additional suite was repeated. GPU mask doubles
still require bit identity separately. Other stage acceptance is numerical.

Both actual80x96/96x80 pairs use30ppd,age24,sp0Filters. Row-major Float32 files
retain native /1,/2,/4,/8,/16 normalization and separately centered base bands.
Neighbor/CSF resize uses native imresize symmetric extension. Pooling uses zero
edges. No additional FFT padding, normalization or DC processing is introduced.
The unchanged bounded ABI13 map shader performs masking, resize and CSF; no new
shader was compiled in this checkpoint. Fresh strict MSVC Release checker and
all linked source/dependency hashes are recorded.

```powershell
octave-cli --quiet --no-gui tests/prepare_gpu_band_pool_inputs.m <fresh GPU-input directory> <all-chain GPU dumps> <full native masking trace>
pathway-pool-test <accepted pool first.spv> <accepted pool second.spv> <GPU inputs>/pool.txt <fresh GPU pool>
pathway-masking-test <bounded ABI13 map.spv> <native masking fixtures> <fresh output> <D_p.spv> <GPU comparison dumps> <GPU inputs> <GPU pool> <native log-adaptation trace>
```

Remaining native runtime inputs are upstream P/log-adaptation maps, the base
neural-noise scalar and fixed model parameters/LUT coefficients. GPU P production
and base-noise derivation still need integration. Components exchange data through
files/synchronous API host transfers, not one resident image-to-score command
buffer. Matching scores here must not be presented as full metric completion.
