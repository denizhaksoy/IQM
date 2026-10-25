# Complete160x192 inverse GPU transform

Checkpoint: `validation/mixed_inverse_full_639283669024826224`.
This is component validation from independently exported filtered spectra;
it does not enable public general-dimension quality or establish GPU filter/map
integration. Production shaders and dimension guards remain unchanged.

`generate_fft160x192_inverse_shader.ps1` composes the already accepted scalar
inverse160 first axis with a new inverse192 second axis. It uses scalar `n1_4`
leaves with backward real/imaginary pointer roles, `t1bv_6_avx2` for24-point
inner transforms, and `t3bv_8_avx2_128` for the outer radix8 stage. The t1 graph
uses the ordinary generated branch with explicit SIMD FMA macros; t3 uses the
generated FMA branch. This matches the native candidate arithmetic; it does
not identify original Octave cached objects or its build-time branch choices.
The previously matched native second-axis plan groups160 transforms as54/54/52;
the GPU performs each lane's arithmetic independently.

The newly pinned backward codelets retain original GPL notices under
`fftw_reference/mixed_inverse/SOURCES.json`. The generator checks LF hashes,
`t1b.h` BYTW1/TWVL1 selection, `t3b.h` VTW3/TWVL3 selection and descriptor
powers. BYTW1 uses cosine-product FMA with separately rounded sine products.
Precise scalar/vector helper results retain the proven rounding boundaries.
24/192 twiddle tables use host double trig rounded once to Float32 with exact
quadrant controls. `compile_inverse160x192.ps1` compiles only the new shader,
adds explicit DenormPreserve32/RoundingModeRTE32 SPIR-V modes, assembles and
validates Vulkan1.2. Capability checks alone would not enable those modes.

Two modes use the same shader:

- `--inverse160x192-raw` returns the complete unnormalized inverse. Reference:
  prior matched native rank2 FFTW unscaled exports, using three threads,
  fresh ESTIMATE|UNALIGNED and out-of-place inverse. These are arithmetic-
  equivalent candidates, not recovered original cached native objects.
- `--inverse160x192` additionally applies the independently validated finite
  Float32 division by30720. Its integer helper body is embedded unchanged
  from `mixed_normalization.comp`; it does not multiply by a rounded reciprocal.
  Reference: independently exported official Octave `ifft2` results.

Input is160x192 column-major interleaved complex Float32, header160 384,
from MTF/LA stage2. It already contains native filtering of a spectrum prepared
using symmetric POST padding for MTF or replicate POST padding for LA. First
inverse output is transposed for contiguous192-point second-axis reads.
Final GPU output is column-major; the checker transposes it for comparison
and exports row-major interleaved160x192. `axis0_storage.txt` retains the
checker's storage header192 320; its word sequence matches the prior first-axis
row-major160x192 oracle (header160 384). There is no additional padding/filter,
mean/DC removal, crop, exponentiation or map processing here. Parameters remain
30ppd, ref/test factor1.05 component fixtures, not full-quality image pairs.

Build `fft256-inverse-test` from `tests/hdrvdp3_fft256`, then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_inverse160x192.ps1 -OutputDirectory <fresh shader directory>
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/validate_mixed_inverse.ps1 -NativeDirectory <mixed_fft_plans checkpoint> -AxisDirectory <inverse160 checkpoint>/verified_preserve -Executable <GPU exe> -Shader <shader directory>/inverse160x192.spv -OutputDirectory <fresh directory>
```

The reusable driver copies prior native inputs/references into fresh fixture
directories with source paths and hashes, verifies raw and normalized results,
and checks first-axis regression values independently in both modes. Its
24 per-mode logs, `results.tsv`, input/binary hashes and final `PASS` are under
`verified/`. Require numerical equality; the FFT checker does not assert
signed-zero bits. The normalization helper has separate prior bitwise tests;
those do not extend a bitwise claim to the whole inverse transform. Inputs
and expected outputs are finite; no NaN/infinity parity claim is made.

`shader_reproduced/` validates the final generator assertions and produces
identical SPIR-V bytes to the tested `shader/` (`reproduction_check.txt`).
Initial asymmetric ref MTF raw/normalized probes both pass; these repeats do
not add unique input coverage. No tolerance acceptance is used.

Final result on Intel Arc Pro Graphics: all12 streams pass737280 raw inverse
Float32 comparisons and737280 normalized inverse comparisons, zero mismatches.
The737280 unique first-axis regression values remain exact in both modes.
The newly built checker also passes a61440-value asymmetric ref MTF forward
regression. Shader capabilities are only Shader,DenormPreserve,RoundingModeRTE
(no Float64/Int64). There were no failed numerical probes in this checkpoint.

Next: inspect and port the transposed192-FIRST orientation rather than assume
that swapping dimensions preserves its graph. Before public80x96/96x80 quality,
also complete120-sample mean/DC,10x12 residual transforms and filter/map
integration, then compare full official stages and scores.

`transposed_next_plan.txt` extracts the selected fresh UNALIGNED/out-of-place
candidate for that next shape (both directions). Its real192 first axis uses
hc2cf_8/r2cf_8/r2cfII_8 with nested hf_12/r2cf_12/r2cfII_12/r2cf_2;
its inverse192 first axis instead uses t2_16/n1_12. The complex160 second
axis uses t3fv_20/n2fv_8 forward and t3bv_20/n1_8 backward. These differences
are why the current160x192 pass does not imply a192x160 pass.
