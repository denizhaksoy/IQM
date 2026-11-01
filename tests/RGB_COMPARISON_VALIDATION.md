# RGB comparison mask

2026-10-27: checkpoint at `validation/rgb_comparison_active_path.txt`.

`run_pathway_comparison_rgb` accepts finite nonnegative 96×96 RGB input in
three row-major planes. Its GPU mask ORs the three native relative difference
tests. For a zero reference, unchanged zero returns false (native NaN > .001),
while positive test returns true (native Inf > .001). Other inputs use the
existing finite division helper and threshold. Binary masks then use the
accepted Q20/Q40 integer resize arithmetic to produce exact Float64 values.
The mono API retains its positive-reference contract and supported geometries.

`compile_general_comparison.ps1 -RGB` emits an isolated RGB-capable shader
with mode3. Without that flag its module SHA256 remains
`548a5a77b4df909e29cb67476f02191b882d7450796d3bf76c36b94c77d45016`,
identical to the accepted public v2 comparison module. No old SPIR-V or
reference file is overwritten. spirv-val passes; Float32 preserve/RTE enabled.

Fresh official3.0.7/Octave11.3 captures use the RGB96 three-display fixture
with a new blue .25 patch inside the reference's zero patch. This differs
from earlier RGB cohorts. Instrumentation captures the actual official
`diff_mask`; an independent expression matches it and asserts changed and
unchanged zero pixels. Stock/traced Q/Q_JOD bits still agree. Native double
imresize goldens are exported at scales1,2,4,8 in column-major binary format.

GPU acceptance: twelve display/scale cases, **110,592 binary words and
36,720 Float64 resize values bitwise exact**. Counted binary words include
the same image mask repeated at four target sizes. This is comparison-only
acceptance, not full RGB quality. Input/output layout is explicitly converted
by the checker. Intel Arc Pro, Vulkan validation on, no reported errors.

Reproduction: generate fresh fixtures with
`generate_rgb_projection_reference.m <fresh-native> --jnd`; generate a fresh
shader with `compile_general_comparison.ps1 -OutputDirectory <fresh-shader>
-VulkanBin <sdk>/Bin -RGB`. Build `rgb-comparison-test` from
`tests/hdrvdp3_residual144`; arguments: native directory, generated
`general_comparison.spv`, fresh output directory.

Remaining: connect actual RGB P/adaptation and these masks to shared SPYR/
quality downstream processing, then validate final Q/Q_JOD and intermediate
bands. No full RGB metric-completion claim.
