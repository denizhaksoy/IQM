# Age-dependent spectral calibration

Updated 2026-10-19. A new independent C++ calibration component matches the
official v3.0.7 age-dependent D65 calculation. Full metric age integration
is still pending.

`calibrate_spectral_age` in `spectral.cpp` accepts already resampled double
wavelengths, D65 emission and LMSR sensitivity. It computes lens optical
density, clamped pchip interpolation, transmission, filtered emission,
trapezoidal spectral integrals and the normalized four-channel matrix.
The implementation follows the shape-preserving derivative arithmetic in
[SLATEC DPCHIM](https://www.netlib.org/slatec/pchip/dpchim.f), with explicit
non-FMA evaluation and strict compiler floating-point options.

## Measured scope

The official loader supplies **420 wavelengths**, including both endpoints
360 and 780. Despite its comment describing a 1 nm step, its linspace count
is 420. The exact exported grid is preserved; using a newly constructed
421-point grid would change the inputs. This component does not yet port
raw spectral-file resampling. Native data files and old references are left
unchanged; generated inputs remain local validation artifacts.

Thirteen ages cover 15, 19.999, 20, 24, 28, 32, 45, 60, 60.001, 75, 83,
83.001 and 100. These include the lens formula branch at 60 and the
age-32 zero-density correction. Values outside typical observer ranges
exercise the calculation; they do not establish valid public input ranges.

| Stage | Double values | Numeric differences |
| --- | ---: | ---: |
| Optical-density nodes | 338 | 0 |
| Interpolated optical density | 5,460 | 0 |
| Transmission | 5,460 | 0 |
| Filtered emission | 5,460 | 0 |
| Spectral integrals | 52 | 0 |
| Normalized LMSR matrix | 52 | 0 |
| Total | 16,822 | 0 |

The test also requires the age-24 L/M/R coefficients to equal the legacy
constants exactly. Comparisons use numeric double equality, not tolerance;
they do not separately assert signed-zero bits.

An additional native integration check calls official `hdrvdp3` with
identical single-channel 32x32 images at each age. All 26 printed matrices
match the oracle at the diagnostic's 15 decimal places and after conversion
to single (104 coefficients); native identical-image Q/Q_JOD are 10.
Those limited-precision logs are not the double-equality proof above, and
these are official-only runs, not C++ full metric age tests.

## Reproduction and next step

Evidence root: `validation/spectral_age_639280297796803042`, selected by
`validation/spectral_age_completed_path.txt`. The root contains source/data
hashes, complete initial/final comparison logs, official integration logs,
build logs and CTest output. `tests/hdrvdp3_scalar` builds the independent
`spectral_age_test` target without mixed GPU/application integration.

```powershell
octave-cli --quiet --no-gui tests/generate_spectral_age_reference.m '<fresh fixture directory>'
cmake -S tests/hdrvdp3_scalar -B '<fresh build directory>' -DIQM_HDRVDP3_SPECTRAL_FIXTURE='<absolute fixture directory>'
cmake --build '<fresh build directory>' --config Release --target spectral_age_test
ctest --test-dir '<fresh build directory>' -C Release -R '^spectral_age_double$' --output-on-failure
octave-cli --quiet --no-gui tests/validate_spectral_age_official.m '<fixture directory>'
```

Next pass the calibrated L/M/R coefficients through C++ JND and Vulkan
adaptation/JND preparation, replacing the fixed age-24 constants while
retaining identical default behavior. Propagate observer age into pupil and
CSF/AESL consistently and update validators. The initial integration may use
the pinned resampled spectral fixture; raw data loading/resampling remains
a separate requirement. Full age support requires fresh end-to-end metric
comparisons afterward. See [component age checks](OBSERVER_AGE_VALIDATION.md).
