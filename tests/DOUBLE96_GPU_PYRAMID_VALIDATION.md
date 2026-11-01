# Double96 GPU SP0 pyramid

Validated2026-10-29. Artifacts:
`validation/double96_pyramid_completed_path.txt`.

Runtimev5 adds a complete DoubleSP0 GPU pyramid stage:high/low/orientation
filtering,reflect1 borders,downsampling,five band normalizations,compensated
base mean andDC removal. The380coefficients are generated from the existing
source model on CPU;no native coefficient/intermediate file is supplied.
Filtering preserves reversedfx/fy traversal and the native16-rowFMA/separate
tail split. Normalizations by1/2/4/8/16 are applied after the corresponding
band calculation;raw low-pass intermediates stay onGPU for the next level.

The12x12base mean is a serial GPU Neumaier reduction in native column order,
with explicit separately rounded additions,followed by the previously tested
FMA remainder-corrected division for divisor144. GPU subtraction performsDC
removal. This preserves the native algorithm;it is not a parallel reassociation
or a proof of arbitrary-input correctly rounded GPU division. Normalized
bands,base_pre andbase_mean are returned to CPU because later operations
still need them. No CPU filtering/normalization/mean/DC arithmetic is used.

The production backend owns an optional11th pipeline;the quality API selects
its optional pyramid callback and validates returned dimensions/finiteness.
PublicCLI profile records GPU pyramid yes/no. V5requires17payloads(11SPV+
6rawmodel files);v1-v4 retain their previous CPU pyramid path.

-388,818 official native Double values across18sides match bitwise:all5bands,
 normalized base beforeDC,andbase mean. All126saved arrays/scalars also compare
 bytewise after layout conversion.
-86,404 CPU-oracle comparisons match for corner impulse,center impulse,
 constant andsigned patterned inputs,includingmean/DC.
-3invalid-stage cases reject(empty,NaN,Inf).
-9publicPFM CLIv5 configurations match72quality Double scalars,decoded inputs
 and13existingCLI negative cases. Allprofiles confirm GPU pyramid selected.
-9malformedv5packages reject beforeGPU,covering pyramid corruption/missing,
 profile/key/path/dependency problems.
-v1-v4OLEDstress regressions each keepquality andtwoinputexports byte-identical;
 allaccepted Vulkan logs have noVUID/[ERROR]. MainRelease/stage builds pass.

The same two distorted pairs+identity control andthree spectra are reused;
these are overlappinghistoricalcoverage,notnewimages. Scope96sq/30ppd/age24/
qualitydefaultsp0,NVIDIA RTX A500 withFloat64/RTE64/signedzero64 andno
DenormPreserve64. No generalgeometry/viewing/device orfullGPUmetric claim.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUPyramid
```

-GPUPyramid implies-GPUNeighbor,masking andprojection. Defaultbuilder remains
v1. CMake targetdouble96-pyramid-test accepts<v5-runtime> <native-cpu-root>
<fresh-output>. Audit:node tests/hdrvdp3_evaluation/audit_double96_pyramid.cjs
<root>. PROFILEmetadata clarified after runs;payloads unchanged,inventory
refreshed. No shader_out oroldreference changes.

RemainingCPU work includes display preparation/calibration/padding,Double
log/exp,JND,CSF/comparison andnonlinearquality/pooling. Next:comparison-mask
arithmetic/resizing orJND stages,withDoublethreshold/division/log semantics
validated before changing production behavior. Thefixed-profile hybridCLI is
usable;thewholemetric is notyetentirelyGPU orfullygeneralreferencecompatible.
