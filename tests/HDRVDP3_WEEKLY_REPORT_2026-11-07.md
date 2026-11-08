Dear Professor,

I am sharing my weekly progress report below.

satisfaction with my own work last week: 8/10

accomplished:
completed a declared local evaluation of 27 LIVE HDR videos containing 513 sampled frame pairs. Q and Q_JOD matched the accepted official HDR-VDP-3 v3.0.7 reference records bitwise, and all video-level mean Q_JOD scores agreed exactly.
extended the combined experimental GPU backend validation to 529 distinct real HDR frame pairs across four contents.
performed subjective correlation analysis on the documented 27-video subset. The official and C++ predictions produced identical SROCC (0.935287) and raw PLCC (0.901651), independently checked in Octave. These are local-subset results, not reproduced paper results.
implemented and validated Vulkan comparison-mask generation, mutual masking, and Double-precision comparison-mask resizing. The resize-stage sweep matched 1,456,866,000 Double values across 529 real frame pairs exactly, with no reported Vulkan validation errors.
integrated GPU L+M adaptation-input calculation and optical/adaptation padding. The combined integration passed 16 end-to-end cases, covering 14 synthetic settings and two real 4K frame pairs, while preserving reference Q and Q_JOD bits.
validated an isolated 128x128 GPU FFT implementation against the actual reference FFTW library: 524,288 forward/inverse Float32 components matched bitwise. The combined GPU FFT, filter multiplication, inverse FFT, and cropping tests also passed. Began preparing the larger FFT sizes required for 4K processing.
verified the integrity and HDR stream metadata of all 310 supplied LIVE HDR files and automated evaluation of the 279 distorted videos, selecting 4,716 frame pairs. At this reporting checkpoint, 23 videos and 405 frames were complete, with matching reference and C++ scores.
added disk-efficient official reference scoring, input-integrity checks, independent result audits, and documentation. Published validated GPU stages and evaluation tools to GitHub.

not accomplished:
the complete 279-video evaluation and its final independent correlation analysis are still running.
the metric is not yet fully GPU-based. The general quality pipeline still uses CPU FFTW, model/LUT preparation, JND processing, pyramid construction, and remaining quality calculations. The new isolated GPU FFT supports only 128x128 and is not yet a replacement for the complete pipeline.
an earlier football-frame reference result differs from subsequent official runs; the original evidence is preserved and the discrepancy remains unresolved.
exact reproduction of the paper's original evaluation has not been established. We have the public LIVE HDR release, but the exact original evaluation split, submitted prediction scores, and some execution details have not been obtained. UPIQ is the model-calibration dataset; LIVE HDR is the video-evaluation dataset described in Section 6.

read since last report:
continued examining the official HDR-VDP-3 MATLAB/Octave implementation and FFTW operation graphs, focusing on masking, resizing, precision, and FFT planning.
reviewed the original HDR-VDP-3 paper, the 2023 challenge documentation, and its historical submission template.
reviewed the HDR-VDP-2 quality-calibration and validation sections: LIVE image data were used for fitting, while TID2008 was used for testing. These are separate from the LIVE HDR video evaluation.

to read before next report:
reference FFT code and numerical operation ordering required for larger image sizes and the remaining Vulkan stages.
available original evaluation artifacts and documentation needed to distinguish implementation equivalence from reproduction of the published experiment.

next steps:
continue the full LIVE HDR comparison and investigate any numerical mismatch before accepting the results.
extend the GPU FFT implementation to the required larger and non-square dimensions, then transfer the remaining CPU calculations to Vulkan while preserving reference agreement.
clarify whether applying the published procedure to the public LIVE HDR release and demonstrating equivalence with the official implementation meets the thesis validation requirement, or whether the authors' original evaluation split and predictions are also required.

planned for next week:
prioritize the remaining GPU processing stages and their bitwise validation - 10/10.
continue the complete LIVE HDR evaluation, independently audit the resulting video scores and correlations, and document the supported validation scope - 9/10.

Best regards,
Deniz Aksoy
