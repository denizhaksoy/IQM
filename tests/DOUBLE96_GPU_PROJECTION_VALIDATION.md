# Double96 GPU optical/spectral projection

Validated2026-10-29. Artifacts:
`validation/double96_projection_completed_path.txt`.

The optional v2 Double96 runtime now runs optical RGB clamp(1e-5..1e10),
DoubleRGB->LMSR projection,LMSR clamp(1e-8..1e10),andL+M adaptation-input sum
on GPU. The projection preserves one separately rounded first product and
nestedDoubleFMA(green,...),FMA(blue,...) order. Calibration coefficients are
still generated on CPU from raw model data;Doublelog remains CPU. No Float32
transcendental substitution is used.

SRGBQuality96BackendDouble has an optional project callback. The metric API
skips its CPU optical clamp when this callback exists,and consumes the GPU
clamped response/log result. The production GPU context owns an optional8th
pipeline and separate82944-word output buffer. Existing7shader constructors
remain valid and select the original CPU projection path. Public CLI passes
the runtime projection module and records GPU spectral-projection yes/no.

Runtimev1 retains13payloads. Runtimev2 requires projection as14th payload,
verified by SHA256 and SPIR-V header checks. The package builder adds it only
with -GPUProjection. Its shader compile uses the existing fresh-directory
compiler,explicitRTE64/SignedZeroInfNan64 andspirv-val. No shaders_out edit.

Results:
-18native sides(2distortedpairs+identity,3spectra):1,658,880 Double values exact
  for response,clamped,L+M input and CPUlog boundary. Saved response/clamp/input
  binaries also byte-equal the official captures.
-276,480 additional control comparisons across3spectra include negative/zero,
  optical lower/upper boundaries and values above1e10,with CPU oracle applying
  optical clamp before the already accepted projection. All bitwise exact.
-3invalid-stage inputs reject(empty,NaN,Inf).
-9publicPFM CLIv2 configurations:72quality Double scalars exact;13CLI negative
  checks pass,including big-endian/scale2 PFM decoding in the positive scope.
-6v2package failures reject beforeGPU creation:missing projection,bad digest,
  bad magic with updated digest,v1header with extra projection,duplicate entry,
  missing payload.
-v1OLEDstress CLI regression:quality and both input exports byte-identical;
  profile metadata records projection no. Newv2 records yes.
-All accepted Vulkan logs have no VUID/[ERROR]. MainRelease and stage test build.

The first package attempt included projection but retained a v1header;loader
correctly rejected it. The builder was fixed and rebuilt into a fresh final
root;failed package/runs preserved. Accepted package is final/runtime_repro/
runtime under the root. PROFILE.txt metadata was clarified afterward without
changing any numerical payload;its hash inventory was refreshed.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUProjection
# Use <fresh>/runtime with the existing --hdrvdp3-encoding srgb-display command.
```

CMake tests/hdrvdp3_double_gpu target double96-projection-test accepts
<v2-runtime> <native-cpu-root> <fresh-output>. Saved-output audit:
node tests/hdrvdp3_evaluation/audit_double96_projection.cjs <root>.

Scope remains96x96/30ppd/age24/quality/defaultsp0,NVIDIA RTX A500 with accepted
Float64 controls;denormPreserve64 unavailable. Native validation reuses the
prior corpus and overlaps previous value coverage. Most comparison values
are GPU arithmetic;the adaptation log boundary explicitly remains CPU.
Display/EOTF,calibration,log/exp,JND,pyramid and remaining masking/pooling are
not yet fully GPU. This does not finish whole-metric/general-profile parity.
