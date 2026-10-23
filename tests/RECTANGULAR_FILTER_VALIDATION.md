# Rectangular MTF and local-adaptation integration

Evidence root:
`validation/rectangular_filter_639282611436713337/`.
This extends the [rectangular component checkpoint](RECTANGULAR_COMPONENT_VALIDATION.md).
Public evaluate_quality dimensions remain square32/64/128.

The new native generator produces six fixtures:64x128 and128x64 images,
each with asymmetric, three-impulse and HDR ramp patterns. The ramp spans
1e-3..1e4 cd/m2. Reference/test images differ by a global single1.05 factor.
Official v3.0.7 fast_conv_fft/hdrvdp_local_adapt outputs are independently
compared against exported native staged operations before accepting fixtures.
The local native profile is single,3threads,estimate,30ppd. MTF uses symmetric
post padding;LA uses replicate post padding and native single log/exp.

The isolated R32 image chain passes24 cases (six fixtures x two labels x
MTF/LA), comparing6,488,064 GPU Float32 values with zero numeric mismatches.
These are four complex padded stages and the final cropped R32 output.
Host-generated filters match786,432 Float32 reference coefficients after
rounding;this is not a binary64 filter-identity claim. Accepted evidence is
gpu/COMPLETED.json,PASS.txt,records and per-case logs/exports.

All transforms use FFT shapes128x256 or256x128, column-major interleaved
storage and inverse normalization once1/32768. Filter coefficients are
explicitly transposed from host row-major to match the spectra. No DC removal,
CPU FFT fallback or CPU intermediate-output substitution is used. Stage traces
are copied/read only for comparison after GPU execution. Signed-zero bit
identity is not asserted by numeric stage equality.

The combined source-generated shader ABI64256 dispatches forward graphs8/9
and inverse3/4 using the accepted rectangular codelets. A fresh regeneration
has the same SPIR-V SHA256:
`5A8C4BE851DC6BE0ABE6B150397DE8B5179ED59A37A07D65756FE5FEFCD2C558`.
Square shader contracts/assets remain separate. The shader preserves
Float32 denormals and RTE. FFTW GPL notices/COPYING remain with the generated
components;native log/exp reuse the previously pinned AMD source-backed math.

LocalAdaptFFT now recognizes rectangular64x128/128x64 reference execution
at30ppd with explicit math assets and ABI64256. It selects each axis length,
batch count and coefficient layout explicitly. Wrong square/rectangular shader
contracts are rejected. evaluate_quality continues rejecting rectangles until
MapGPU and continuous score parity are established. Its private filter helper
accepts explicit dimensions only for the diagnostic caller;normal API calls
retain their square dimension inference.

The first production test run stopped while the test reader parsed tiny
double Gaussian coefficients directly asfloat. All GPU stages/output in that
case already matched;it was not a completed validation pass. The reader now
parses double then rounds tofloat, as the existing isolated checker does.
Initial production/ artifacts remain preserved;accepted reruns use
production_fixed/. A separate production_smoke repeats one MTF case.

All24 production_fixed cases completed with zero numeric mismatches in the
same6,488,064 GPU values and786,432 rounded filter coefficients. This repeats
the isolated suite's input domain;it is not24 additional unique inputs.
Current-source public32/64/128 API regressions preserve score bits and
96/107/107 numeric export hashes. Nineteen invalid API cases and three wrong
square/rectangular FFT shader-contract cases are rejected. No accepted run
contains Vulkan ERROR/Validation Error/VUID diagnostics. RootCOMPLETED.json,
PASS.txt andvalidation/rectangular_filter_completed_path.txt mark completion.

Reproduction from the repository root:

```powershell
# Repeat for shapes64 128 /128 64 and three patterns.
octave-cli --quiet --no-gui tests/generate_rectangular_filter_reference.m fresh-native 64 128 asymmetric
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_fft256/compile_rectangular_inverse.ps1 -OutputDirectory fresh-combined -Combined
cmake --build validation/spyr_dimensions_639282601672947206/build --config Release --target IQM-filter-dimension-test IQM-quality-api-test
```

Build the dimension targets with IQM_HDRVDP3_DIMENSION_DIAGNOSTIC=ON in the
standalone tests/hdrvdp3_evaluation project. validate_rectangular_filter.ps1
accepts Executable,NativeRoot,ForwardShader,InverseShader,MathDirectory and
OutputDirectory. Default execution uses fft256-filter-test with separate
forward/inverse shaders. With -ThroughLocalAdapt use IQM-filter-dimension-test
and supply the combined shader for bothshader arguments. Every numerical
output directory must be fresh. validate_rectangular_regressions.ps1 runs the
current public API against32/64/128 pinned outputs/scores.

MTF andLA use separately uploaded native inputs. This is not a continuous
visual-pathway/JND/masking/quality run and does not establish rectangular
Q/Q_JOD parity. Remaining work is an explicit rectangular MapGPU contract:
128-sample DC/base adaptation,8x16/16x8 residual first-max/frequency,
rectangular CSF/mask resizing and masking/pooling,then full native stages/scores.
native_reductions128/ additionally prepares512 native128-sample reduction
cases and two rectangular residual FFT/selection fixtures for that next step;
these fixtures alone are not a GPU pass.
