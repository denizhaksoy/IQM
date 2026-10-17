# Isolated HDR-VDP-3 FFT parity tools

Fresh standalone verification: [publication validation](VALIDATION.md).

These test-only tools reproduce selected arithmetic graphs of the single
FFTW library used by Octave. They do not replace production preprocessing or
claim complete HDR-VDP-3 parity. Derived FFTW sources and generated shader keep
their GPL-2.0-or-later notices and license in `../fftw_reference/`.

Build independently of unfinished metric changes (from repository root):

```powershell
cmake -S tests/fft_parity -B build-fft-parity
cmake --build build-fft-parity --config Release
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/generate_fft_codelet_shader.ps1
glslc tests/fftw_reference/fft_codelet_test.comp -o build-fft-parity/fft_codelet_test.spv --target-env=vulkan1.2
```

Requirements: C++20, Vulkan SDK and validation layer, a Vulkan compute device,
and (for the CPU oracle) Windows with local Octave 11.3.0. The diagnostic DLL
path in `inspect_fftw_plan.cpp` targets that installed version explicitly.
Script policy changes above affect the process only. Use new fixture output
directories for every reference generation.

```powershell
$exe='build-fft-parity/Release'
& "$exe/IQM-fftw-plan-test.exe" --codelets 'build-fft-parity/new-leaf-reference'
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-leaf-reference' build-fft-parity/fft_codelet_test.spv
& "$exe/IQM-fftw-plan-test.exe" --inverse2d 'build-fft-parity/new-inverse-reference'
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-inverse-reference' build-fft-parity/fft_codelet_test.spv --inverse2d
& "$exe/IQM-fftw-plan-test.exe" --forward2d 'build-fft-parity/new-forward-reference'
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-forward-reference' build-fft-parity/fft_codelet_test.spv --forward2d
& "$exe/IQM-fftw-plan-test.exe" --r2c 'build-fft-parity/new-r2c-reference'
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-r2c-reference' build-fft-parity/fft_codelet_test.spv --r2c
& "$exe/IQM-fftw-plan-test.exe" --forward-second 'build-fft-parity/new-main-reference' 3
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-main-reference' build-fft-parity/fft_codelet_test.spv --forward-second
& "$exe/IQM-fftw-plan-test.exe" --forward-second 'build-fft-parity/new-tail-reference' 2
& "$exe/IQM-fft-codelet-test.exe" 'build-fft-parity/new-tail-reference' build-fft-parity/fft_codelet_test.spv --forward-tail
```

The main forward second-axis graph differs from its final two-frequency tail.
Reference plan assertions reject unexpected codelets; strict comparison fails
on any unequal finite float component. Full 128x128 forward and inverse now
match the native Octave reference on the documented fixtures. Production
integration, log/exp/subnormal parity and full metric validation
remain outstanding. See [detailed results](../fftw_reference/README.md) and
[reference plan inspection](../FFTW_PLAN_VALIDATION.md). Historical `validation/`
paths in those documents describe local artifacts; generate fresh synthetic
fixtures with the commands above when those artifacts are unavailable.
