# Mixed-radix FFT plans and fixed inverse normalization

Checkpoint: `validation/mixed_fft_plans_639283629886514449`.
This follows the [general dimension frontier](GENERAL_DIMENSION_FRONTIER.md).
Public80x96/96x80 quality is still rejected:the GPU FFT graphs are not ported.

Native fixtures cover80x96/96x80,asymmetric/impulse/HDR-ramp,reference/test
(test factor1.05),MTF andlocal adaptation:24 independent staged chains.
`generate_rectangular_filter_reference.m --mixed` explicitly admits only these
additional diagnostic shapes;the original generator's default contract is retained.
Inputs are Float32,filters double,30ppd/age24 conventions,MTF symmetric andLA
replicate postpadding,exact160x192/192x160 FFT sizes,noDC processing.

`inspect_fftw_plan.cpp --mixed-plan <fixture> <fresh-output> H W` tests32
candidate configurations per fixture:3/1threads,aligned/unaligned planning,
in-place/out-of-place complex inverse,for both labels andchains. The main census
is192 candidates across6 fixtures. Eight candidates per fixture match both
forward and inverse numerically;these duplicate the same24 unique streams under
two thread configurations. The selected3thread/unaligned/out-of-place candidates
match2,949,120 Float32 forward+inverse values. Signed-zero bits are not asserted
for this CPU FFT plan comparison.

The source is column-major interleaved. FFTW C dimensions are `{W,H}`;compact
real output begins at complex offset `W*((H-1)/2)`. The tool expands Hermitian
storage and exports row-major comparisons. Full execution-plan text records
strides,batching andcodelet ordering. The160-point first axis uses scalar
`r2cf_20/r2cfII_20/hc2cf_20` plus`r2cf_8`;the192-point complex graph includes
`t3fv_8_avx2_128`,`t1fv_6_avx2`,`n1_4` (inverse variants likewise recorded).
This is not the assumed5x32/3x64 graph.

Wisdom import succeeds,but no WISDOM_ONLY plan is recovered for these candidate
contracts. Successful comparisons use freshly constructed ESTIMATE plans.
An additional32-candidate PRESERVE_INPUT probe did not resolve that cache miss.
The selected graphs are arithmetic-equivalent on the measured fixtures;we do
not claim that the original Octave cached plan object was reconstructed.
The original/default tool mode was rechecked after adding the preserve probe.

Inverse normalization is **one Float32 division by30720**,not multiplication by
a once-rounded Float32 reciprocal. For example,the asymmetric80x96 streams show
35,721–37,312 differing values with reciprocal multiplication while division
matches exactly. A new isolated shader implements this finite binary32 division
using uint32 significand division and round-to-nearest/ties-to-even,including
subnormal results and signed zero. It exploits30720=15*2^11;it is not a generic
division implementation. No shader Float64/Int64 or floating-point execution
modes are required. NaN/infinity semantics are outside the measured scope.

On **Intel Arc Pro Graphics**,the shader passes:

- 1,054,696 synthetic finite inputs bitwise against strict volatile CPU division,
  including every finite exponent class,both signs,subnormal/zero androunding
  boundary controls. This is a probe,not exhaustive enumeration of all mantissas.
- 1,474,560 values from24 native inverse FFT streams bitwise against independently
  exported official `ifft2` results. The input is the matched CPU FFT's unscaled
  output;this does not constitute a GPU FFT or a production CPU fallback.

The host checker requires RTE/gradual underflow. Final accepted output is
`gpu_normalization_bits/`;the reusable end-to-end driver also passes under
`runner_verified/`. Repeated runs are regressions,not additional unique inputs.
Initial normalization attempts failed on quoted Windows backslashes in the
manifest and PowerShell terminating on informational Vulkan stderr. Those
artifacts remain preserved;the driver writes forward-slash paths and checks
native exit codes explicitly. There were no numerical shader mismatches.

Build targets are `dimension-native-plan-test` (Windows/Octave diagnostic) and
`mixed-normalization-test` in `tests/hdrvdp3_dimensions`. Run
`validate_mixed_fft.ps1` with the fixture root,both executables,the generated
normalization shader and a fresh output directory. Compile only the new shader;
the production FFT/map binaries and public dimension guards remain unchanged.

Next:port the160-point real first-axis graph and export compact half-spectrum
fixtures for isolated GPU comparison,then the192-point second axis andinverse
graphs. Integrate the already validated padding and fixed normalization only
after stage parity.120-sample DC/base adaptation and10x12 residual FFTs remain
separate requirements before full quality for80x96/96x80.
