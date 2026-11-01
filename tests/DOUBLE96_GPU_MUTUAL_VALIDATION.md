# Double96 GPU mutual masking

Validated2026-10-29. Artifacts:
`validation/double96_mutual_completed_path.txt`.

Runtimev3 adds GPU3x3 mutual masking for the fixed default quality/sp0 profile.
It computes min(abs(reference),abs(test)),then a zero-padded3x3box mean.
It preserves the native fx-ascending/fy-descending order,FMA for source rows
inside16-row blocks,and separately rounded multiply/add for the remaining
rows and border. The coefficient is the correctly rounded Double1/9 literal.
This matters particularly for24x24tails and the entirely non-FMA12x12band.

The production backend owns an optional9th pipeline and reuses its existing
buffers/descriptors. It supports the actual96/48/24/12square band sizes,
validates shape/finiteness,and exposes an optional metric callback. The API
validates returned masks and uses them directly in neighbor/noise/quality
processing. Other masking/quality operations remain CPU. Existing runtime
v1/v2 routes use their prior CPU mutual mask and remain supported.

-193,104 official native Double self-mask values across9configurations and
  five bands match bitwise. Saved row-major arrays compare bytewise after
  the native column-major layout conversion.
-64,368 CPU-oracle control comparisons pass,covering sparse impulses,zero
  border,signs,large/tiny values and FMA/tail behavior.
-5stage-invalid cases reject(geometry,rectangular,shape,NaN,Inf).
-9publicCLI v3 configurations match all72quality Double scalars;the existing
  13CLI invalid-case checks pass and decoded inputs remain exact.
-7v3package failures reject beforeGPU creation:missing mutual,hash/magic
  corruption,v2header with extra mutual,duplicate,missing file,missing projection.
-v1andv2OLEDstress regressions each preserve quality and both input exports
  byte-for-byte. All accepted Vulkan logs are clean.

These are the same two distorted pairs+identity control and three spectra as
prior validation,not additional real-image/profile coverage. Stage and CLI
counts are different scopes and should not be added as independent content.
NVIDIA RTX A500,Float64/RTE64/SignedZeroInfNan64;no denorm64 guarantee.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUMasking
```

-GPUMasking implies-GPUProjection and createsv3with15payloads(9shaders+6model
files). Default builds remainv1;-GPUProjection alone producesv2. The first
package attempt missed the implied projection;the builder was fixed and all
assets rebuilt fresh into accepted_pack/runtime_repro/runtime. The incomplete
attempt is preserved and is not an accepted runtime. PROFILE metadata was
clarified after numerical runs and its inventory refreshed;payloads unchanged.

CMake target double96-mutual-test accepts<v3-runtime> <native-cpu-root>
<fresh-output>. Audit:node tests/hdrvdp3_evaluation/audit_double96_mutual.cjs
<root>. MainRelease and stage builds pass. No shaders_out or old reference
changes. FFTs,projection and mutual mask are GPU;calibration,EOTF/log/exp,JND,
pyramid,neighbor masking,nonlinear quality and pooling still need GPU work.
The fixed-profile CLI is usable;fullGPU/generalHDR-VDP-3 completion is open.
