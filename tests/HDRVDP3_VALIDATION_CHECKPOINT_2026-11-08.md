# HDR-VDP-3 publication checkpoint ? 2026-11-08

The C++/Vulkan implementation remains experimental and partly CPU-based. Exact
results below apply to the measured configurations, not every image, device or
paper experiment. Numerical parity and subjective prediction accuracy are separate.

- Completed public LIVE HDR subset: 25 videos / 433 sampled frames;
  Q/Q_JOD bits and ordered Single video means match the accepted official v3.0.7
  records. This is not confirmation of the original paper's evaluation cohort.
- GPU L+M input and padding are optional runtime backends; their 16-case integration
  checks passed. Existing CPU/FFTW profiles remain available.
- GPU FFT128, scalar leaves, FFT48 and corrected FFT720 passed their documented
  bitwise comparisons. FFT720 covered 13,829,760 Float32 components.
- Actual 4K-plan forward t1fv6 and t3fv16 stages passed 230,496 and 3,687,936
  Float32 components respectively. These are isolated stages, not a full 4K FFT.
- Light3 alignment remains blocked. Ignoring MP4 edits adds five leading frames;
  matching header counts alone does not establish content alignment.
- The historical football frame150 reference discrepancy remains unresolved.

Compact success AND failed-attempt summaries are committed under
[results/hdrvdp3_checkpoint_20261108](results/hdrvdp3_checkpoint_20261108).
The t3fv16 executable summary inherited an FFT24 wording error: its actual scope
is radix16/m24, not a complete384-point or4K transform. See
[FFT4K_SCALAR_LEAF_VALIDATION.md](FFT4K_SCALAR_LEAF_VALIDATION.md) for scope and fixes.
[HDRVDP3_FULL_GPU_COMPLETION.md](HDRVDP3_FULL_GPU_COMPLETION.md) lists remaining
CPU calculations, device-buffer work and integration.

## Reproducing local diagnostics

Test tools use a matching official FFTW Single DLL, Vulkan SDK/validation layers,
MSVC/CMake, Node.js and FFmpeg/Octave as applicable. Numerical flags, three-thread
planning, memory layouts and selected codelets are significant. Guarded native
FFTW plan inspection uses internal layouts only in disposable test executables;
production code does not depend on those layouts.

Generated sources and fixtures under validation/ are intentionally not committed.
Several diagnostic generators reference dated local artifact directories; they
require the documented source extraction and plan/fixture generation steps before
running. The source archive is fftw-3.3.10-inspection.tar.gz, SHA256
56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467.
Preserve FFTW original notices and COPYING when extracting codelets. Do not run
compile_shaders.sh for these experiments; generate and compile individual shaders
into fresh directories. The committed compact measurements do not substitute for
raw-data reproduction. Dataset archives and video files remain local.
