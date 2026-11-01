# Double sRGB optical and adaptation checkpoint

Date: 2026-10-28. Completed root:
`validation/srgb_optical_completed_path.txt`. The accepted native exports
are `native_v2`; all older exports and failed diagnostic directories remain.

This checkpoint adds CPU stage APIs `project_optical_rgb_double` and
`finish_local_adaptation_double`. The former independently calibrates the
LMSR matrix from raw spectral files, projects already optically filtered
Double RGB using the native forward FMA order, clamps responses, sums L+M
and takes the adaptation logarithm. The latter exponentiates an already
Gaussian-filtered Double log map. Their inputs describe stage boundaries;
neither API supplies an optical FFT or Gaussian convolution implementation.

`capture_srgb_optical.m` reads the original encoded Single stress pair from
the preceding sRGB preprocessing checkpoint. It instruments local copies of
official HDR-VDP-3 v3.0.7 to capture Double optical RGB, LMSR, adaptation,
filters, padded arrays, complex FFTs and real inverse transforms. Native
stock/instrumented Q and Q_JOD bits match for all three spectra. Octave11.3,
96x96,30ppd,age24,defaultsp0,Double FFTW3threads3/estimate/cleared dwisdom.
The first three convolutions use symmetric post padding to192x192; the
fourth filters log-adaptation with replicate post padding. Layout is
column-major planar, with interleaved real/imaginary FFT components. Native
DC handling, inverse normalization and clamp order are preserved.

Measured results:

- Independent LMSR calibration:72 captured coefficient comparisons exact
  (three matrices checked twice).
- Stage API:608,256 Double words bitwise exact,20 invalid-input checks passed.
  This uses captured optical and Gaussian outputs as explicit stage inputs.
- Existing sRGB preprocessing regression:497,664 Double words exact and18
  rejection checks; normal IQM Release build succeeded.
- Isolated FFT diagnostic with matching unaligned planning:1,769,472 forward
  components and884,736 inverse values exact. It uses captured stage inputs.
- Independent CPU diagnostic starting from original encoded input: forward
  FFT1,769,472 components, inverse884,736 values, padding884,736 values,
  filters884,736 values and final adaptation55,296 values all bitwise exact.
  Total4,478,976 compared Double words across three spectra/two sides of
  ONE stress pair. Captures supply expected values only in this mode.

The independent CPU diagnostic uses production preprocessing/projection
functions and computes its own MTF/Gaussian filters, padding and FFT inputs.
Its FFT backend explicitly loads the local Octave Double FFTW DLL for this
Windows test; it is **not linked into production IQM**. This establishes an
independent CPU reproduction through adaptation, not a production FFT backend,
GPU equivalence, full sRGB image-to-score acceptance or cross-platform parity.

The initial aligned FFT plan produced1,469,452 forward and498,056 inverse
bit mismatches. Setting `FFTW_UNALIGNED` matched the native scalar codelets
and eliminated these differences; DLL identity and thread count alone did
not establish equivalence. Diagnostic plans and native wisdom are preserved.
The first diagnostic process exited with heap corruption after releasing a
foreign-runtime plan string through the wrong deallocator. That diagnostic
was corrected to retain the few plan strings until process exit; successful
runs were fresh, and production code does not use this diagnostic loader.

```powershell
$r = Get-Content validation/srgb_optical_completed_path.txt
$p = Get-Content validation/srgb_double_completed_path.txt
& "$r/srgb-fft-double-inspect-used.exe" "$r/native_v2" "validation/srgb_optical_recheck_$([DateTime]::UtcNow.Ticks)" unaligned C:/HDR-VDP-3_Ref/data "$p/native_stress"
```

Next: capture post-pupil, JND and combined perceptual response in Double,
implement those stages and compare independently before Double pyramid,
masking and score pooling. A production Double FFT/runtime path also remains
required before enabling CLI `srgb-display`. No shader/runtime assets or
existing CLI routing changed in this checkpoint.
