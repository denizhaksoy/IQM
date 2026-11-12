# Frozen LIVE HDR acceptance selection ? 2026-11-12

Frozen BEFORE numerical evaluation. No replacement or selection change based on results.

|Content|Distorted video|Resolution|Metadata bitrate (bps)|Frames|Samples|
|---|---|---|---:|---:|---:|
|Bonfire|540p_2.2M_Bonfire|960x540|2198781|527|18|
|Bonfire|1080p_1M_Bonfire|1920x1080|1035763|527|18|
|Bonfire|4k_3M_Bonfire|3840x2160|3258231|527|18|
|CargoBoat|540p_2.2M_CargoBoat|960x540|2243250|650|22|
|CargoBoat|1080p_1M_CargoBoat|1920x1080|1015585|650|22|
|CargoBoat|4k_3M_CargoBoat|3840x2160|3021739|650|22|
|Daylight1|540p_2.2M_Daylight1|960x540|1661187|480|16|
|Daylight1|1080p_1M_Daylight1|1920x1080|892939|480|16|
|Daylight1|4k_3M_Daylight1|3840x2160|2730759|480|16|
|football1|540p_2.2M_football1|960x540|2233752|506|17|
|football1|1080p_1M_football1|1920x1080|1013785|506|17|
|football1|4k_3M_football1|3840x2160|3072188|506|17|

12pairs/219sampled frames. All references are3840x2160. HEVC Main10 archives, YUV420p10 limited-range BT2020nc/PQ, archivebytes pinned. Nominal labels are target bitrate names, not inferred QP/GOP/encoder settings or quality ordering. Exact headers and actual bitrates are retained in the manifest.

Sampling: zero-based0,30,60,... strictly below framecount. Expected per-content samples18Bonfire/22CargoBoat/16Daylight1/17football1, repeated across3distortions. Rational frame rates are pinned from stream headers; header/decoded counts and relative clock grids must agree exactly. RGB48 decode via existing pinned helper, no edit-list/seek change. Identical PNG bytes go to both implementations.

Profile: final clean-build IQM.exe live-hybrid; quality/sp0, Single images, BT2020PQ, age24/OLED/PPD62.729130063099447, ambient200lux/reflection.005/peak2000. Distorted image bicubic-resized/clamped in PQ to reference geometry before inversePQ. CPUFFTW3threads/estimate. Every frame gets NEWofficialv3.0.7 and NEWfinalIQM scores; no cached quality predictions. Compare both Single Q and Q_JOD bits. Ordered Single video-mean aggregation matches the already-declared local protocol; it is not a claim about an undocumented author aggregation precision.

Selection rationale: daylight, fire texture, moving vessel and sport motion;3archived resolution/compression combinations per content. Light3 is explicitly outside these4contents and remains an OPENalignment issue, not a passed or repaired case. football1 ISincluded, including540p frame150. Its original40ec4f50/41037c75 versus later40ec7071/41038aea discrepancy stays OPEN even if the current frozen reference agrees. No voting, replacement or tolerance.

A mismatch halts the bounded run, preserves PNGs/decode logs/reference outputs/IQMscore/basefrequency/dispatch/log outputs and requires first-stage localization before anyfix. No modified official implementation or algorithm. This is not the279video run, benchmark, fullyGPU implementation or privatecompetition reproduction.

Frozen artifact: C:/Users/DenizAksoy/OneDrive - Tieto/Desktop/IQM/validation/hdrvdp3_live_acceptance_frozen_20261112_run01/manifest.json with manifest.sha256 and mediaheader files. The runner checks source/media/package/tool hashes and unmodified official tree. Runtime/commands/per-frame comparisons and per-video report will be retained in a separate fresh output directory.
