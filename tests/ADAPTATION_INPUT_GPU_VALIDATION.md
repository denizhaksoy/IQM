# GPU adaptation input - 2026-11-07

Vulkan L+M adds two clamped Float32 LMSR planes. One precise addition per pixel,
separate buffers,2D workgroup dispatch, <=4K pixels, finite[1e-8,1e10] inputs.
Standalone NVIDIA RTX A500 validation passed9cases/8,394,980values by exact
Float32 bit comparisons, including4K,workgroup tails,exponent ranges anddomain
rejection. Shader compilation and spirv-val passed. Artifacts:
validation/adaptation_input_acceptance_20261107_run01 and
validation/adaptation_input_shader_20261107_run01 (source/hash snapshots).

Integrated as optional SingleAdaptationInputBackend, requiring explicit matrices;
empty callback,wrong shape/nonfinite/out-of-domain output fail without fallback.
Experimental CLI option --adaptation-input-shader reports2dispatches per imagepair.
The new API defaults retain CPU behavior. Allfour CLI targets built successfully.
Preserved independent official scores matched16/16cases (14synthetic settings and
2real4K pairs), with2L+M dispatches andallprevious stage counts checked. No reported
Vulkan errors. Evidence:validation/adaptation_input_integration_20261107_run01.
The initial football150 official outlier remains unresolved; its test here uses
the explicitly named later clean stock score, never replaces original evidence.
Four old CLI modes also matched the reference on one portrait pair with optional
L+M disabled:validation/adaptation_input_compatibility_20261107_run01.

This is not fullGPU:FFT,model/LUT preparation andothermath remainCPU; stage API
still uses synchronous host transfers. Source PNGs/layout/Single precision/model/
PPD/padding/DC match the preserved reference cases; no new official scores were
claimed during these integration checks.
