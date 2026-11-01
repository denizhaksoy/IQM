# Double96 GPU neighbor masking

Validated2026-10-29. Artifacts:
`validation/double96_neighbor_completed_path.txt`.

Double96 runtimev4 adds GPU neighbor-mask processing. Existing source
make_resize_axis_plan generates Double bicubic weights,antialias factors,
symmetric source indices and normalization on CPU. The GPU performs each
horizontal/vertical weighted sum with separate Double multiplication/addition,
then max(resized,0) and lower-neighbor-before-upper-neighbor accumulation.
Same-size neighbors skip resizing exactly as the CPU implementation does.
No hardware texture interpolation and no CPU intermediate between axes.

For this fixed profile,all actual resize ratios are1,2or0.5 and the generated
normalizations are exactly1. The production backend checks that fact before
upload;this does not establish genericDoubleGPUdivision/resize parity.
Internal submissions serialize coefficient/input updates. Only the completed
neighbor result returns to CPU;weights remain source-generated model inputs.

An optional10th shader pipeline and optional neighbor callback are used by
the production quality API and public CLI. V4requires16payloads(10shaders+
6model files). V1/v2/v3 remain supported and use their prior CPU neighbor path.
CLI profile records GPU neighbor-masking yes/no. Build with-GPUNeighbor;
it implies-GPUMasking and-GPUProjection. Default package build remainsv1.

Results:
-193,104 official native Double neighbor values across9configurations/five
 bands match bitwise. Saved row-major arrays byte-match native column-major
 arrays after layout conversion.
-64,368 CPU-oracle controls match,covering impulses,borders,smooth values,
 large/tiny values,same-size,upsampling,antialias downsampling and sum order.
-6stage-invalid cases reject(missing masks,badband,badshape,negative,NaN,Inf).
-9publicPFM CLIv4 configurations match all72quality Double scalars,decoded
 inputs and13existingCLI rejection cases.
-8v4package cases reject beforeGPU:missing neighbor,digest/magic corruption,
 v3header extra neighbor,duplicate,missing file,missing projection/mutual.
-V1/v2/v3OLEDstress regressions each retain byte-identical quality and two
 input exports. All accepted Vulkan logs have no VUID/[ERROR].

The corpus remains two distorted pairs+identity and three spectra,96sq,
30ppd,age24,quality/defaultsp0,NVIDIA RTX A500 Float64/RTE64/signedzero64.
Counts overlap historical stages and do not add new image/profile coverage.
DenormPreserve64 is unavailable. MainRelease and isolated-stage builds pass.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUNeighbor
```

CMake target double96-neighbor-test accepts<v4-runtime> <native-cpu-root>
<fresh-output>. Saved audit:node tests/hdrvdp3_evaluation/audit_double96_neighbor.cjs
<root>. Runtime metadata was clarified after numerical runs;payloads unchanged
and hash inventory refreshed. No shaders_out or old reference modifications.

FFTs,convolution,projection,mutual and neighbor processing are now GPU in v4.
CPU display preparation/calibration/weights,log/exp,JND,pyramid,CSF and nonlinear
quality/pooling remain. This is a usable fixed-profile hybrid metric,not full
GPU or generalHDR-VDP-3 parity. Next:DoubleSP0pyramid filtering with native
operation order,normalization andDC processing preserved.
