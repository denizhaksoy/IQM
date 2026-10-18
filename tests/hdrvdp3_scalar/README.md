# HDR-VDP-3 scalar reference checks

This standalone project builds the production CSF and bicubic resize sources
without Vulkan, generated shaders or the experimental IQM wrapper. It checks
native Octave fixtures with exact equality. It is a scalar implementation
checkpoint, not a complete HDR-VDP-3 metric or GPU backend.

The CSF preserves the reference's interpolation, AESL expression and division
order. In particular, dividing by a power must not be replaced by multiplying
by a reciprocal power. Single luminance queries round interpolated parameters
to single before double evaluation. LUT samplers retain the actual log grid
and distinguish double and single query arithmetic.

Prerequisites: CMake, a C++20 compiler, Octave with the image package, and an
independently installed official HDR-VDP-3 v3.0.7 reference. The generators
accept the reference directory as their second argument, verify its version,
and never modify it. Their output directories must be new. Generated fixtures
and build products are not checked in.

From the repository root, generate fresh fixtures (each path below must be new):

```powershell
octave-cli --quiet tests/generate_csf_reference.m '<fixtures>/csf' '<official reference>'
octave-cli --quiet tests/generate_base_csf_reference.m '<fixtures>/base' '<official reference>'
octave-cli --quiet tests/generate_csf_lut_reference.m '<fixtures>/lut' '<official reference>'
octave-cli --quiet tests/generate_resize_reference.m '<fixtures>/resize'
cmake -S tests/hdrvdp3_scalar -B '<new build>' -DIQM_HDRVDP3_FIXTURE_ROOT='<absolute fixtures>'
cmake --build '<new build>' --config Release
ctest --test-dir '<new build>' -C Release --output-on-failure
```

On Windows, use the full octave-cli executable path if it is not in PATH.
An Octave shutdown warning with exit0 is not a numerical failure.

The direct CSF, base noise and both resize tests reject any mismatch at their
declared output precision. The LUT test requires exact float outputs and
single query results; it also reports double LUT/query differences separately.
A successful LUT float check alone does not establish double LUT equality.
Direct fixtures include ages24/45/70 with AESL enabled/disabled, base frequency
and luminance grids, clipping knots and adjacent floats. Resize fixtures cover
single/double data, antialiasing, symmetric edges, non-square/one-row shapes,
downsampling and upsampling. These sampled checks do not establish full-metric
parity for arbitrary images, dimensions, color spaces or viewing conditions.

## Verified checkpoint (2026-10-18)

The selected source files were copied to a fresh isolated tree, configured
without the remaining workspace changes, built with MSVC19.51 and checked
against fresh Octave11.3 fixtures: all seven CTest tests passed.

- Direct CSF:1152 double-luminance and1152 single-luminance queries have
  zero double differences across ages24/45/70 and AESL enabled/disabled.
- Base CSF/noise:15675 cases have zero double differences and zero final
  single neural-coefficient differences, including infinite noise at zero
  frequency. The strict modes enforce double equality by exit status.
- Bicubic resize:45 cases/32643 output values match exactly in both float32
  and float64, including non-square, one-row and upsampling cases.
- CSF LUT:3840 nodes and22530 queries match at float32 precision, including
  the separate single-query sampler. There are539 double node differences
  and3194 double-query differences (maximum observed query error about
  8.88e-16). This checkpoint does **not** claim double LUT parity. The direct
  CSF/noise equality above does not remove this distinct LUT construction/grid
  issue. Generated shader/GPU pipeline code is outside this standalone project.
