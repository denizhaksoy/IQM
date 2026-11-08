#4K FFT scalar-leaf progress - 2026-11-07

The captured4K FFTW plan uses13distinct codelets. These tests validate two scalar
leaves independently; they do not implement orvalidate acomplete4K transform.
OriginalFFTW3.3.10sources/wrappers/COPYING arepreserved in
validation/fftw_4k_sources_20261107_run01 witharchive/member SHA256manifests.
Derivedshaders retainGPL-2.0-or-later attribution andordinaryoperation order.

- r2cf_6:4802real transforms/38416Float32components,zero bitmismatches against
actualFFTW1dR2C6.6batchsizes includingworkgroup tails and4096batch; constant,
impulse,ramp,seededrandommultiple scales. Probeasserts actualplanname r2cf_6.
Artifact validation/r2cf6_acceptance_20261107_run01; shader/sources/provenance in
validation/r2cf6_shader_20261107_run01.
- n1_4:9604complex transforms/76832Float32components,zero bitmismatches against
actualFFTW1dcomplex4forward/inverse,unnormalized. Samebatchsizes,complexinputs,
multiple scales. Probeassertsactualplanname n1_4. Artifact
validation/n1_4_acceptance_20261107_run01; source/shader/oracleplans/provenance in
validation/n1_4_shader_20261107_run01.

StrictMSVC builds andshader/spirv-val passed; NVIDIA RTX A500 execution with
Vulkan validation, no reportederrors. Padding/DCprocessing doesnotoccur in these
leaf tests. Input/output are contiguoustransform batches;4K stage strides,
twiddle-generation,threadgroup layout andcompletecomposition remainunvalidated.
Counts representmeasuredcoverage,not universalproof. NoCPUfallback inGPUleaves.

Subsequent composition:48pointcomplexFFT usingt1_12+n1_4 PASSED9604forward/inversetransforms /921984Float32components,zerobitmismatches againstactualFFTW plans assertingt1_12. Same6batchsizes per direction,randommultiplescales,unnormalized. Shader/spirv-val/strictbuildpassed; noVulkanerrorsreported. Artifacts fft48_acceptance_20261107_run01 andfft48_shader_20261107_run01. Twiddlecoefficients are offlinebitencoded constants; CPUfixturepreparationisnotper-imageGPUwork. Thisdoesnotcover4Kstrides/fullcomposition. A720point t1fv_15+48compositionisunderdevelopment.


## 720-point composition ? 2026-11-08

The corrected t1fv_15 + 48 composition passes 9,604 forward/inverse transforms:
13,829,760 Float32 components with zero bit differences against the installed
FFTW reference (3 threads, flags 82, plan name asserted). Artifact:
validation/fft720_acceptance_20261108_run01; provenance.json records inputs,
layout, normalization, coefficients, source/shader/executable/DLL hashes.

The first version failed 2,050,572 components; its run01 artifacts are retained.
A strided native 48-point inner plan reproduced the same mismatch, ruling out
that layout as the cause in this diagnostic. The native 720-point plan's packed
twiddle table was read in a separate guarded ABI diagnostic and all 1,344 Float32
coefficient components match the shader. Internal ABI access is diagnostic only.

Disassembly of the frozen DLL's t1fv_15 kernel at RVA 0x284dd0 revealed six
compiler FMA contractions beyond the explicit source VFMA/VFNMS calls: the
products TI, TY and T1d feed two fused consumers each. Modelling these explicitly
first matched all 90,720 forward diagnostic values on CPU, then passed the full
GPU suite. The generator explicitly preserves these contractions; it does not
rely on driver reassociation. Shader compilation and spirv-val passed; Vulkan
validation reported no errors on NVIDIA RTX A500 Laptop GPU.

No padding, DC adjustment or inverse normalization occurs in this isolated test.
Offline coefficient generation and synchronous host transfers remain. This is
not yet a complete 4K FFT, GPU optical filter or end-to-end GPU metric pass.

## Actual 4K t1fv6 forward stage ? 2026-11-08

Extracted the real 7680x4320 R2C FFTW plan rather than substituting a standalone24 plan. Guarded internal-layout diagnostics locate forward t3fv20 (radix20, m384, RVA291b10), t3fv16 (radix16, m24, RVA28fcf0), and t1fv6 AVX2 (radix6, m4, RVA226510). Exact plan/parameter evidence: validation/fft4k_subplans_20261108_run01 and validation/fft4k_subplan_fixtures_20261108_run01. Internal callbacks remain limited to disposable test tools.

The Vulkan t1fv6 forward twiddle stage PASSED 4802 disjoint fixture groups /230496 Float32 components against the actual native callback. Tests cover batch sizes1,63,64,65,513,4096, finite complex constants/impulse/seeded random at four scales. Groups are radix6 by four frequency lanes (24 complex values), not complete24 FFTs. No padding, normalization or DC processing. The40 coefficient components match the native AVX2-256 interleaved table exactly. An earlier packing report incorrectly assumed AVX2-128 layout; it is preserved and superseded by coefficient_comparison_interleaved.json.

Shader compilation, spirv-val, strict MSVC build and Vulkan validation passed without reported validation errors. Artifact validation/fft_t1fv6_acceptance_20261108_run01/provenance.json includes exact scope and source/shader/binary/DLL/fixture hashes. This does not yet integrate t3fv16/t3fv20, R2C recombination or full4K processing.

## Actual 4K t3fv16 forward stage ? 2026-11-08

The Vulkan radix16/m24 forward twiddle stage passes4802 disjoint fixture groups /3687936 Float32 components with zero bit differences against the callback selected in the actual7680x4320 R2C FFTW plan (3threads,flags82). Canonical layout:16rows x24 interleaved-complex values, native rs=48*j, ms2, mb0, me24. No padding, normalization or DC handling. Seed7616, finite complex constant/impulse/random input at four scales, batch sizes1/63/64/65/513/4096.

First attempt had610593 mismatches and is retained at validation/fft_t3fv16_acceptance_20261108_run01. Native kernel disassembly exposed eight FMA contractions in consumers of TL,TZ,T1o,T1t products. Explicitly reproducing them removed all differences. Corrected pass: validation/fft_t3fv16_acceptance_20261108_run02/provenance.json. All192 coefficients also match the native table exactly. Shader, spirv-val, strict build and Vulkan validation passed, with no reported validation errors.

Fixture extraction retains full plan/strides/source/binary provenance in validation/fft4k_t3fv16_fixtures_20261108_run01. Internal ABI access remains diagnostic only. This is not a complete384-point FFT or4K transform. The executable summary contains an inherited FFT24 label; provenance corrects that wording without changing historical outputs. Remaining forward t3fv20 composition, real4320 recombination, inverse stages and integration remain unfinished.
