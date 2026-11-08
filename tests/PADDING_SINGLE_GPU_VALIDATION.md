# GPU FFT input padding - 2026-11-07

Implemented row-major Float32 plane to column-major doubled postpadding, matching
the existing optical Single CPU helper. Symmetric mode mirrors the last edge;
replicate mode repeats the last edge. Indexing is exact integer arithmetic and
uint storage copies preserve signedzero andsubnormal bits. No FFT arithmetic or
DC modification occurs in this stage. Dimensions are bounded and device storage/
dispatch limits are checked. Input finite, <=4K pixels; output4xinput pixels.

Compiled shader/spirv-val and strictMSVC probe passed. NVIDIA RTX A500 validation:
20cases,66,680,056 Float32 values EXACT. Cases include1x1,one-dimensional,odd,
portrait/landscape,workgroup tails,full3840x2160,randomfinitebits,signedzero and
subnormals, each tested in symmetric andreplicate modes. No reported Vulkan
validation errors. Artifacts:validation/padding_acceptance_20261107_run01 and
validation/padding_shader_20261107_run01 (shader/log/source SHA snapshots).

Optional SinglePaddingBackend added to optical/adaptation APIs. Empty provider,
incorrect output shape andnonfinite results fail; no fallback on provider errors.
Experimental CLI --padding-shader counts8dispatches/imagepair:3optical channels
plus1adaptation plane for each image. Defaults remainCPU andfull_gpu=false.
End-to-end integration PASSED16/16:14synthetic settingcases(12distinct PNGpairs)
and2real4K pairs, Q/Q_JOD bits identical to preserved independent official scores.
Each pair used2L+M and8padding GPUdispatches; otherGPUcounts andnoVulkanerrors
verified, runtime posthashes passed. Artifact:
validation/padding_integration_20261107_run01/checkpoint.json andcompletion_audit.json.
This is not fresh official execution or a fullGPU FFT/metric acceptance.

Host uploads/readbacks remain synchronous. API allocates a4byte unused binding1
buffer to share its three-binding layout; actual shader accessesonlybindings0and2.
This is preparation for GPU FFT, not a GPU FFT implementation.
