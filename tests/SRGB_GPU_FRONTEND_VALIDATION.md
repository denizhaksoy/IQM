# Original-input sRGB hybrid quality with all FFTs on GPU

Validated2026-10-29: all9configurations pass with zero bit differences,
18Q/Q_JOD values exact and no Vulkan VUID/[ERROR].

Artifact pointer: `validation/srgb_gpu_frontend_completed_path.txt`.
Profile:96x96 Single encoded RGB,30ppd,age24,defaultsp0;three display spectra.

`srgb-gpu-frontend-test` (output filename
`srgb-gpu-frontend-allfft-test.exe`) connects independent CPU display
preparation to the accepted Double GPU convolution chain. Three optical
channels are convolved and cropped on GPU, then CPU spectral projection and
clamp generate log adaptation from those actual GPU outputs. The log field
is independently replicate-padded and convolved/cropped on GPU. CPU adaptation
exp, JND, pyramid, masking and quality calculations consume these results.
The base12 frequency-selection FFT also runs on GPU. No native FFTW DLL is
loaded or used by this final diagnostic. Frequency selection after the FFT
and the remaining non-FFT calculations stay on CPU.

Official captures supply only comparison values. The original Single encoded
images and raw spectral data are model inputs. Padded images, spectral
filters, spectral matrices, LUTs, visual/quality maps and base frequency are
independently generated. No captured padded input or FFT/intermediate array
is passed into the calculation. Optical padding is symmetric postpadding;
adaptation padding is replicate postpadding;192sq Double column-major FFT
layout,DC unchanged,inverse divisor36864. Existing corrected Double division
and real-spectrum signed-zero semantics are preserved.

Final acceptance covers stress, ramps and identity (two distorted pairs plus
an identity control, not three independent photographs),three spectra each:
9configurations /18Q andQ_JOD values. The audit checks all convolution inverse
and GPU-crop bytes, visual/quality comparison counts, original reference
scores and Vulkan validation logs. The exact reported results are saved in
results.json and final_audit_PASS.txt. Frontend/quality comparison scope is
17,457,408 Double values;663,552 crop values overlap inverse coverage.

Reproduce in a fresh directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/validate_srgb_gpu_frontend.ps1 -Executable <build>/Release/srgb-gpu-frontend-allfft-test.exe -CpuRoot <srgb_csf-root> -EncodedRoot <srgb_double-root> -ChainRoot <double192_chain-root> -BaseRoot <double_gpu-root> -OutputDirectory <fresh>
node tests/hdrvdp3_evaluation/audit_srgb_gpu_frontend.cjs <root>
```

Build source directory:tests/hdrvdp3_double_gpu;target:srgb-gpu-frontend-test.
The executable takes native-oracle directory,fresh output,a reserved legacy
`unaligned` token,raw spectral data,original encoded inputs,six convolution
shaders and the base12 shader. The token does not select any CPU FFT plan.
The validation runner has a syntax check; the saved acceptance uses the
same executable argument sequence directly. The runtime module sample in
the saved root was collected separately during the stress run.

Float64 is explicitly selected on NVIDIA RTX A500 with validation enabled.
RTE64 and SignedZeroInfNanPreserve64 are explicit in accepted shaders.
DenormPreserve64 is unavailable;this does not prove arbitrary-subnormal,
other-device,other-size or other-viewing-condition parity.

This is a diagnostic integration, not the public metric runtime. Remaining:
extract a reusable fixed-profile metric API and wire runtime/CLI with original
image inputs and explicit supported-device/profile checks;then move remaining
CPU visual/quality calculations to GPU, preserving bitwise behavior. Other
prediction modes and broader geometry/viewing/device coverage remain open.
Existing production paths,old references and shaders_out are preserved.
