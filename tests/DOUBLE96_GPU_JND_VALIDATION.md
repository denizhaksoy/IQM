# Double96 hybrid JND GPU linear stages

Finalized2026-10-30. Artifacts:`validation/double96_jnd_completed_path.txt`.

Runtimev7 adds GPU pupil multiplication,binary upper_bound LUT search,
separately rounded Double slope*offset+table interpolation,zeroS plane and
(L+M)+R response. CPU retains geometric mean(log/exp),pupil model,LUT/slopes
and per-pixelDoublelog10 queries. Actual GPU post-pupil values return to CPU
for those queries,then GPU lookup consumes them. This is explicitly partial
JND GPU processing,not GPU transcendental evaluation or wholeGPU completion.

Original source-generated2048-node JND LUT is used. GPU binary search and
last-node clamp match the native interval selection. CPU precomputes slopes
using the original table/grid differences and Doubledivision. GPU prevents
FMA contraction in interpolation. Rod lookup uses input channelR(index3),
notS(index2);outputfourthplane is+0. Profile is96sq/30ppd/age24/defaultsp0.

The production API has an optional JND callback and validates all returned
arrays/scalars. Backend owns optional13th pipeline. Runtimev7requires19
payloads(13SPV+6model);v1-v6 retain priorCPU JND. PublicCLI metadata states
GPU JND-linear yes;CPU log/exp queries. -GPUJND includes all earlierGPU stages;
default package build remainsv1.

Accepted stage_fullgrid results:
-1,492,992 GPU array values(post_pupil,JND channels,P) match native captures
 bitwise across18sides;36CPU geometric-mean/pupil scalars also match.
-414,720 GPU array control values and10CPU scalars match the independent CPU
 function. Five controls cover constant,lower/upper clamp,channel layout,
 exact interior-grid query,and a sweep over all2048grid nodes.
-Total1,907,758 comparisons;GPU arrayvalues1,907,712 andCPU scalars46. Counts
 overlap earlier inputs;not additional images/general-profile coverage.
-8invalid-stage cases reject(shape,nonpositive/nonfinite input,invalid LUT).
-9publicCLI v7configs match72quality Double scalars,decoded input bits and
 the13CLI negatives. Eleven malformedv7packages reject beforeGPU creation.
-v1-v6OLEDstress regressions each preserve quality andtwoinputexports exactly.
-Allaccepted Vulkan logs are clean;mainRelease/stagebuilds pass.

Initial stage(four controls) also passed;it remains saved but is overlapping
coverage. Final stage_fullgrid added the full LUT sweep. Source/binary for the
four-control run are preserved separately. Runtime metadata was clarified
after tests;payloads unchanged and hashinventory refreshed.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_double_gpu/build_srgb96_double_runtime.ps1 -OutputDirectory <fresh> -RawData C:/HDR-VDP-3_Ref/data -GPUJND
```

CMake targetdouble96-jnd-test accepts<v7-runtime> <native-cpu-root> <fresh-out>.
Audit:node tests/hdrvdp3_evaluation/audit_double96_jnd.cjs <root>.
NVIDIA RTX A500 Float64/RTE64/signedzero64;denorm64preservation unavailable.
No arbitrary LUT/subnormal/device/viewing/geometry/fullGPU guarantee.
RemainingCPU:EOTF/calibration/padding,log/exp queries,CSF andnonlinear quality/
pooling. Next:CSF resize/clamp/interpolation/neural-noise arithmetic,keeping
model generation andunverified transcendental operations explicitly CPU.
