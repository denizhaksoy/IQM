# Draft request for missing Section 6 reproduction details

Prepared 2026-11-05. This is a draft only; no message has been sent.

Subject: HDR-VDP-3 Section 6 — LIVE HDR evaluation reproducibility

Dear Professor Mantiuk,

I am reproducing the HDR-VDP-3 Section 6 LIVE HDR evaluation for my thesis and
validating a C++/Vulkan implementation against the official v3.0.7 release.
My documented local reconstruction currently matches the official implementation
on 513 sampled frame pairs from 27 distorted videos. I would like to distinguish
this numerical validation from reproduction of the original paper experiment.

Could you please clarify or share the following details of that experiment?

1. The exact video IDs/split used for evaluation and whether undistorted
   references were included in the correlation analysis.
2. The FFmpeg decoding/color-conversion command, PNG bit depth, and sampling
   phase for every 30th frame. The release video example appears to use a fixed
   frame filename inside its frame loop; what frame-selection code was actually
   used for the reported evaluation?
3. The MATLAB/Octave version, input precision and numerical backend, and whether
   the release example's viewing geometry, OLED spectrum, age 24 and ambient
   illumination settings match the original submitted predictions.
4. The original per-video prediction vector and the corresponding subjective
   score selection, correlation calculation and any fitting/mapping procedure,
   or the exact public artifact containing these results.

The archive available to me contains 310 clips: 31 references and 279 distorted
videos. I have not assumed that my selected 27-video subset is the paper's
original evaluation cohort. I am retaining the UPIQ-calibrated model without
fitting it to LIVE subjective scores.

Thank you for your help.

Best regards,
Deniz Aksoy
