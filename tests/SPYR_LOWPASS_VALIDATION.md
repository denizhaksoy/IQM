# General-shape SPYR lowpass checkpoint

Accepted root: `validation/pathway_spyr_lowpass_639284274200978716`.
Actual device: Intel Arc Pro Graphics. This validates the lowpass route to
the raw base band, not all SPYR bands or complete GPU quality.

`generate_pathway_spyr_lowpass_reference.m` starts from the actual native
P_reference/P_test maps in `general_pathway_639284244350897941`: two image
pairs,80x96 and96x80,30ppd,age24. It exports independent native corrDn stages:
7x7 lo0 at step1, then three13x13 lowpass stages at step2. Reflect1 is applied
at every stage with native start[1,1] (zero GPU offset). The last stage is
divided by16, matching get_band's normalization for band index4. Native
assertions reproduce the raw SPYR base and the previously traced DC-centered
base **bitwise**. No mean is removed inside the new GPU lowpass chain.

`spyr_lowpass.comp` uses independent SSBO input/output/filter buffers, Float32
coefficients rounded once from the official double filter tables, descending
tap traversal and reflect1 integer coordinates. The longer image axis
determines traversal. Native32-element vector blocks use explicit FMA; their
remaining tail uses precise separate multiplication and addition. These are
two separately compiled variants. Each invocation writes only its variant's
portion of output; both variants execute before advancing to the next stage.
At20x24/24x20 the entire output uses the separate arithmetic path.

The checker submits all four stages/eight dispatches in one command buffer
with compute barriers. It uploads the original P map and filter coefficients
only; no intermediate host calculation or upload occurs. It reads stage
outputs after the final fence for verification. Image/filter/output layout
is row-major Float32 words; no FFT padding, MTF/LA filtering or resampling
outside the specified corrDn operations occurs. No power-of-two enlargement
is used. Raw output geometry is10x12/12x10.

`gpu/PASS.txt` and `gpu/results.tsv`: all16 stage comparisons pass, **40800
Float32 values numerically exact**. Observed bit differences are also zero;
signed-zero bit identity is reported separately and is not the acceptance
requirement for convolution. Counts per input:7680+1920+480+120=10200. These
four inputs are reused from previous pathway fixtures, not new unique data.
No numerical trial failed in this checkpoint.

New shader variants explicitly declare DenormPreserve32/RoundingModeRTE32.
Vulkan1.2 spirv-val passes, with no Float64/Int64 capabilities. Existing
production shaders, including the staged+unstaged corrdn test shader, remain
untouched. The isolated target `spyr-lowpass-test` uses a fresh MSVC Release
build with /fp:strict. Source hashes for official buildSpyr/buildSpyrLevs,
corrDn/rconv2 and sp0Filters are saved in native metadata; checkpoint hashes
freeze generated native references, experimental source, shaders and checker.

```powershell
octave-cli --quiet --no-gui tests/generate_pathway_spyr_lowpass_reference.m <fresh native> <full pathway checkpoint>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_spyr_lowpass.ps1 -OutputDirectory <fresh FMA shader>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_spyr_lowpass.ps1 -OutputDirectory <fresh separate shader> -NonFma
cmake -S tests/hdrvdp3_dimensions -B <fresh build> -G "Visual Studio 18 2026" -A x64
cmake --build <build> --config Release --target spyr-lowpass-test -j 4
spyr-lowpass-test <FMA shader>/spyr_lowpass.spv <separate shader>/spyr_lowpass.spv <native> <fresh GPU output>
```

The lowpass checker and the previously accepted mean/DC/difference/FFT chain
are still separate submissions/tools. Joining their GPU buffers, validating
highpass/orientation bands, and general-shape visual-pathway/filter/resize/CSF
integration remain before full-stage and Q/Q_JOD GPU acceptance.

## Complete SPYR and joined residual chain, 2026-10-24

Those historical limitations are resolved for the saved P maps in the newer
checkpoint `validation/pathway_spyr_all_chain_639284285430220524`.
`--all-chain` runs all sp0Filters bands plus residual processing in one
command buffer per image pair. Four lowpass operations, highpass and three
orientation correlations use16 dispatches (two arithmetic variants each),
followed by mean120, difference/packing and residual FFT:19 dispatches total.
Only original paired P maps and coefficients are uploaded; intermediate
bands, means, differences and spectra stay on GPU. Native goldens are used
only for comparison after the fence. This is sp0Filters' single orientation
per scale; no other SPYR filter family is covered.

Native references and batched shaders are in
`validation/pathway_spyr_bands_639284284245362379`. The native generator adds
hi0/orientation filters and high/orient0/1/2 outputs, each asserted against
hdrvdp_spyr.get_band. The shader supports paired images and native orientation
normalization /2,/4,/8 alongside existing base /16. Explicit Float32 controls
and distinct FMA/vector-prefix versus separate/tail arithmetic remain.

Final `gpu_all_chain/PASS.txt` and `results.tsv`, Intel Arc Pro Graphics:
111840 SPYR/lowpass values numerically exact;484 mean/DC,480 packed-difference
and242 magnitude/index words bitwise exact;480 FFT components numerically
exact. Total **113526 comparisons**, all observed bit differences also zero.
Convolution/FFT signed-zero bits remain outside their acceptance. These are
the same two image pairs, not additional unique input coverage.

Fresh Release final checker also passes default lowpass40800, separate
`--bands`111840 and lowpass/residual `--chain`42486 regressions. Initial joined
lowpass development artifacts remain in
`validation/pathway_spyr_chain_639284283321272327`; all old binaries remain.
No numerical trial failed. Final source/binary and all referenced native/
shader inputs are frozen in the final checkpoint hash manifest.

```powershell
spyr-lowpass-test <batched FMA.spv> <batched separate.spv> <native SPYR directory> <fresh GPU> --all-chain <mean120.spv> <differencepack.spv> <extended FFT.spv> <combined native residual directory>
```

Full quality still requires GPU production of P, adaptation resize/CSF,
masking/pooling and full-stage/Q/Q_JOD comparisons. This checkpoint starts at
native P and ends at residual frequency selection. Public guards are unchanged.
