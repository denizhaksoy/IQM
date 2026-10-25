# IQM HDR-VDP-3 session handoff

Last updated: 2026-10-25 (Europe/Prague). Branch: `feature/hdr-vdp3`.

## Current checkpoint (2026-10-25, Europe/Prague)

This checkpoint supersedes the stale running-session notes below.
- All six CPU-filter replays completed: asymmetric/impulse/hdr_ramp, 80x96
  and96x80. Evidence: validation/actual_content_stress_639284716222632999/
  summary_cpu_filters/PASS.txt and runs_cpu_filters.json.
- Original CPU create_jnd_lut had60 Float32 rod-slope mismatches per image.
  Failed runs remain preserved. Native symmetric logspace, log10 coordinates,
  per-sample Jacobian and cumtrapz prefix operation order fix now applied to
  src/hdrvdp3/jnd.cpp; strict FP options added in production/evaluation CMake.
- Production CLI regression PASSED11 PFM pairs,1166 numerical exports and
  exact quality files. Root validation/production_jnd_order_639284726753249859/
  cli_regression/PASS.txt. Previous session88096 no longer exists; result is saved.
- verify_jnd_model_words.cpp now supports --strict-double, which fails on any
  double node mismatch as well as Float32 node/slope mismatches. Actual
  production source passes10238 Float32 words and6144 double words exactly:
  cpu_jnd_model_639284722715977911/production_strict_double_verified/PASS.txt.
  Initial production_strict_double used wrong fixture folder and failed;
  retained, not a numerical pass.
- Production-linked pathway-jnd-test rebuilt in cpu_jnd_model.../build/Release.
  New HDR replay uses --cpu-lut (including CPU D65 constants), with generated
  CPU filter coefficients. Active root pointer:
  validation/production_cpu_jnd_replay_active_path.txt. Session37113 running.
  Exact next step: poll37113, inspect all stages/final score bits; preserve any
  failure and diagnose without relaxing equality. Then freeze source/binary/
  asset hashes, update docs and this checkpoint with final outcome.
- Still experimental file/host pipeline, not resident public API integration.
  Public geometry guards unchanged; no general parity claim. No commit/push.
## Latest continuation: residual120 composition, base120 and real pathway traces

### Current continuation: experimental original-image ->quality replay passes

LATEST UPDATE: HDR/replay_cpu_filters finishedPASS(allstages/finalscorebits).
Sixpairs acrossasymmetric/impulse/HDR andbothorientations nowpass;initial
aggregate actual_content_stress/SUMMARY.json/PASS exists. New
summarize_actual_gpu_replays.ps1 providesdurable stage/scorebit/modelaudit.
To pinALLsixpairreplays tolatestC++model-filter source/binary, currently
running asymmetric/replay_cpu_filters via execsession49777. Afteritpasses,
run impulse/replay_cpu_filters withimpulse/assets_cpu_filters.json;HDR already
usesCPUmodelchecker. Then runsmanifest+freshsummaryallCPUprofiles, freeze
source/assets/binary/referencehashes/completedpointers andupdate thisrecord.

- User asks continuous autonomous work,without stopping after every update.
  Quota is not visible; no claim to monitor it. Alllocal,noautopublication.
- Newpathway_jnd_639284560688608036: verify_pathway_jnd.cpp +isolatedtarget.
  Native-retinal/pupil48checks368640 pass; GPU7680samplepupil52/368656 pass;
  GPUretinal+GPUadaptation60/430096 pass. Existingmap13 supports7680pupil,
  sequentialFLOAT32logsum/geomean; doNOTusemean120doubleaccumcontracthere.
  NativeLUT/matrixfixedmodel only. PriorcheckerEXE/sources retained.
- actual_pathway_filters_639284562227581717: new mixed_filter_io.comp,
  generator/targetcompile script andverify_mixed_filter_chain.cpp/target.
  SSBOpad/log ->acceptedmixedFFTforward ->exactmultiply ->acceptednormalized
  inverse30720 ->crop/clamporExp.7dispatchesONEcommandbuffer. Actualreference/
  localizedtest nativefilterrefs assertretinal/adaptation againstfullvisual
  trace.8cases2027520values pass;4LAfromGPUadaptinput reruns also pass.
- Actualimage-dependentP/log nowcomputedGPU,notloadednative. Prepare-onlyJND
  stage uses ONLYGPUMTFretinal+fixedmatrix forLAinput; unusednativepupil/P
  are skipped. FinalJNDpupil/adaptation come fromGPU LA outputs.
- Manualassembly root actual_gpu_composition_639284565879130482: new
  prepare_gpu_visual_composition.m serializesGPU P/log withoutimage math,
  nativeSPYR/maps copiedasgoldensonly. InitialOctavecopyfilefailedWindowsxcopy
  prompt;fixeddirectbytecopy,freshinputs_verified accepted,failedinputskept.
  SPYR113526,pool71520,freshcomparison90cases,andquality126/393384allPASS.
  validate_actual_gpu_composition.m verifiesoriginalfilter/comparisoninputs
  bit-identical,fixedmatrix/JNDtables sameacrossimages,finalscoresBITWISE.
- Newrun_actual_gpu_composition.ps1 andassets.json provideONENEW-DIRECTORY
  command replay. FULLFRESHREPLAY actual_gpu_replay_639284715589047637/PASS:
  alloriginalimage->GPUfilters/JND/P/log/SPYR/masks/qualitystages PASS.
  Nativeimage-dependentmaps are goldensONLY; fixedmodeltables/axisplans and
  existingCPUdoubleNCSF intentional. File/hosttransfers remain,notresidentAPI.
  No publicshape/productionalgorithmchanges. IntelArcPro,30ppd/age24/D65/
  singlepositive luminance/sp0Filters only. FinalQ/JOD bitwise same asnative.
- cpu_model_filters_639284717818610970: mixedfilterchecker nowC++generates
  MTFandGaussian coefficients, nativecoeff onlybitgoldens. Freshbinary,
  4orientation/mode cases each30720coefficientbitmatches+253440GPUstagevalues
  exact. Oldnative-coefficient binaries retained. No model-filter fallback.
- Contentstress root actual_content_stress_639284716222632999: nativevisual
  --pattern impulse andhdr_ramp addedwithoutalteringdefaultasymmetric. Fresh
  visual/stages/SPYR/masking/comparison/residual/filtergoldens generated and
  stock/tracednativeQidentity verified. exporter--new-fixture explicitlyskips
  inappropriatecross-contentPidentityclaim. Impulse/replay/PASS COMPLETED.
  HDR/replay_cpu_filters INPROGRESS via exec session69768, assets usesnew
  C++model-filterchecker. Poll sessionandfinishaudit/hashes/docs afterresult.
- DocsACTUAL_GPU_COMPOSITION_VALIDATION.md, MASKING_QUALITY_VALIDATION updated.
  Stillneedfreeze latestrootassets/source/binaryhashes andcompletedpointers.

Immediate next step: finish HDR stress replay,inspect everyPASS/result,count
andfinalbitcheck. Ifexact, record siximagepairs across3contents/2orientations;
ifnot, preservefailedstageanddiagnosewithouttolerance. Then durable runner/
artifact hashes/handoff. ProductionAPI/runtime integration andbroaderPPD/age/
content validation remain; doNOTenablegeneralpublicshapes orcommit/push yet.

### Newest: base noise from GPU data; upstream references prepared (2026-10-24)

- Root `validation/gpu_base_noise_639284555583605395`, active/completed
  gpu_base_noise pointers. Localexperimentalonly; no publicguard/algorithm/
  runtimeasset changes,no commit/push. Preservestagedcorrdn,dirtyVkFFT/oldrefs.
- verify_pathway_masking.cpp newargc11 profile: GPUresize/clamp logs ->accepted
  base120.spv mean/pow10; GPUCSFparams fromGPUluminance; residualindex read
  acceptedSPYRallchain stage10. Noise computed byEXISTINGCPUdouble
  evaluate_ncsf_parameters usingGPUderivedfrequency/params. NativebaseNoise
  no longerruntimeinput,onlygolden. NativeP/log maps andmodelconstants remain.
- Newprivate referenceGridFrequency preservesnativecoordinateoperationorder.
  IMPORTANT: productionquality.cpp foldedk/n helper differsfewdoublebits on
  10x12/12x10. DoNOTenablegeneralpublicshapeswiththatoldhelper. No production
  change made; experimentalhelper mathematicalimplementation independently
  validatedagainstnewnativegrid. generate_base_frequency_grid_reference.m
  outputs240values; --frequency-grid modebitwiseall240PASS frequency_check/.
  Sourcehashonly forofficialcreate_cycdeg_image;noinstalledreferenceedits.
- gpu/PASS/results:126checks393384values numericEXACT,observedbitdiff0;
  additions4GPUmean/powoutputs +6selectedindex/frequency/noise scalars.
  Double noiseexact, allprior122checks(D,Dp/errors/Q/JOD,etc)included. Device
  IntelArcPro. No numericalfailures. Basebounded120logs[-5,5],normal/zero,
  exponentmin-90,max100,spread16 checked. No secondDCremoval orpadding.
- Refactored runFloatKernel hasSEPARATEinput/outputbuffersizes; exercised
  bothDpandsmall2wordbaseoutput. NewstrictReleasebinaryfreshroot/build;
  oldchecker binariespreserved. No newshadercompile;reusesmap13,Dp,base120.
- Newnativeupstream root `validation/general_visual_trace_639284557380638502`.
  generate_general_pathway_reference.m optional--visual makeslicensedCOPIES
  ofhdrvdp3.m andhdrvdp_visual_pathway.m infreshvalidationdir,capturesretinal,
  LMSRmatrix/before/afterpupil,adaptinput/adaptation,pupil/area,JNDchannels,
  P/pnlookupcells. Bothstock/tracednativeQ/JODexact. Originalinstalluntouched.
- export_visual_pathway_stages.m writesstages/ perchanneltext17digits and
  metadata:retinal/adapt/LMSR/JND/P SINGLE,matrix/areaDOUBLE,pupilvectorSINGLE.
  NativeMATpreservesfullcells/arrays. stages/PASS asserts P_reference/P_test/
  log_La BITWISEsameasold general_masking_trace. TheseareNATIVE references
  ONLY,no newGPUupstreamacceptance. Same2pairs30ppd/age24,notnewcoverage.
- DocsGPU_BASE_NOISE_VALIDATION.md,MASKING_QUALITY_VALIDATION.md; hashes/
  completedpointerssetafterPASS. CompositionstillFILE/HOSTtransfers,
  CPUdoubleNCSFintentional;notfullmetriccomplete.

NEW exact next step: use general_visual_trace/stages toportupstream retinal
->adaptation andphotoreceptorP paths. Need7680samplepupilgeomean/reduction
withoutwideningexisting32/64publicpupilguard. Inspectnativepupil/areaand
pnlookups; compareGPUadaptation_input/JNDcomponents firstusingactualretinal
andnativepupilgoldens,thenreplacepupilwithbounded7680exactreduction.
ActualMTF/LA160x192/192x160filterstages neednewreferencesfortheseLOCALIZED
testimages(oldFFTfixturesusedtest*1.05). JoinacceptedmixedFFTwithactualfilters
andnativevisualretinal/adaptation. OnlyafterP/logproductionandfullnative
image-to-scorecomparisons considerpublicgeneralshapeprofile/guardchanges.

### Newest: GPU bands/self-mask/CSF replace native runtime maps (2026-10-24)

- Root `validation/gpu_band_masking_639284552580517314`, active/completed
  gpu_band_masking pointers. Alllocal, no commit/push/publicguard/algorithm
  changes. Preserveoldrefs,stagedcorrdn,dirtyVkFFT. DeviceIntelArcProGraphics.
- Newprepare_gpu_band_pool_inputs.m reads ACCEPTED GPU SPYR all-chain dumps
  pathway_spyr_all_chain_639284285430220524/gpu_all_chain:stages4..7 high/
  orientation pairs, stage8 mean+centeredbasepairs (skipmeans, noarithmetic).
  Asserts nativebitidentity,serializesGPUwords ->gpu_inputs/*band*.f32 and
  pool.txt runtimefields. Nativebands/raw/pooledmaps are GOLDENSonly; nohost
  filtering/normalization/DC/runtime maskcalculation inadapter.
- Reusedacceptedpool checker/shaders frompathway_resize_csf checkpoint;
  gpu_pool/PASS10bands71520values numericexact, observedbitdiff0. Runtime
  poolinputs nowGPUSPYRbands, notnativebands. Same actualimagepairs asbefore.
- verify_pathway_masking.cpp newargc8 profile loadsGPUreference/testbands,
  pooledhalfofGPUcase*.u32 outputs into caches, validatesagainstnativegoldens,
  thenUSES GPUvalues for banddiff,self/neighbor,N_mask,D,etc. Adjacentmasks
  alsoGPUcache. PriorGPUdoublecomparisonmask retained. Nativeband/selfruntime
  dependencyremoved,notfallback. gpu_masking/PASS112checks357614values exact;
  observedbitdiff0,Q/Q_JODbothnativepairsunchanged. Preservedexecutable/source
  band_self_only.exe /band_self_only_source.cpp beforeaddingCSFreplacement.
- Newargc9 profile loads ORIGINALlog_La map fromgeneral_masking_trace;
  GPUresize/clamp/sample_csf all5bandshapes,standardCPUCSFLUT30/2^b(age24).
  NativeCSFmaps nowGOLDENSonly,runtimeusesGPUCSF. gpu_masking_CSF/PASS:
  122checks393374values numericalEXACT,observedbitdiff0. Adds35760CSFvalues
  toprior112checks. No redundantbroadrepeat; finalrunincludespriorchecks.
- FreshstrictMSVCReleasechecker inroot/build, unchangedboundedABI13map and
  priorDp shader. No newshadercompile. Nativefixturepathway_masking/native_
  verified, GPUcomparison general_comparison/gpu. Input/expectedshapes pinned
  80x96/96x80,30ppd,age24,sp0Filters;normalizedbands /1/2/4/8/16,basecentered
  separately. NoextraDC/padding/filter, neighbor/CSFresize symmetric,poolzero.
- This isFILE/HOSTCOMPOSITION,notoneresidentfullGPUmetric. Runtime native
  P/log_adaptation,baseNoise andfixedparameters/LUTcoefficients remain. Base
  neural stillfloat(pow(nativeDoubleNoise,2*nativeP));doNOTclaimfullquality.
  Valuesinclude repeatedsource/goldenchecks,notnewuniqueimagecoverage.
- DocsGPU_BAND_MASKING_VALIDATION.md/MASKING_QUALITY_VALIDATION.md; inputs.json
  andcheckpoint_hashes bindGPUsourcefiles/pooloutputs,allreferences, binaries,
  source and unchangedshaders. CompletedpointeronlyafterbothacceptedPASS.

NEW exact next step: remove nativebaseNoise using acceptedGPU residual
selection(all-chain stage10 floatencodedindex), GPUresizedbase logs and
acceptedbase120 mean/powshader, then existing CPUdoubleNCSF contract with
GPUderivedfrequency/luminance. Assertallbaseintermediatesagainst native
base_selection.txt beforeusingnoise inD/quality. Keep CPUdoubleNCSF clearly
reported; no needportdoubleNCSFtoGPUforparity. Afterthat upstreamMTF/LA/
photoreceptorproductionofP andlog_adaptation stillrequirejoiningaccepted
mixedFFT/filter/JNDcomponents,thenfullimage-to-score/nativecomparison.

### Newest: general binary/double comparison mask and score composition (2026-10-24)

- Root `validation/general_comparison_639284533493306890`, active/completed
  pointer general_comparison_{active,completed}_path.txt. New
  generate_general_comparison_reference.m:2orientations80x96/96x80 x9patterns
  x5bandtargets=90cases. Actualpair+zero/one/corners/checker/random/stripes/
  threshold-adjacentFloat32. Nativebinary expression abs(test-ref)./ref>.001
  EXACT; freshnative_verified/ afterverbatimthresholdclarification;earlier
  native/ preserved. Actualdoublegoldens assertedBITWISEagainst fullmetric
  trace.masking comparison. Rowmajorinputu32/binaryinteger/decimaldoubleoutput.
- Newgeneral_comparison.comp, compile_general_comparison.ps1 injectspinned
  existingfiniteFloat32dividehelper frommapreference;explicitDenormPreserve/
  RTE32,spirv-valVulkan1.2;noFloat64/Int64. Newverify_general_comparison.cpp,
  CMakegeneral-comparison-test strictMSVCRelease freshbuild.
- ONEcommandbuffer3dispatches:binarymask ->horizontalQ20 ->verticalQ40+IEEE
  doublebitpacking. CPUuploads ONLY originalFloat32luminance andexactaxisplans
  inbuffer suffixes; GPUwritesmask/intermediateprefixes;nohostintermediate
  reads/calculation/upload. Hostguards exactQ20weights/unitnormalization/
  absoluteweightsum<=2/scales1,2,4,8. Firstabsnum<=2*2^20,second<=4*2^40 fits
  signed43magnitudebits anddouble53;nativepartials/exactconversion no rounding.
  Resize symmetricedges/bicubicantialias,noFFTpadding,nomean/DCprocessing.
- gpu/PASS/results.tsv IntelArcPro:90cases691200binarywordchecks+321840DOUBLE
  values BITWISE EXACT. Binarycalculation repeated5targets,18uniqueinputpairs
  /138240uniquemaskpositions;identitytargetsrepeat. DoNOTclaimunique691200.
  No numerical failures. Publicresize_comparison_maskguardNOTwidened.
- Extendedverify_pathway_masking.cpp optionalarg6 consumes generated
  orientation_p0_b*.u64 masks, asserts exactnativeDOUBLEbitidentity, thenUSES
  GPUderivedmask inDp/banderrors. Newcheckerbinaryinfreshcomparisonrootbuild;
  oldmaskingbinariespreserved. Inputs nativepathway_masking/native_verified,
  unchangedboundedABI13mapandpriorDp shader. gpu_masking_with_comparison/PASS:
  82checks250334values numericalEXACT,observedbitdiff0, nativeQ/Q_JODbothpairs
  unchanged. Includes35760bitwisedoublemaskcomparisons andprior72stagechecks.
- This composition usesFILE/HOST transfers, NOToneresidentfullGPUquality
  pipeline. Native self_masks,CSF,base_noise remainruntimeinputs. Nofullmetric
  claim. Earlier82scopecountsrepeatGPUmasks,notnewuniqueGPUcoverage.
- Docs GENERAL_COMPARISON_VALIDATION.md,MASKING_QUALITY_VALIDATION.md.
  inputs.json/checkpoint_hashes.json pin native/shaders/helpers/axisplanner,
  bothnewexecutables andcurrentchecker/source. Allpriorartifacts/stagedcorrdn/
  dirtyVkFFT preserved, no commit/push/publicguardchanges.

NEW exact next step: remove native self-mask andreference/testband inputs
frommaskingcomposition using accepted GPU SPYR all-band dumps and accepted
experimentalpool outputs. Preserve eachbandnormalization/baseDC semantics;
validate everyreplacement againstnative beforejoiningresidentbuffers. Then
replaceCSF/base-noise inputs viaacceptedGPUresize/CSF/base120 andexisting
doubleNCSFcontract. UpstreamMTF/LA/photoreceptorproductionofP stillpending;
full image-to-score/generalpublicquality NOT complete. Comparison-mask
generation/doubleresize is now an experimental componentpass,notpending.

### Newest: masking/D/D_p/quality components pass (2026-10-24)

- Root `validation/pathway_masking_639284292364400623`, active/completed pointer
  pathway_masking_{active,completed}_path.txt. Actualnativeinput component
  scope ONLY; no publicguards/productionalgorithms changed, no commit/push.
- New generate_pathway_masking_reference.m reads existing fullnative
  general_masking_trace_639284290290539478, exportsperbandself/neighbor/N_mask,
  excitation/neural/D/comparison/D_p/parameters,psychometric,error,scoremaps.
  Initialnative/ failedbecauseminkowski_sum private; freshnative_verified/
  usesexactreferenceformula sum(abs(Dp).^.8/numel).^(1/.8),assertsallnative
  accumulatedQerroridentities. Oldpartialnative/ kept. No referenceinstall edits.
- New verify_pathway_masking.cpp,CMakepathway-masking-test linksUNCHANGED
  map_gpu.cpp,resize.cpp,csf.cpp,quality.cpp,Vulkanhelpers. StrictMSVCRelease
  checkpoint/build. Earlybuildbeforeconfigurecompletion failed(defaultVS17);
  waitedconfigureVS18completionthenbuildPASS. No numericalmaskingfailures.
- Reuses ABI13 publish_prepare_639283319336040304/runtime_bounded/map.spv.
  Firstgpu/PASS62checks178814values exact. Finalgpu_with_Dp/PASS72checks
  214574Float32values exact, observedbitdiff0, IntelArcPro. Priorbinary saved
  masking_first_pass.exe beforeaddingdirectDp. Finalruncontainsalloldchecks,
  so no additionalunchangedbroadregression. Numericalacceptance,zerobitsnot
  required. Newdp shader Float32DenormPreserve/RTE32/NoContraction,spirv-val.
- For10bands: GPUresizeadjacentnativepooledself ->neighborclamp/add, N_mask
  fromnative self+GPUneighbor, normalizeddifference(test,ref,1), D fromGPU
  difference/N_mask+nativeCSF,psychometric,qualityerrors,finalQ/JOD. Base
  neural=float(pow(nativeDoubleNoise,2*nativeDoubleP)) CPU as existingcontract;
  nonbaseGPUcomputesneural. Self/N_mask/D/Dp nativeSINGLE, comparisonDOUBLE.
- DirectDp usesnew pathway_mask_product.comp andcompile script; checker
  downloadsDfromsynchronousMapGPU thenuploadsD+Float32castnativecomparisonmask
  toseparateGPUprecisemultiply. MatchesnativeDp; NOT residentjoinedpipeline.
  35760valueseach neighbor,N_mask,banddiff,D,psychometric,Dp +10errors+4scores
  =214574. Same twoimagepairs/30ppd/age24,notnewuniquecoverage. Q/Q_JODmatches
  dependonNATIVE intermediates; doNOTclaimfullGPUmetricacceptance.
- Allinput/textfiles rowmajor17digits, stagescarrynativeclassesinmetadata.
  Neighborresize nativeimresize symmetricextension, elementwisemasking has
  nopadding, no additionalDCremoval. Already-normalizedbands, centeredbase.
  Componentcalls usehostreadback/uploads; joiningSPYR/pool/CSFnotdonehere.
- Docs PATHWAY_MASKING_VALIDATION.md/MASKING_QUALITY_VALIDATION.md, hashes/
  completedpointerupdated. Preserveoldrefs,stagedcorrdn,dirtyVkFFT/allchanges.

NEW exact next step: general80x96/96x80 binarycomparison-mask generation and
nativeDOUBLE imresize for5bandshapes, using saved trace.masking{b}.comparison
asgolden. Existing resize_comparison_mask guard onlysquare/2:1; keepit until
separateexperimentalfixed-pointprofile validated. ThenjoinGPU SPYR/pooling/
neighbor/CSF/masking outputs, removingnativeintermediateinputs; upstream
MTF/LA/photoreceptorP stillneededforfullimage-to-scorequality acceptance.

### Newest: actual resize/CSF and experimental mutual pooling (2026-10-24)

Latest additional native reference: general_masking_trace_639284290290539478.
generate_general_pathway_reference.m now inserts read-only captures for
self/neighbor masks,N_mask,excitation,neural,D,comparisondouble,D_p and
accumulatedQerror. Savestrace.mat andperbandtext; nativeoriginal/tracedQ/JOD
stillEXACTbothorientations, officialinstallationunchanged. ActualGPU pooled
outputs independently compared to nativeFULLMETRICself_mask using new
validate_pathway_pool_trace.m: full_trace_pool_comparison/PASS35760BITWISE
words over10bands. This reusespreviousGPUoutputs,notnewuniquecoverage.
Othernewmaskingmaps are referenceONLY,notGPUvalidated. Latestgeneratorhash
differsfromhistoricalearliertracegenerators;oldartifacts/sourcehashespreserved.

Immediate next step updated: native masking instrumentation is DONE; use new
general_masking_trace/trace.mat for neighborresize/N_mask/D/D_p stagechecks.
Do not regenerate instrumentation unnecessarily. Need isolatedchecker linking
MapGPU resize,masking_denominator,difference,psychometric andqualitypool APIs,
without wideningmutual_masking orcomparisonmask guards. Nativeclasses and
base neural scalar vs per-pixel neural maps must remain explicit.

- `validation/pathway_resize_csf_639284286981781158`; actual twoimagepair
  trace.log_La/log_resized/csf and normalized/DC-centered bands from
  general_pathway_639284244350897941. New
  generate_pathway_resize_csf_reference.m verifies all10resizedlogmaps against
  nativeimresize/clamp andexports resize.txt,bounds.txt,pool.txt. Options
  maskingnorm0/pool3/noGaussian/nodilation asserted. Float32/30ppd/age24,
  row-major decimal17digits, imresizebicubicantialias/symmetricedges, noDC.
- Existing verified checker full_age_639280300414897968/verified_bin/
  IQM-map-gpu-test.exe +publishedbounded ABI13map from
  publish_prepare_639283319336040304/runtime_bounded/map.spv. No runtimechange.
  --resize gpu_resize.log:10cases35760values numericexact. --bounds
  gpu_resize_csf.log:20comparisons71520values numericexact(resize+CSF).
  These repeat35760resizevalues;NOTextraunique coverage. All5bandshapes,
  firsttwo same originalsize; basegenericCSF also tracedbutactualbasenoise
  usesselectedfrequency separately. DeviceIntelArcPro, numericalchecks only.
- Originalnative/ hadresize/CSF; freshnative_with_pool/ addspool. Oldresize
  andbounds file hashes identical acrossboth. Nativepooled=conv2(minabs,3x3/9,
  same),fromalready normalizedbands soinputnormalization1. Same sourcepairs.
- ProductionMapGPU.mutual_masking correctlyrejectsgeneralshapes:gpu_pool.log
  exit2 UnvalidatedGPUmaskingdimensions. GuardNOTchanged. Newisolated
  tests/pathway_pool.comp,verify_pathway_pool.cpp,compile_pathway_pool.ps1,
  CMakepathway-pool-test. StrictfreshReleasebuild incheckpoint/build.
- Initial destination-wideFMA/tail experimentalpass differs11on40x48case2
  atx31/32, preservedgpu_pool_experimental/pool_fma/pool_separate. Nativefull
  conv2 transitions dependonSOURCEaxisperTAP. MixedhardwareFMA/separate
  pertaptrial differs2494onfirst80x96case, preservedgpu_pool_per_tap/
  pool_tap_fma/separate. Corrected usesacceptedINTEGERpositive_fma helper
  fromshaders/hdrvdp3/hdrvdp3_map_reference.glsl (helpersinjected/hashes
  recorded) pluspreciseseparateproduct/add, per-source-tap32elementprefix.
- Accepted pool_integer_fma/separate shaders, gpu_pool_integer/PASS/results:
  10bands71520outputs numericexact, observedbitdiff0, IntelArcPro. Hostuploads
  onlynativebands, GPUcomputesrawminabs+pool, zeroedges/native longeraxisorder.
  Two variantsdisjointwrites;bothcontainpertapmixedARITHMETICwithintegerFMA.
  NoFloat64/Int64,explicitDenormPreserve/RTE32,spirv-val passes. Empirical
  dispatchmatchonfixtures, notcompilednativescheduleproofforallinputs.
- Docs PATHWAY_RESIZE_CSF_POOL_VALIDATION.md,MASKING_QUALITY_VALIDATION.md.
  Hashmanifest binds oldresizechecker/shader,newpoolsource/binary/shaders,
  nativefixtures andlogs. PASS markers forresize/CSF written onlyafterexit0
  andexact expectedcounts, completedpointerafterallthreeacceptedscopes.
  Allproductionfiles/guards, stagedcorrdn,dirtyVkFFT preserved;noautopublish.

NEW exact next step (supersedes olderparagraphs): instrument fresh native
hdrvdp3 copy toexport perband self_mask,N_mask,excitation/D_p maps andquality
pooling, preservingoriginal/tracedQ/Q_JODidentity. Extendexperimental masks
to neighborbandresize/denominator/difference/psychometric using actualmaps;
check each stageexactly. Nativecsf/resizeandrawpool nowcomponentpassesbutnot
joinedwithSPYR. UpstreamGPUproductionofP/filterstages stillneededforfull
metric; generalcomparison-maskdouble resize andfinalQ/Q_JODnotyetvalidated.

### Newest: full SPYR +residual chain on GPU (2026-10-24)

- User explicitly requests continuing without waiting for another prompt.
  No new publication authorization; all local. Preserve staged corrdn,
  dirtyVkFFT and all prior changes. Public shapes/production binaries unchanged.
- First joined lowpass root pathway_spyr_chain_639284283321272327 passes
  42486 comparisons and oldlowpass40800 regression. New shader supports
  paired input via finalpushword (zero still1batch); no host baseband upload.
- Native/allband shader root:
  `validation/pathway_spyr_bands_639284284245362379`. Extended nativegenerator
  adds hi0.txt/orient.txt filters and high.txt/orient0/1/2 outputs perimage,
  each asserted against hdrvdp_spyr.get_band. Native raw/centeredbase assertions
  retained. sp0Filters has ONE orientation perscale; no otherfilterclaim.
  New normalize selectors2/3/4 -> /2,/4,/8;selector1 remains /16.
  gpu_bands/PASS111840 numerical comparisons, observed bitdiff0.
- FINAL root `validation/pathway_spyr_all_chain_639284285430220524`, latest
  strictMSVCRelease spyr-lowpass-test. New --all-chain profile, eightSPYR
  operations(lo0,3down,hi0,3orientation),mean/DC,differencepack,residualFFT.
  Paired reference/test,19dispatches inONEcommandbuffer perimagepair.
  3bindingSPYR and2bindingresidual layouts/barriers keepallintermediatesonGPU.
  Host uploads ONLY originalPmaps andcoefficients; reads afterfinalfence.
- gpu_all_chain/PASS/results.tsv IntelArcPro:111840SPYR/lowpass numericexact,
  484mean/DC+480packed-difference+242mag/index BITWISE,480FFTcomponents numeric
  exact =113526 comparisons. Observed allbitdiff0, butFFT/convolutionzero-sign
  bits areNOTacceptance. Same twoactualimagepairs, notnewuniquecoverage.
  No numerical failures. Starts atnativeP, ends atresidualfrequencyselection.
- ActualP source general_pathway_639284244350897941, Float32,30ppd,age24;
  once-roundedFloat32coefficients,reflect1/start0/step1then2,FMA32elementprefix
  alonglongerinputaxis andprecisenonFMA tail in separatevariants. /16rawbase,
  separateoriginalmeans, no recenterdifference,rawFFTDCretained/magDCzero,
  nativecolumnmajorfirstindex. NoFFTpadding/filter withinSPYRcomponent.
- Finalchecker defaultlowpass(40800),--bands(111840),--chain(42486) regressions
  pass in finalroot/gpu_{lowpass,bands,chain}_regression. Source
  verify_spyr_lowpass.cpp now supports4profiles; shaders/native frombandsroot,
  mean120/differencepack/residualextended unchanged. Combinedgoldensfrom
  pathway_difference_gpu_639284272183191896/native. inputs.json and
  checkpoint_hashes.json recordexactdependencies/finalsource/finalbinary.
  Historical earlierbinaries retained; finalrootbindslatestsource/binary.
- Docs SPYR_LOWPASS_VALIDATION.md,MASKING_QUALITY_VALIDATION.md updated.

NEW exact next step (overrides historical paragraphs): general-shape GPU
adaptation/log-map resize andCSF against saved trace.log_resized/trace.csf for
all5bandshapes. Inspect MapGPUguards/nativeimresize antialiasing/coordinates;
separateexperimentalprofile, doNOTenablepublicquality. Preserveactualbase120
poweracceptance. UpstreamGPUproductionofP also requiresjoining mixedFFT with
actualMTF/LAfilters andphotoreceptorstages. Then masking/pooling andindependent
fullstage/Q/Q_JOD comparisons. DoNOTclaimfullmetriccomplete fromnativePtests.

### Newest checkpoint: GPU SPYR lowpass from real P maps (2026-10-24)

- `validation/pathway_spyr_lowpass_639284274200978716`, active/completed
  pointers pathway_spyr_lowpass_{active,completed}_path.txt. New
  generate_pathway_spyr_lowpass_reference.m exports native4stage lowpass
  for4actualP maps from general_pathway_639284244350897941. Native raw and
  DC-centered base bands reproduce prior trace BITWISE. Same source images,
  not new coverage;30ppd/age24. Source hashes recorded for buildSpyr,
  buildSpyrLevs,corrDn,rconv2,sp0Filters; official install untouched.
- New tests/spyr_lowpass.comp, SSBO input/output/filter.7x7lo0step1 then
  three13x13lostep2,reflect1 every stage,start0 GPU /start[1,1]native. Final
  output /16 exactly matches normalized rawget_band. No DC removal here.
  Float32image/once-roundedFloat32coefficients, descending taps along longer
  input axis. Native32elementvectorprefix explicitFMA,tail precise separate
  multiply/add in TWO variants, disjoint output writes.20x24/24x20 entire
  axis is separate arithmetic; no incorrect blanket FMA extension.
- compile_spyr_lowpass.ps1 fresh directory, optional-NonFma; only newshader
  compiled. ExplicitDenormPreserve32/RTE32, Vulkan1.2spirv-val passes, no
  Float64/Int64. Existing/staged corrdn shaders/binaries not touched.
- New verify_spyr_lowpass.cpp and isolated CMake targetspyr-lowpass-test.
  StrictMSVCRelease freshbuild. One command buffer4stages/8dispatches with
  write/readbarriers. Host uploads only originalP andfilters, no intermediate
  CPU calculation/upload; reads all4outputs after finalfence forcomparison.
- gpu/PASS.txt/results.tsv onIntelArcPro:16stagecomparisons40800Float32values
  numerical EXACT,observed bitdiff0. Perinput7680+1920+480+120=10200,4inputs.
  Signedzero bits recorded butnot required forconvolution. No numerical
  failures. NoFFTpadding/filter inunit,onlyspecifiedreflect1/downsample,
  row-majorlayout; output10x12/12x10. No general publicshape/ABI changes.
- Docs SPYR_LOWPASS_VALIDATION.md and MASKING_QUALITY_VALIDATION.md updated.
  checkpoint_hashes.json freezes source/native/shader/executable; completed
  pointer set afterPASS. No commit/push; preserve dirtyVkFFT/alloldchanges.

NEW exact next step (overrides historical paragraphs): connect the new GPU
lowpass output buffers to accepted mean120 ->differencepack ->residualFFT
without host readback/reupload, paired reference/test. Need one combined
checker with lowpass descriptor layouts3bindings vs residual2bindings and
the /16 normalization retained. Golden raw bases can come from this new
checkpoint and combined expectations from pathway_difference_gpu checkpoint.
Then extend actual-P-map decomposition to highpass/orientation outputs with
native band normalization; finally generalfilter/visualpathway/resize/CSF
and fullstage/Q/Q_JOD comparisons before publicshape guards change.

### Newest checkpoint: combined GPU difference chain (2026-10-24)

- `validation/pathway_difference_gpu_639284272183191896`, active/completed
  pointers pathway_difference_gpu_{active,completed}_path.txt. Local only;
  production guards, existing shaders, staged corrdn and dirty VkFFT preserved.
- New tests/mean120_difference_pack.comp: reference/test mean+centered pairs,
  skip means, precise test-reference subtraction, column-major complex packing
  with positive-zero imaginary words. No second DC subtraction. New targeted
  compile_mean120_difference_pack.ps1 uses fresh directory, explicit
  DenormPreserve32/RTE32, NoContraction, Vulkan1.2 spirv-val. NoFloat64/Int64.
- verify_residual120_pipeline.cpp adds --pathway-difference: two mean groups
  ->one pair/difference->one residual FFT group per orientation. Separate input
  and output counts/buffer sizes/domain checks; original --pathway and12-case
  modes retained. Generator now writes combined_manifest.txt with uint32
  difference/expected files while retaining separate float-parser manifest.
- Native source general_pathway_639284244350897941, same two real image pairs,
  no new unique source data. Single/30ppd/age24; raw exponents9..9. Mean/120,
  one Float32 DC subtraction per original band, then test-reference, forward
  FFT unnormalized. No padding/filter in residual unit; native upstream uses
  official2H x2W. Input row-major raw real; packed complex column-major;
  final FFT/magnitude row-major, first-max native column-major. DC magnitude
  zero only, raw FFT DC retained. Native assertions connect to trace selection.
- gpu_combined/PASS and results.tsv, IntelArcProGraphics:484 mean/DC words,
  480 packed-difference words and242 mag/index words BITWISE exact;480 FFT
  components numerical exact (observed bit diffs0, not asserted). Single
  command buffer/3dispatches/write-read barriers; host only uploads RAW bands,
  waits then reads intermediates for comparison. NO native difference upload
  and NO host intermediate computation in combined path. No numeric failures.
- Fresh MSVC strict Release checker in build/Release; unchanged accepted
  mean120.spv and residual120_extended.spv, only difference shader new.
  gpu_pathway_regression/PASS (4bands) and gpu_original_regression/PASS
  (12bands) both pass with latest checker. Source/reference/executable/SPIR-V
  hashes in checkpoint_hashes.json. Docs updated, git diff --check passes.
- Reproduce: generate_pathway_residual120_reference.m <freshnative> <trace>;
  compile_mean120_difference_pack.ps1 -OutputDirectory <freshshader>;
  residual120-pipeline-test <mean.spv> <differencepack.spv> <extendedfft.spv>
  <native>/combined_manifest.txt <freshgpu> --pathway-difference.

NEW exact next step (supersedes earlier next-step paragraphs): trace and port
the upstream general-shape SPYR corrDn/decomposition on actual saved P maps,
first native inputs -> GPU raw base bands, before joining the now-accepted
mean/DC/difference/FFT chain. Inspect existing corrdn runtime guards/shader
profiles and native buildSpyr edge/step/start semantics; do not overwrite the
staged+unstaged corrdn shader. Use a separate experimental shader/ABI and fresh
exports. Then filter/visual-pathway/resize/CSF integration and independent full
stage/Q/Q_JOD comparisons. General public80x96/96x80 guards remain disabled;
no commit/push authorization added.

### Newest checkpoint: actual pathway residuals (2026-10-24)

- `validation/pathway_residual120_639284268283635835`; source actual P maps
  from general_pathway_639284244350897941. New generator
  tests/generate_pathway_residual120_reference.m reconstructs official SPYR
  sp0Filters/30ppd, four real reference/test raw bands. Native mean/DC output
  equals traced centered bands BITWISE; raw exponents9..9. Age24, original
  asymmetric/local-distortion fixtures. No new full quality GPU claim.
- New explicit --pathway profiles: pipeline checker requires4matrices (2per
  orientation), residual checker requires2extended matrices. Original12/76
  guards retained. Checker records dump/results before magnitude failure.
- Accepted native_float_contract/, gpu_pipeline_float_contract/PASS and
  gpu_difference_float_contract/PASS. IntelArcPro: four raw bands mean/DC484,
  pack960,mag/index484 words bitwise,FFT960 numerical exact. Two actual
  centered test-reference differences:480FFT components numerical exact,
  240magnitudes+2indices bitwise. Observed allFFTbitsmatch but not asserted.
  Native assertions compare difference to trace and spectrum after DCzero,
  native selectedindexexact. No recentering of difference; separate native
  means removed per reference/test before subtraction. Forward unnormalized,
  no padding/filter in residual component; layout as RESIDUAL120_VALIDATION.
- Initial standalone difference run failed356/361 onfirstmatrix because new
  fixture wrote uint32 text but old residual parser expects decimal floats.
  Diagnostic dumps showed bogus large inputs. Corrected generator uses float
  text ONLY difference manifest; pipeline stilluint32. Failed artifacts
  gpu_difference/,gpu_difference_diagnostic/,native/ preserved. Fresh corrected
  directories above passed; no shader arithmetic changes or tolerance use.
- Fresh MSVC Release targets residual120-test,residual120-pipeline-test in
  checkpoint/build (CMake VisualStudio18 2026,x64,/fp:strict). Reuses accepted
  mean120/shader/mean120.spv, residual120_pipeline/mean120_pack.spv and
  residual120/shader_extended_verified/residual120_extended.spv. No shader
  recompilation, production files/guards/ABI or stagedshader/VkFFT changes.
- Regressions: gpu_pipeline_regression/PASS old12cases passes;
  gpu_extended_final_regression/PASS latest checker old76matrices passes,
  27436 numeric values,490rawFFTzero-signbits only. Source/binary/reference
  hashes frozen in checkpoint_hashes.json; completed pointer set after PASS.

NEW exact next step (supersedes older next-step paragraphs): extend the real
pathway pipeline to subtract GPU-centered test/reference bands on GPU, pack
that difference and dispatch residual FFT/magnitude/index without uploading
native intermediate differences. Keep native trace selection as independent
golden and no second mean subtraction. Current difference test uploads native
difference separately, so do not claim combined end-to-end dispatch yet.
Then progress experimental filtering/SPYR/resize/CSF integration and full
stage/Q/Q_JOD comparisons before enabling general public shapes. No commit/push.

- Local work only; no commit/push. Preserve all prior changes, staged/unstaged
  corrdn shader, old artifacts and dirty VkFFT. Production guards unchanged.
- Residual checkpoint `validation/residual120_639283813321637573`: raw GPU
  76 matrices/18240 FFT Float32 values numerically exact;490 differences are
  zero-sign bits. Extended test adds9120 bitwise magnitudes and76 bitwise
  first-max indices. Native FFT plans match UNALIGNED scalar10/12 codelets;
  aligned candidates do not match. See RESIDUAL120_VALIDATION.md.
- Latest extended checker raw-mode regression on2026-10-24 passes again in
  fresh gpu_raw_regression_* directory, same18240/490/0 result onIntelArcPro.
- Actual three-dispatch GPU pipeline checkpoint
  `validation/residual120_pipeline_639283821352963342`: mean/DC -> uint packing
  -> FFT/magnitude/index, shader barriers in one command buffer, no host
  intermediate computation/upload. Twelve bounded standalone official SPYR
  fixtures:1452 mean/DC +2880 pack +1452 mag/index words bitwise;2880 FFT words
  numerically exact (observed zero bit differences, not FFT-bit acceptance).
  These12 are a subset of the76 above, not new unique fixtures. No padding or
  normalization except mean/120; DC subtraction precedes forward FFT and only
  magnitude DC is zeroed afterwards. Host checks bounded mean/norm domains.
- Base checkpoint `validation/base120_639283822820535556`:8192 synthetic plus
  12 native residual-derived logmaps,16408 mean/pow10 output words BITWISE.
  Input120 logs[-5,5], normal/zero, min exponent-90, exponent spread<=16;
  integer bounded mean +existing native-style power helpers, explicitFloat32
  controls, noFloat64/Int64. Native logs originally come from positive
  standalone SPYR residuals, NOT complete visual-pathway adaptation.
  Separate input/output buffer sizes; no numerical GPU failures.
- residual120, residual120_pipeline, base120 completed pointers now set after
  checking PASS files. checkpoint_hashes_20261024.json freezes source, native
  references, shader, artifacts and executable hashes in each root. Original
  base executable hash refers to pre-pathway checker source; the newer source
  and executable are frozen separately in the new pathway checkpoint.
- New `tests/generate_general_pathway_reference.m` creates a licensed traced
  COPY of installed hdrvdp3.m in a fresh validation directory, never edits
  C:/HDR-VDP-3_Ref. Function rename/base_dir adjustment/trace assignments only;
  source hashes recorded. Checkpoint
  `validation/general_pathway_639284244350897941`, original and traced native
  calls both fresh wisdom/3threads/ESTIMATE. For80x96 and96x80 asymmetric
  images with localized distortion,30ppd/age24, Q and Q_JOD exactly equal.
  Scores respectively4.030489921569824/4.869779586791992 and
  3.958810567855835/4.790723323822022. NOT GPU quality scores.
- Native trace exports adaptation/log/P maps, all normalized SPYR bands,
  resized logs/CSF, base difference/spectrum/selection; trace.mat preserves
  classes/complex values, decimal row-major text17digits. Base spectrum is
  captured AFTER official DC zero. Base difference exactly as used by metric;
  do not assume another mean subtraction is equivalent. MTF/LA padding remains
  official2H x2W; no power-of-two rounding. Source classes recorded.
- New generate_pathway_base120_reference.m extracts actual resized logmaps;
  asserts double-sum mean and power==tracedL_mean. verify_base120.cpp adds
  explicit --pathway mode requiring2batches/4words; old mode still8192+12.
  Fresh MSVC Release base120-test in pathway/build, unchanged accepted base
  SPIR-V: base_gpu/PASS bitwise4/4 onIntelArcPro. Same new checker passes old
  16408words in base_original_regression/PASS. No production ABI extension.
- Commands: octave-cli --quiet --no-gui tests/generate_general_pathway_reference.m
  <fresh>; generate_pathway_base120_reference.m <freshbase> <trace>;
  cmake -S tests/hdrvdp3_dimensions -B <trace>/build -G "Visual Studio 18 2026"
  -A x64; cmake --build <build> --config Release --target base120-test -j4;
  base120-test <acceptedbase.spv> <manifest> <freshgpu> --pathway.
  Octave cleanup warning returns0; PASS/results asserted separately.
- Docs BASE120_VALIDATION.md, RESIDUAL120_VALIDATION.md and
  MASKING_QUALITY_VALIDATION.md reflect current versus historical scope.

Exact next step: use the new trace.mat P_reference/P_test to reconstruct raw
official SPYR base bands before visual-pathway DC subtraction; verify these
against exported centered bands. Generate mean120/DC fixtures from those REAL
pathway bands, and an explicit-count pipeline checker profile (do not duplicate
fixtures to satisfy the old12-case guard). Check actual base difference FFT /
magnitude/index against traced native selection, keeping official DC semantics.
Then integrate filtering/SPYR/resize/CSF in experimental general-shape paths,
and compare every stage plus final Q/Q_JOD before enabling public80x96/96x80.
Both big FFT orientations, bounded residual composition and actual base power
are component passes only. Old next-step entries below are historical.

## Latest continuation: transposed FFTs and exact bounded mean120/DC

- User explicitly requests continuous work without asking whether to continue.
  No publication authorization added; do not auto-commit/push. All work local,
  production guards/shaders unchanged, old artifacts/stagedshader/dirtyVkFFT
  preserved. Completed pointers below are component acceptance, not fullquality.
- real192 first-axis checkpoint validation/real192_axis_639283673902853437,
  completed pointer real192_axis_completed_path.txt. verified_complete/PASS:
  12streams372480Float32values exact onIntelArcProGraphics. SelectedUNALIGNED
  native rank2 firstsubtree match forall12(not arbitraryalignedplan). Outer
  hc2cf_8/r2cf_8/r2cfII_8 over real24; inner24 twelve r2cf_2 sum/difference
  leaves then r2cf_12/r2cfII_12. Pinned hc2hc-direct.c proves inner m2 ->mb=me1,
  so hf_12body loop empty,onlyendpoints run. Sources GPL+hashes inreal192/.
  Input192x160colmajorinterleavedrealFloat32(header192320),compact160x97
  complex(header160194),unnormalized. Initialverified/ GPU passed butharness
  searchedinverse_axis tag;fixedfreshverified_complete isacceptance.
- inverse192firstcheckpoint validation/inverse192_axis_639283803326908322,
  verified/PASS:12streams737280Float32valuesexact,allselectedrank2subtrees.
  t2_16/n1_12 withswappedbackwardscalarpointerroles,transposedrowmajoroutput,
  no normalization. Sourcepowerassertcaughtassumed7 beforeGLSLcompile;actual
  powers1,3,9,15;shader_correct_descriptor accepted. Completedpointerset.
- Fullforward192x160checkpoint validation/forward192x160_639283801741962283,
  verified/PASS:12streams737280Float32valuesexactagainstofficialfft2,plus
  372480first-axisregressionvaluesexact. completedpointerset. Complex160
  n2fv_8_avx2_128 FMAgraph0 +t3fv_20_avx2_128 ordinarygraph1 withNATIVE
  COMPILERCONTRACTIONofvectorhelpers. Keepfloatscalarhelperspreciseforreal192;
  allowingvectorhelpercontractionisnecessaryoncurrentArc/compilercontract.
  NativeDLLread-onlyPEscan+objdump foundkernelVA0x3026f1b10,registration
  0x3026f24a0,extraFMAinstructions;disassembly/scan/DLLhashsaved. Addressonly
  appliespinnedDLL,notproduction. Notportabilityclaimoruniversalscheduleproof;
  pin/revalidateshader/device/driverbeforeintegration. No tolerancepasses.
  Graph/helperstrictprobesfailed31598/23205/23996/10322values;nestedproduct
  fusionswap18421;preserved. CorrectdefaultSPIRVbytes=successfulprobe.
  Nativeleafdiagnostic --complex160-leaves:initialinplaceUNALIGNEDplanselected
  n1_8,mismatches7565,excluded. Alignedoutplaceinputstride20/distance1,
  outputstride1/distance8 selectsactualn2fv_8_avx2_128;31040GPUleafvaluesexact.
  RawGPUleafprobe fullforwardrejectionintentional;separateleafcheckaccepted.
- Fullinverse192x160checkpoint validation/inverse192x160_639283807856355507,
  verified/PASS:12streams737280rawvaluesexactvspriornativeFFTWunscaledexports
  and737280normalizedvaluesexactvsofficialifft2,first-axisregressionsbothmodes
  exact. t2_16/n1_12firstaxis;scalarbackwardn1_8+t3bv_20ordinarywithnative
  vectorcontractionsecondaxis;unchangedinteger30720divisionembedded. NoCPU
  fallback,cachedoriginalnativeplansNOTrecovered. completedpointerset.
- FFTfixtures:priorh96_w80family asymmetric/impulse/HDR-ramp,ref/test1.05,
  MTF/LA30ppd;NOTfullqualitypairs. Forwardstage0alreadyMTFsymmetricPOSTor
  LAreplicatePOSTpadded;inverseinputstage2alreadyfilterednativecomplexFloat32.
  Inputcolmajor192x160;finalGPUcolmajor,checkercomparesrowmajorinterleaved.
  FFTunnormalizedexceptexplicitfinalinverse/30720,nopadding/filter/DCinunit.
  SignedzerobitsNOTassertedforFFTs. Allnewcompile_* scriptsuseexplicit
  DenormPreserve32/RTE32,Vulkan1.2spirv-val,onlytargetednewshaders. FreshMSVC
  Releasebuildsescalated;oldbinaries retained. Docs TRANSPOSED_MIXED_FFT_VALIDATION.
- New bounded120samplemean/DC checkpoint validation/mean120_639283811026801827,
  completedpointerset;tests/mean120.comp,verify_mean120.cpp,mean120-testCMake
  target,compile_mean120.ps1 andgenerate_mean120_reference.m. Exactsumstored
  intwo uint32 words (<=47magnitudebits),integerdivisionby120withRTEthen
  singleprecisesubtraction. NOFloat64/Int64capability,NOroundedreciprocal.
  Hostdomain:finiteFloat32normalorzero,nonzeroexponents[-90,100],spread<=16.
  Doublepartialsumsexact;47bitbound+odd15factorpreventsdouble-roundingoff
  floatmidpoint(doubledivisionthenfloatmatchdirectrationalround). Noextension
  outsideboundedproof/domain. Allzeromean+0,negativezeroDCwordspreserved.
- ActualGPUIntelArcProGraphics:8192syntheticcases991232outputwordsBITWISE
  exact +12officialSPYRresidualcases1452wordsBITWISEexact. gpu/PASS.txt,
  results.tsv,input/expected/outputu32,GPUlog. Syntheticall/negativezero,
  constants,cancellation,randommantissas/signs/exponentscoverdomain;NOTall
  possibletuples. HoststrictdoublemeanandfloatDCcross-checknativebeforeGPU.
  NativeSPYR12fixtures:80x96/96x80,asymmetric,constant10+impulse7,bounded
  HDRramp1e-2..1e3,ref/test1.05,30ppd. Residual10x12/12x10,observedexponent
  span0or15,mean2..11;inputrowmajor120words,outputmean+120centeredwords.
  Thesearedifferentboundedcomponentfixturesfromprior9decadeHDRFFTfixtures;
  NOTwholevisualpathway/fullquality. NoFFT/padding/filter/pow10incurrentunit.
  ExplicitFloat32controls;nativeOctavewarnings/exitcleanupmessagewithcode0,
  all12completed,nofailednumericalmeanruns. Docs MEAN120_VALIDATION.md.

Exact next step: generateofficial10x12/12x10 residualFFT referencesfromthe
newmean120nativecenteredresidualspluscontrolledsynthetics;inspectfreshFFTW
candidateplansandpinactualr2cf_10/r2cf_12/complex10/12codeletsbeforeGPUport.
ThenGPUresidualFFT,magnitudes/DCzerofirstmax;compose120meanandbasepow10via
separateexperimentalABI. BothmixedbigFFTorientationsnowcomponentpasses,
notpublicquality. Filter/SPYR/CSF/mapgeneralshapeintegrationandfullofficial
stages/scoresremain. Keepguardsandstagedshader/dirtyVkFFT;noautopublication.

## Latest continuation: complete160x192 inverse and normalization on GPU

- Freshcheckpoint `validation/mixed_inverse_full_639283669024826224`,active and
  completed pointers `validation/mixed_inverse_full_{active,completed}_path.txt`.
  Localonly,no commit/push. ProductionFFT/mapcodeandpublicguardsunchanged.
- Added pinnedFFTW3.3.10 backward t1bv_6/t3bv_8 andISAwrappers to
  tests/fftw_reference/mixed_inverse/SOURCES.json,originalGPLnotices/member
  LFhashesretained. Reusespinnedt1b.h/t3b.h,simd-avx2.h,n1_4 andprevious
  inverse160generator. No newCPUFFT implementation orproductionCPUfallback.
- generate_fft160x192_inverse_shader.ps1 combines acceptedinverse160firstaxis
  withinverse192secondaxis:scalar n1_4 withbackwardreal/imagpointerroles,
  t1bv_6_avx2 inner24,then t3bv_8_avx2_128 outer8. Precisescalar/vectorhelpers,
  explicitSIMDFMA,t1ordinarygraph1,t3FMAgraph0;arithmeticmatchnotnativebuild
  branchrecovery. ChecksheaderhashesBYTW1/TWVL1,t3VTW3/TWVL3,descriptorpowers
  1/2/3/4/5 and1/3/7. Hostdoubletrigonce-roundedFloat32,exactquadrants.
- EmbedsunchangedfiniteintegernormalizeWord bodyfrommixed_normalization.comp.
  Mode --inverse160x192-raw returnsunnormalizedinverse; --inverse160x192 applies
  correctlyroundedFloat32division30720onceaftersecondaxis,p.fused=1(flagonly;
  firstaxishelpersremainnonfused). No roundedreciprocalmultiply.
- New compile_inverse160x192.ps1 (tests/hdrvdp3_fft256) compilesonlynewshader,
  explicitlyaddsDenormPreserve32/RoundingModeRTE32SPIRVcapabilities+modes,
  assemblesandvalidatesVulkan1.2. OnlyShader/DenormPreserve/RoundingModeRTE
  capabilities,noFloat64/Int64. shader_reproducedbytesidenticaltotestedshader
  afterfinalheader/powerassertions(reproduction_check.txt).
- ExtendedGPUcheckernewraw/normalizedmodes,freshstrictReleaseMSVCbuild/build
  succeeded(escalated). Reusablehdrvdp3_dimensions/validate_mixed_inverse.ps1
  copiespriornativeinputs/referencesintofreshfixtures,recordsoriginalpaths and
  hashes,checksrawandnormalizedoutputsplusfirstaxisagainstpriornativeoracle.
  ActualGPU IntelArcProGraphics. All12streams(asymmetric/impulse/HDR-ramp,
  ref/test,MTF/LA)737280rawFloat32valuesmatchpriornativeFFTWrank2unscaled
  exports;737280normalizedvaluesmatchindependentofficialOctaveifft2. Allzero
  mismatches.737280uniquefirst-axisvaluesexactinbothmodes(repeatedchecks).
  verified/PASS/results.tsv/24per-modelogs/input_binary_hashes.txt areacceptance.
  SignedzerobitsNOTassertedforwholeFFT;priornormalizerbitwisecoverageisseparate.
- Nativeunscaledreferencesreusepriorfresh3threadESTIMATE|UNALIGNED/outofplace
  candidateexports,NOTrecoveredoriginalOctavecachedobjects. Their192second
  axisplanhas54/54/52groups;GPUperlanearithmeticmatchedindependently.
  No newnativeCPUoraclesgeneratedthischeckpoint;oldreferencefilespreserved.
- Contracts:h80_w96componentfixturefamily,30ppd,ref/testfactor1.05,stage2already
  filterednativecomplexFloat32,column-major160x192(header160384),precededby
  symmetricPOST(MTF)/replicatePOST(LA)preparation. Firstinverseoutputtransposed
  forcontiguous192secondaxisreads;finalGPUcolumn-major,checkerexportsrow-major
  160x192interleaved. axis0_storageheader192320describesstorage;wordsequence
  matchespreviousrow-majorfirstaxisoracle(header160384). Nopadding/filter/
  mean/DC/crop/exp/mapworkperformedhere. NOTfullqualityimagepairacceptance.
- InitialasymmetricrefMTFraw+normalizedprobespass;newcheckerforwardregression
  passes61440values(forward_regression.log). Repeatedfixturesarenotnewcoverage.
  No failednumericalrunsinthischeckpoint. Oldartifacts/stagedshader/dirtyVkFFT
  preserved. Docs tests/MIXED_INVERSE_VALIDATION.md,MASKING_QUALITY_VALIDATION.md.
- Inspectedselectedtransposednativeplan,NOTfirstalignedplan: extracted
  ref_mtf_t3_unaligned_outplace forward+inverse totransposed_next_plan.txt.
  Graphisdifferent:real192first hc2cf_8/r2cf_8/r2cfII_8 withnested
  hf_12/r2cf_12/r2cfII_12/r2cf_2;inverse192first t2_16/n1_12.
  Complex160secondforwardt3fv_20_avx2_128/n2fv_8_avx2_128,
  backwardt3bv_20_avx2_128/n1_8. Do notsimplyswapdimensionsoruse192secondgraph.

Exact next step: extractnativefirst-axiscompact97frequencyfixturesfor
192x160(h96_w80family),pinnewrealcodeletsandportactual8x24nestedrealgraph
withhf_12. CompareisolatedGPUfirstaxisbeforenewcomplex160secondaxis. For
inverseuseactualt2_16/n1_12firstaxisand t3bv_20/n1_8 second. Existing160x192
forward+rawinverse+normalizedinversearecomponentpasses,notpublicquality.
120samplemean/DC,10x12residualFFTandfilter/mapintegrationremainbefore
full80x96/96x80officialstages/scores. Retainpublicguards,noautocommit/push.

## Latest continuation: unnormalized inverse160 first GPU axis

- Freshcheckpoint `validation/mixed_inverse160_639283665236700141`, active and
  completed pointers `validation/mixed_inverse160_{active,completed}_path.txt`.
  Localonly,no commit/push. ProductionFFT/maps/publicguards unchanged.
- Added pinnedFFTW3.3.10 t2_20.c,t1b.h,t3b.h under
  tests/fftw_reference/mixed_inverse,originalGPLnotices+SOURCES.json LFhashes,
  archivehash/memberprovenance. t1b.h explicitlyselects BYTWJ1/TWVL1,justlike
  forwardt1f.h;do notinfer backwardtwiddlecontract fromname.
- generate_fft160_inverse_axis_shader.ps1 ports ordinaryscalar t2_20 anduses
  existingordinaryn1_8 leaves.20decimated8pointFFTs then20pointtwiddlestage,
  nativebackward real/imagpointerroles swappedbefore/after. Precisescalar
  helpers/results,nonfusedFloat32operations. Descriptorpowers1,3,9,19,
  hostdoubletrigonce-roundedFloat32,exactquadrants. NoCPUfallback.
- Nativeinspectoradds --inverse160-axis <stage2.txt> <freshdir>,freshwisdom,
  threads3,ESTIMATE|UNALIGNED,outofplace. Exportsactualplan andunnormalized
  rowmajoraxis0_expected.txt;copiesinputref_mtf_stage2. All12nativeplans
  identical,eachcomplete whitespace-normalizedplan isexactsubtreeofprior
  matchedrank2inversegraph(three64columngroups,t2_20/n1_8). Stillcandidate
  arithmeticgraph,NOT recoveredoriginalcachedOctaveobjects.
- GPUcheckeradds --inverse160-axis,onesinglepass,noinverse192/normalization.
  tests/hdrvdp3_dimensions/validate_inverse160_axis.ps1 orchestratesnative/GPU
  foreachasymmetric/impulse/HDR-ramp xref/test xMTF/LA,checksplans,records
  sourcepaths/hashesandper-streamlogs. FreshstrictMSVCReleasebuild/build_native
  succeeded(escalated). All12streams737280Float32valuesnumericallyexact on
  IntelArcProGraphics,zero maxerror. Finalacceptance verified_preserve/PASS,
  results.tsv,input_binary_hashes.txt. SignedzerobitsNOTasserted,no tolerance.
- Inputcontract: priornativeh80_w96 fixturefamily,factor1.05ref/test,30ppd,
  MTFsymmetricPOST/LAreplicatePOST160x192preparationwithdoublefilters;
  stage2alreadyfiltered,columnmajorinterleavedcomplexFloat32,header160384.
  Each192columns transforms160complexsamples;GPUwritestransposedstorage
  equivalentrowmajor160x192interleavedoutput,header160384. Unnormalized,
  noextraFFTaxis/filter/padding/mean/DC. NOTfullqualitypairacceptance.
- IMPORTANT: initialrawglslangshader lackedexplicitfloat-controlmodes.
  verified/ passedasymmetricrefMTF butfailedrefLA9280values,maxerror
  6.018531076210112e-36. DevicecapabilitycheckaloneisNOTmodeactivation.
  Preservefailedrun;excludedfromacceptance. New
  tests/hdrvdp3_fft256/compile_inverse160_axis.ps1 followsexistinginverse
  protocol:glslc,SPIRVdisassembly,explicitDenormPreserve32/RoundingModeRTE32
  capabilities+executionmodes,assembly,spirv-valVulkan1.2. shader_preserve is
  final;capabilitiesShader/DenormPreserve/RoundingModeRTEonly,noFloat64/Int64.
  Thisfixrestoresall12exactresultswithoutalgorithm/tolerancechanges.
- NewlybuiltGPUcheckerforward160x192regressionasymmetricrefMTF61440values
  exact(forward_regression.log);repeatedfixture,notnewuniqueinputs. Allprior
  references/shaderbinaries,stagedshaderanddirtyVkFFTpreserved. Docs
  tests/INVERSE160_AXIS_VALIDATION.md, MASKING_QUALITY_VALIDATION.md.

Exact next step: port192complexinverseSECONDaxisfromactual
t3bv_8_avx2_128/t1bv_6_avx2/n1_4(backwardscalarpointerroles)with54/54/52
nativebatchgroups. Exportunnormalizednativeintermediates,compareGPUbefore
composingalreadyverifiedfixed30720divisionagainstofficialifft2for12streams.
UseexplicitDenormPreserve32/RTE32inthecompiledSPIRV,notjustdevicechecks.
Inverse160firstaxisalreadyexact;don'tredoitsalgorithm. Transposed192-FIRST
orientationstillunported.120sampleDC/mean,10x12residualFFT,mapintegration
stillpendingbeforepublic80x96/96x80quality. Noautocommit/push.

## Latest continuation: complete160x192 forward GPU FFT

- Fresh checkpoint `validation/mixed192_forward_639283658517194869`; active and
  completed pointers `validation/mixed192_forward_{active,completed}_path.txt`.
  Local only,no commit/push. No production shader/map/API guard changes.
- Added pinned FFTW3.3.10 sources under tests/fftw_reference/mixed192:
  t3fv_8,t1fv_6,n1_4,simd-avx2.h,t1f.h,t3f.h,ISA wrapper files. OriginalGPL
  notices retained,SOURCES.json archive/member/LF SHA256 provenance.
- generate_fft160x192_forward_shader.ps1 combines prior real160 scalar20x8
  graph with complex192: n1_4 leaves -> t1fv_6_avx2 inner24 ->
  t3fv_8_avx2_128 outer8. Precise scalar and vector helper results are required
  across function boundaries. t3 FMA graph0 selected; t1 ordinary graph1 with
  explicit SIMD FMA matches. Both t1 branches were arithmetic-equivalent on
  probe inputs; do not claim native build-time branch recovered.
- Critical correction: t1f.h explicitly selects BYTWJ1/TWVL1, NOT BYTWJ2.
  BYTWJ1 rounds sine products separately and fuses cosine products, as does
  native VZMULJ. Header is now hash-checked and selection asserted. Hostdouble
  trig tables once-rounded Float32 with exact quadrants,24/192 denominators.
- verify_fft256_inverse.cpp adds --forward160x192 (h160,w192). Old modes
  retained. Strict fresh Release MSVC build tests/hdrvdp3_fft256; binaries
  inside checkpoint/build. New shader only, glslangVulkan1.2+spirv-val pass.
  shader_reproduced SPIRV bytes equal shader_final after final source assertions.
- Added reusable tests/hdrvdp3_dimensions/validate_mixed_forward.ps1. Reads
  prior6fixture native root and prior first-axis root, copies selected h80_w96
  MTF/LA streams into fresh renamed ref fixtures with original source JSON and
  hashes. All12 streams x61440=737280 full-forward Float32 values match
  independently exported official fft2 numerically on Intel Arc Pro Graphics.
  All12 first-axis checks x31104=373248 remain numerically exact. Signed-zero
  bits NOT asserted; no tolerance pass. verified/PASS/results.tsv/per-stream
  logs/input_binary_hashes.txt record accepted result. Nativeparams:30ppd,
  ref/testfactor1.05,asymmetric/impulse/HDR-ramp;NOTfullqualitypair acceptance.
- Contracts: prior stage0 already MTF symmetric POST / LA replicate POST padded
  to160x192;Float32,column-major interleaved real input,imag0. First compact
  192x81complex;finalGPUcolumn-major expanded Hermitian output compared/exported
  row-major interleaved against officialfft2. Unnormalizedforward,no extra
  padding/filter/inverse/normalization/DC. No productionCPU fallback.
- Native inspector adds --complex192-axis <compact input> <fresh dir>:
  fresh3threadESTIMATE|UNALIGNED,full192 and8strided24 probes,actualplans
  matchingexpected3groups27columns. AsymmetricrefMTF full192 vs official half
  31104 values exact(native_second/reference_check.txt). CorrectedGPUinner24
  also31104 exact across8offsets,inner24_gpu_check.txt/INNER24_PASS.
  Export-only inner24 shader intentionally fails generic fullforward comparison
  in itsrawlog;onlyseparateintermediatecheck isaccepted. No recovered original
  cachedOctaveobjects;matchedfreshcandidategraphs only.
- Failedprobespreserved: initialassembly firstaxis262differences/full32935;
  restoreoriginalscalarhelpers =>firstaxis0/full32293. Precisevectorhelpers
  andt3graph0 =>full26354 withwrongBYTWJ2. All4graphvariantprobes,wronginner24,
  nonfusedcomplexmultiply(full22586) retained;NOTtolerancepasses. Correctheader
  BYTWJ1 =>full0. Docs tests/MIXED_FORWARD_VALIDATION.md.
  Pre-existing stagedshader,dirtyVkFFT,oldreferences andotherwork preserved.

Exact next step: port inverse160 t2_20/n1_8 and inverse192
t3bv_8_avx2_128/t1bv_6_avx2/n1_4 using actual native operation order; inspect
the backward t1b.h header instead of inferring twiddle kind from codelet name.
Export unnormalized native intermediates and compare GPU before composing the
already verified fixed30720 normalization against officialifft2 for12streams.
The192-FIRST transposed orientation remains unported.120sampleDC/mean,10x12
residual FFT andmap integration still pending before public80x96/96x80quality.

## Latest continuation: isolated real160 GPU first axis

- Fresh checkpoint: `validation/mixed160_axis_639283655659964334`; active and
  completed pointers `validation/mixed160_axis_{active,completed}_path.txt`.
  Local only; no commit/push or production/public-guard changes.
- Verified local FFTW3.3.10 archive SHA256 against the pinned hash. Added
  original GPL sources r2cf_20/r2cfII_20/hc2cf_20 under
  tests/fftw_reference/mixed160, with SOURCES.json LF hashes/provenance.
- New generate_fft160_forward_axis_shader.ps1 derives the scalar20x8 graph
  from pinned sources plus existing r2cf_8. Nonfused Float32 operation order,
  precise temporaries, once-rounded host-double trig twiddles. Output compiled
  by glslangValidator Vulkan1.2 and passes spirv-val. No old shaders overwritten.
- inspect_fftw_plan.cpp adds --forward160-axis <stage0.txt> <fresh-output>.
  Local Octave FFTW DLL, threads3, fresh ESTIMATE|UNALIGNED, 192 columns of160,
  compact81 complex per column. Actual plan: three64-column groups of20x8.
  plan_structure_check.txt confirms whitespace-normalized complete isolated
  plan is a subtree of prior matched rank2 graph; all12 stream plans agree.
  Still a fresh candidate, NOT recovered original Octave cached plan objects.
- verify_fft256_inverse.cpp adds isolated --forward160-axis; old modes retained.
  Strict build targets fft256-inverse-test and dimension-native-plan-test
  succeeded in fresh build_verified/build_native dirs, MSBuild escalated.
- Actual GPU Intel Arc Pro Graphics. All12 streams (asymmetric,impulse,HDR-ramp
  xref/test xMTF/LA, h80_w96 fixture family) match 373,248 Float32 values
  numerically with zero mismatches/max error. PASS/results.tsv/per-streamlogs,
  binaries/SPIRV hashes preserved. No signed-zero bit claim for this checker.
- Equivalence: prior stage0 already symmetrically postpadded(MTF) or replicate
  postpadded(LA); input160x192 Float32 interleaved column-major, imag0, text
  header160 384. Output192x81 complex column-major, textheader192 162.
  Forward unnormalized, no extra padding/filter/second axis/inverse/DC here.
  These are prior factor1.05 native component fixtures, NOT fullquality pairs.
- Reproduction commands and scope: tests/MIXED160_AXIS_VALIDATION.md.
  Generator launch initially hit PowerShell execution policy; explicit process
  Bypass succeeded. Sandbox configure failed to find MSVC (build/configure.log);
  fresh elevated build_verified succeeded. Preserve failed artifacts.
  Historical work and stagedshader/dirtyVkFFT preserved; no auto-publication.

Exact next step: port 192-point complex SECOND axis for160x192 using the pinned
native t3fv_8_avx2_128 -> t1fv_6_avx2 -> n1_4 graph and its actual SIMD/FMA
ordering, then compare complete forward output against independent official
fft2 for these12 streams. First-axis pass alone does not establish fullforward
parity. Inverse160/192 still pending;192-FIRST transposed shape needs its own
graph. Fixed30720 normalization and padding already pass separately.120-sample
DC/mean,10x12 residual FFT and map contracts remain before public80x96 support.

## Latest continuation: mixed FFT plan census and exact30720 GPU division

- Freshcheckpoint validation/mixed_fft_plans_639283629886514449;
  active/completed pointers validation/mixed_fft_plans_{active,completed}_path.txt.
  Localonly,no commit/push. No publicAPIguards orproductionFFT/mapcodechanged.
- Extended nativefilterfixture generator with explicit fifthargument --mixed
  permittingonly80x96/96x80;defaultpriorguards retained.6fixtures:2orientations
  xasymmetric/impulse/HDR-ramp,ref/test factor1.05,bothMTF/LA.24nativechains,
  exactofficialstaged assertions,Float32input/doublefilters,30ppd,3threads,
  estimate/freshsinglewisdom. Stage0/2columnmajor,forward/inverserowmajor,
  postpaddingMTFsymmetric/LAreplicate,FFT160x192/192x160,unnormalizedforward,
  inverseonce1/30720,noDC. Notfullqualitypairs.
- Added --mixed-plan mode to existing Windows/OctaveDLL nativeinspector and
  targetdimension-native-plan-test. Cdimensions{W,H},compactoffsetW*((H-1)/2).
  For eachfixture32configurations (labels/chains,3/1threads,alignmentflags,
  inversein/outplace);maincensus192.8perfixturematchforward+inverse(48matched
  configurations,24uniquestreams/twothreadsettings). Selected3threadunaligned
  outplace yields2,949,120numericallyexactFloat32forward+inversevalues.
  Do notclaim signedzerobits forCPUFFTplancomparisons.
- IMPORTANT:wisdombufferimportworks,butallWISDOM_ONLYattemptsreturnnull for
  thesecontracts. MatchingplansarefreshESTIMATE candidates,NOTconfirmednative
  cachedplanobjects.32candidatePRESERVE_INPUTprobealsoallcachemiss;defaultmode
  regressionresultstableafteraddingoptionalprobe. Rawplans,strides,alignment,
  controls/results preserved. Originaltoolmodes unchanged.
- Planexample for160realaxis:r2cf_20/r2cfII_20/hc2cf_20,r2cf_8;192complexaxis
  t3fv_8_avx2_128,t1fv_6_avx2,n1_4;inversefirstaxis t2_20/n1_8 and192inverse
  t3bv_8_avx2_128,t1bv_6_avx2,n1_4. Seefull exact_plan_example.txt,notjustthis
  codeletinventory. ChosenCPUgraphs arefixture-arithmetic-equivalent only.
- Normalization discovery:Float32divisionby30720matchesnativeifft2;Float32
  reciprocalmultiply causes35721..37312mismatches in80x96asymmetricstreams.
  New tests/mixed_normalization.comp usesuint32significanddivision by15 plus
  power-of-twoexponentscale,explicitRTEnormal/subnormal rounding,finiteinputs.
  NoFloat64/Int64/FPexecutionmode requirements,noNaN/infinityparityclaim.
- ActualGPU IntelArcProGraphics. Final gpu_normalization_bits passes1,054,696
  syntheticIEEEfinitewordprobesbitwise plus1,474,560nativeinversewordsbitwise,
  signedzerosincluded. Syntheticallfiniteexponents/bothsigns andedgecontrols,
  notexhaustiveallmantissas. HoststrictvolatileFloat32division,RTE/gradual.
  Nativeinputs areCPUunscaledFFTcandidates;notGPUFFT orproductionCPUfallback.
- Added mixed-normalization-test target andvalidate_mixed_fft.ps1 reusable
  orchestrator. Fullend-to-endrun runner_verified/PASS succeeds. Lateroptional
  preserveprobe onlychangesdiagnosticflag;defaultregressionresultstable.
  Frozenbinaries/source/nativehashes andCOMPLETED/PASS recorded.
- Failednon-numericalattempts preserved:gpu_normalization ran syntheticexact
  thenfailedreadingmanifest paths (std::quoted treatsWindowsbackslashes as
  escapes);normalize_manifest_windows_quoted retainsoriginal. Runnerusesforward
  slashes. gpu_normalization_verified launchstoppedonPowerShell informational
  Vulkanstderr withStop preference;freshCompleted/bits/runnerexports pass with
  propernativeexitchecks. Noactualshadernumericmismatches. Initial32casecensus
  andpreassert-bitpasses arehistorical;finalbitchecksandrunner areacceptance.
- Buildfreshcheckpoint/build,Release,MSBuildapproved;spirv-valVulkan1.2 passes.
  Onlynewshadercompiled;priorbinaries/references/stagedshader/dirtyVkFFT retained.
  Docs MIXED_FFT_PLAN_VALIDATION.md,MASKING_QUALITY_VALIDATION.md. Public80x96
  remainsrejected;thisisnotfullgeneral-dimensionGPUquality.

Exact next step: extractnativefirst-axis compact half-spectrum fixtures for
160realFFT andport scalar r2cf_20/r2cfII_20/hc2cf_20 composition withr2cf_8,
preservingoperationorder/twiddles. Also inspect192-first-axistransposedplan,
do notassumesamegraphbyshape-swap. LocatepinnedFFTW3.3.10sources fornewcodelets,
keepGPLprovenance,portfirstaxis andtestbeforesecondaxis/inverse. GPUfixed30720
normalization andgeneralpadding arealreadyisolatedpasses.120samplemean/DC,
10x12residualFFT andmapcontracts remainafterFFT;keeppublicguards unchanged.

## Latest continuation: general-dimension references and exact GPU padding

- User requested continuing into general dimensions after CLI publication.
  Fresh checkpoint:validation/general_dimensions_639283336574679292;
  active/completed pointers validation/general_dimensions_{active,completed}_path.txt.
  New work local only,no commit/push. No production algorithms or API guards
  changed;original stagedshader,dirtyVkFFT andallpriorreferences retained.
- Added generate_general_dimension_reference.m:8shapes63x65/65x63,80x96/96x80,
  127x129/129x127,192x256/256x192. Records native bandgeometry,exact FFT sizes,
  factorization,residual samples,singleFFTWwisdom,PFMs andindependent official
  quality results. Positiveasymmetricreference/localizedtest*single1.4+single.25,
  30ppd/age24,D65,use_gpu=false,official3.07. Nativefullscores areNOTGPUclaims.
- Exact FFT size is2H x2W,notnextpowerof2. Residualsizes correspondingly8x9/9x8
  (72samples),10x12/12x10(120),16x17/17x16(272),24x32/32x24(768). Existing
  64/128/256 mean/DC and8/16 residualFFT contracts are insufficient.
- First isolated general GPU implementation:tests/dimension_padding.comp,
  ABI1701. Endpoint-inclusive symmetric/replicate postpadding to2H x2W and
  row/column-major complex packing,explicitpositiveimaginaryzero. Pureuintword
  copies,noFPmath/Float64/Int64 capabilities,log/exp/filter/FFT/DC/inverse not
  partofthisunit. Separate test project tests/hdrvdp3_dimensions compilesonly
  thisshader,notcompile_shaders.sh. Noexistingbinaryoverwrites/deletions.
- Native padarray fixtures3patterns per8shapes(asymmetric,signedcornerimpulses
  withnegativezero,HDR1e-4..1e5),2paddingmodes,2layouts:96cases. GPU checker
  passes14,843,520 Float32words bitwise(7,421,760real+samepositiveimaginaryzero),
  including2,096negativezerowords. Actualoutputs/percaserecords preserved in
  gpu/;gpu.log andPASS establish completion. No failed numerical runs.
- IMPORTANT device scope:actualVulkanselected NVIDIA RTX A500 Laptop GPU for
  thisrun (seeGPUlog),NOTIntelArc. Earlier full-metricArc acceptance remains
  separate. Thisonlyextends padding/layout bitcopy component coverage toNVIDIA.
- Build freshcheckpoint/build,Release,MSBuildapprovedescalation. Native generator
  andbuildcompleted successfullybeforeGPUrun. spirv-val Vulkan1.2 passes.
  NativeFFTWsingle3threads/estimate;freshsinglewisdom beforeeach fullnative pair,
  savedbeforeseparate SPYRgeometryprobe. Layout,rowmajorinputUInt32representation,
  complexoutputsrow/columnmajorinterleaved;noDC. NoCPUstage substitution.
- Reproduction:Octave generate_general_dimension_reference.m <fresh-native>;
  cmake -S tests/hdrvdp3_dimensions -B <fresh-build>;builddimension-padding-test;
  exe <native> <build/dimension_padding.spv> <fresh-output>. Externalofficial
  reference/Octaveimage/statistics asusual. Docs GENERAL_DIMENSION_FRONTIER.md
  andMASKING_QUALITY_VALIDATION.md. Full native dataset hashes/shader/exe/source
  hashes andCOMPLETED/PASS recorded undercheckpoint.

Exact next step: use80x96/96x80 asfirstnon-power-of-two quality target. Inspect
native160x192/192x160 FFTplans with explicit strides/alignment/threading,wisdom
inventoryaloneisnotanexecutiongraph. Extend a diagnostic native FFT plan tool
or addone,and exportisolatedaxis/nativeFFTfixtures beforeporting sourcegraphs.
Nativewisdomalreadyshowsr2cf_20/r2cf_10,AVX2t1fv_6/t3fv_8,n1_4 etc;do not
assumea5x32/3x64 decompositionmatchesoperationordering. Laterneed120sample
mean/DC/baseadaptation,10x12 residualFFT,bandresize/masking andfullQ/JOD.
Keep existingpublicdimensionguards untilfullstageparity;no tolerancerelaxation.

## Latest continuation: publish the explicit CLI checkpoint

- User explicitly requested committing and pushing the completed CLI work
  before starting general image dimensions. Scope is11 paths listed in
  validation/quality_cli_639283326163455781/publish_paths.txt:CLI parser/types,
  main/help,quality wrapper,parser test,CMake test target,application replay
  driver andthree validation documents plus this handoff.
- Validated source/binary SHA256 hashes were checked unchanged before
  publication. Parser rerun passes5valid/22invalid cases. Prior numerical
  results remain524 CLI hashes,96legacy hashes,95minimal-default hashes and
  six exact native validators for64square60ppd/age60.001. No extra native run
  or numerical change is claimed in this publication session.
- origin/feature/hdr-vdp3 was fetched with --no-recurse-submodules and matched
  the local parent before the CLI commit. Publication uses scoped staging and
  commit --only;the independently staged shader blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7 anddirtyVkFFT remain excluded.
- Actual commit ID,push outcome andremote HEAD verification are recorded in
  validation/quality_cli_639283326163455781/publication_result.json. Historical
  local-only status in the earlier CLI development entry is superseded by
  that publication record. Noforcepush/reset/clean orsubmodulecommit.

Exact next step after successful publication: begin the general-dimension
frontier using official3.0.7 references,starting with dimension/padding/FFT
planning equivalence. Keep current bounded CLI/API regression acceptance
intact;do not silently lift guards or claim arbitrary-image parity. Portable
quickstart fixtures andRGB conversion remain separate usability work.

## Latest continuation: explicit luminance quality CLI

- Checkpoint:validation/quality_cli_639283326163455781;
  active/completed pointers validation/quality_cli_{active,completed}_path.txt.
  New CLI work is local,not committed/pushed;previous publication remains
  b3d23f2/febbf05. Original staged shader,dirtyVkFFT andexcludedchanges retained.
- Args adds optional HDRVDP3QualityArgs withruntime/spectral/export directories,
  ppd(default30),age(default24). --input TEST.pfm --ref REFERENCE.pfm plus
  --hdrvdp3-runtime selects explicitfilequality. Optional --hdrvdp3-ppd,-age,
  -spectral-data,-export mapped directly toexisting evaluate_quality. Noquality
  environmentread onCLIpath. Existingenv/syntheticpaths preserved. Helpupdated.
- Parser rejects missingpair/runtime,malformed/nonfinite/unsupportednumbers,
  missingvalues,duplicate/unknownHDRflags,HDRflags onothermethods,--outputimage
  withquality,andnondefaultage withoutspectraldata beforeVulkansetup. Numeric
  parsing consumesfullstring. New verify_hdrvdp3_args test:5valid+22invalid,
  includinglegacyHDRsynthetic andPSNRgenericoptions. Fourrealexecutableparse
  rejects andhelp confirmnoGPUsetup. Existingexportsentinel andunsupported
  rectangle60ppd rejectwithoutqualitydispatch/overwrite.
- CLIwrapperloadsPFMs,selectsFFTfilename bydimensions,usesexplicitpackage,
  typedviewing/spectralparameters,freshoptionalexportdir. Libraryengine/shaders
  unchanged. main/header/Args/wrapper plusindependentCMakeargstest arechanged.
  Fresh build incheckpoint/build,Release,applicationON/diagnosticsOFF,succeeded
  afterMSBuildapproved escalation. Frozenexe/argstest/sourcehashes preserved.
- Five supportedshapes(32/64/128square,bothrectangles) CLIreplays preserve524
  numericalexporthashes andexactqualityfiles,withinvalid legacyenvpaths/runtime/
  spectral/ppd/age settings deliberatelypoisoned. Driver nowhas -CLI and
  -RepresentativeOnly switches;normal11-caseenvpath behavior retained.
- Runnerfinalsummaryinitiallyfailed:ordered dictionarieswerenotpropertyobjects
  forMeasure-Object. All5 casesalreadycompletedandJSONrecordswerevalid. Fixed
  records toPSCustomObject;checkedsummary arithmetic,andfinalizedPASSfromthe
  completeJSON(524). NoGPUTest rerun/relabel/tolerancechange;failedsessionoutput
  recordedinnotes. Legacy32envcall separatelypreserves96hashes. MinimalCLI32
  withnooptionalflags preserves95hashes (extra spectral_source provenance tag
  excluded:legacyage24versusrawD65);automaticfreshdirectory/constantD65works.
- Additional64square at60ppd/age60.001 usingexplicitrawD65 passes6exactnative
  validators:preprocessing,SPYR,CSF,masking,D,officialquality. Q9.7612190246582031,
  JOD9.9169960021972656. Acceptedlogs view60_age60p001_default_plans_* andstage
  resultsJSON. CopiedfrozenGPUexporttonewdiagnosticdirbeforenativerevalidation,
  preservingoriginalcomputedreferencefiles. NotasecondGPUexecution.
- Importantfailedoracleprotocol:initiallyusedrectangularsix-checkdriveronthis
  squarecase;its--fresh-fft-plans resetwisdomandcaused17,576 preprocessingstage
  mismatches. Establishedsquaredefaultplanprotocol thenmatchesexactly,including
  officialQ/JOD,withoutchangingGPUoutputs/assertions. Initialfailednative/logs
  retainedunder view60_age60p001_native andoriginalexport. Neverclaimsresetplan
  resultsaspass;FFTWplanning/alignmentconditionsremainparityscope.
- Initialbuildattemptincorrectlytargetedoldsource-snapshotbuildandhitMSBuild
  sandboxfailure beforecompile. old_snapshot_build.log notatestpass. Allaccepted
  CLIexecutionsusefreshcurrent-sourcecheckpoint/build aftersuccessfullink.
- Scope:positiveFloat32PFMabsolute luminance,topdownrowmajorAPI,30ppd/age24for
  extendedsizes;newviewcase64square60ppd/age60.001;D65calibration/constants,
  localIntelArc,nativeOctave3threads/estimatewithestablishedper-shapeFFTplan
  protocol. Padding/layout/DC/inversenormalization unchangedfromearliercheckpoints.
  NoRGB/general-dimension/otherdevice/signedzero-bitparityclaim.
- Docs:tests/CLI_QUALITY_VALIDATION.md,APPLICATION_LUMINANCE_VALIDATION.md,
  MASKING_QUALITY_VALIDATION.md. Build:independent project applicationON then
  targetsIQM-hdrvdp3/IQM-hdrvdp3-args-test. Minimalcommand:IQM-hdrvdp3 --method
  HDRVDP3 --input test.pfm --ref reference.pfm --hdrvdp3-runtime matchingruntime.

Exact next step: make a portable quickstart/smoke fixture workflow that generates
small deterministic luminance PFMs andmatchingruntimepackages without depending
on historical validation/checkpointpaths;documenthowtoobtainofficialnative
oracle separately. Keep CLI/library arithmeticunchanged. Rootmulti-metricCMake
cleanup,arbitrarydimensions,RGBandotherGPUcoverage remainseparate. Commit/push
thisnewCLIcheckpoint onlywhenrequested;thepriorpublicationdoesnotcontainit.

## Latest continuation: reviewed checkpoint committed and pushed

- User explicitly requested commit and push. The prepared102-file scope is
  committed as febbf05a03d2494d81a4bbb94a07aeb0fa827df7,
  "Extend bounded HDR-VDP-3 quality and luminance application input".
- Normal push to origin/feature/hdr-vdp3 succeeded (11797c1..febbf05),including
  five earlier local FFT128/256 validation commits. No force push or history
  rewrite. Commit paths were checked against selected_paths.txt exactly.
- Original staged hdrvdp3_corrdn_test.glsl blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7 remains staged,with its unstaged
  changes intact. Dirty VkFFT and other excluded working/untracked files remain
  local. Only explicitly selected paths were committed;no add-all/reset/clean.
- This publication record is a separate documentation-only follow-up. Evidence
  and remote verification are recorded under
  validation/publish_prepare_639283319336040304/publication_result.json.
- Numerical acceptance is unchanged:clean selected-source API/application
  builds,19 regenerated pinned shaders,620 smoke export hashes and35 rejection
  checks. These are prior measured results,not rerun during publication.
- A first scoped staging command stopped on harmless LF/CRLF warnings because
  PowerShell treated native stderr as terminating. Staging had completed;the
  successful scoped commit used exit-code checks. No source or index content
  was reset,and the independently staged shader was verified afterward.

Exact next step: resume normal CLI luminance PFM/runtime/viewing arguments using
the same explicit API. The supported clean build and remaining RGB/general-size/
root-multi-metric limitations are documented in tests/PUBLISH_READINESS.md.

## Latest continuation: selected-source publish preparation

- User agreed to prepare a scoped GitHub checkpoint before more CLI work.
  No staging,commit,push,reset orclean performed. Original index remains only
  the independently staged hdrvdp3_corrdn_test.glsl blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7 (93 addedlines). Its unstaged
  edits and dirtyVkFFT/CMakeLists plusuntrackedglslang remain untouched.
- Checkpoint:validation/publish_prepare_639283319336040304;
  active/completed pointers validation/publish_prepare_{active,completed}_path.txt.
  selected_paths.txt is the102-file proposed commit manifest;excluded_paths.txt
  identifies other working changes toretain outside this change. Includes
  runtime library/application wrapper,CMake source integration,generators/tests,
  docs and.gitignore. Excludes bin/CMakeLists rootexperiments,mtfgpu test edits,
  stagedshader,dirtysubmodule,localglslang,allvalidation/build/binary artifacts.
- Exported HEAD688eff7085661c8f3427434e12c37b081e6f3ca7 via gitarchive,overlaid
  onlyselectedfiles andclean VkFFT539be29f30ef40384f3ccd24d7884166f82cbef8
  viaitsgitarchive. Notacommittedcheckout/worktree. Snapshot/selectedsource
  hashes andscope manifests retained. No working submodule files copied.
- Newoptional IQM_HDRVDP3_BUILD_APPLICATION in independent
  tests/hdrvdp3_evaluation CMake buildsIQM-hdrvdp3 fromsame main,args,filematcher,
  Vulkaninstance andwrapper,COMPILE_HDRVDP3 only. DefaultOFF;no rootbenchmark
  CMake or othermetric/dependency workarounds. Both diagnostic flagsOFF.
  Clean API/apptargets buildsuccessfully under snapshot/build;MSBuildapproved.
  This doesnotvalidate fullrootmulti-metricbuild. README clearly states scope.
- All19 runtime shaders regenerated fromsnapshot sources (7bounded,6square128,
  6rectangle) withpinnedidenticalSHA256. No oldvalidationexports asshaderinputs.
  Thirdparty source/license dependencies arecommitted HEADfiles.
- Freshclean APIandapp smoke runs32square,64x128,128x64:6calls retain620
  numerical export hashes (2*(96+107+107)),includingqualityfiles. APIscorebits
  nativebaselineexact. InputPFMs/goldens/spectraldata from prioracceptedlocal
  checkpoints,explicitexternaltestinputs;not newnativeoracle executions.
  Thirtyfive invalidAPI cases pass. SMOKE_PASS/smoke_results andlogs preserved.
- Firstsmokelaunch stopped becausePowerShell ErrorActionPreference=Stop
  treatedinformational Vulkanstderr asNativeCommandError. Initialpartiallog
  excluded;validate_clean_snapshot.ps1 usesContinue fornativecalls andchecks
  exit/errors;fresh *_verified_* exports passed. No numerical changes/tolerance.
- .gitignore nowcovers localvalidation/build-vs2026/build-hdrvdp3,CMakecache,
  root.obj/.exe/.log outputs;files keptondisk. Proposedcommit message preserved
  undercheckpoint. No automatic staging ofuntracked backups/binaries.
- Docs:tests/PUBLISH_READINESS.md (cleanbuildcommands,scope/externalfixture
  dependencies),MASKING_QUALITY_VALIDATION.md. Source snapshot finaldocs synced
  aftertesting;production numerical sources unchanged after successfulbuild.

Exact next step: reviewselected_paths.txt andperform the scopedcommit/push when
requested. Preserve originalstagedshader bycommitting onlyselectedpaths;never
usegitadd-all oracommitthatincludesunrelatedstagedcontent. The supportedclean
buildcommand is cmake -S tests/hdrvdp3_evaluation -B build-hdrvdp3
-DIQM_HDRVDP3_BUILD_APPLICATION=ON,then Release targetsIQM-quality-api-test and
IQM-hdrvdp3. Rootmulti-metricbuild cleanup stays separate. After checkpoint
publication,resume explicitCLI luminance/runtime/viewingarguments aspreviously
planned. Historical acceptance below remains separatelyscoped.

## Latest continuation: application PFM quality entry and runtime directory

- Accepted checkpoint:validation/rectangular_app_639283313608789740;
  active/completed pointers validation/rectangular_app_{active,completed}_path.txt.
  Root COMPLETED/PASS,source/binary hashes andvalidated_binaries/IQM.exe preserved.
- bin/shared/wrappers/hdrvdp3.cpp extracts run_quality_inputs helper. Explicit
  IQM_LUMINANCE_REFERENCE/TEST request goes straight toquality instead of
  unrelated component diagnostics. Bothpaths required. PFM dimensions select
  matching32/64/128square or64x128/128x64;noimplicitresize. Synthetic entry
  retains oldcomponenttests andIQM_SPYR_SIZE32/64 behavior,then samehelper.
- New optional IQM_QUALITY_RUNTIME supplies explicit package paths and
  column-first/native referenceFFT config. Bounded usesfft32/64.spv;extended
  usesfft.spv;all usemap.spv,base.spv,math/. Ifomitted,legacyindividualbackend/
  shaderenvironmentsettings remain. Viewing/spectralrules unchanged. Prints
  absolute Quality export directory aftersuccess. Matchingpackage must bechosen;
  thisisnotautomaticassetinstallation orarbitrarydimension support.
- Two successful root IQM builds (initialshape/runtime,thenfilehelper);waited
  forfinalcompletion beforelaunch. Buildlogs preserved;MSBuildescalationapproved.
  FinalexeSHA25669C8AA7708C40C4457B0F7903EDE00F7FF56B63D0B6C38093CE37967B37707FE.
- validate_application_quality.ps1 runs11 normalapplication PFM pairs:3square
  plus8priorrectangularpairs. Allqualityfiles and1,166 nonpath/nontag text
  exports matchacceptedreferences byhash (96+10*107). Staleinvalidsynthetic
  size/backend settings deliberatelyignored forfile+runtime. file_cases/results
  recordsoriginalfreshexports under validation/spyr_spatial_*. Nativeacceptance
  inheritedthroughmatchingoutputs;not11newindependentnativepairs.
- Sevennegativeapplicationrequests:missingpair(2),mismatchedsizes(1),invalid
  rectangularppd60(2),unsupported3x2/64x32(2). Exitnonzero,noqualityexports,
  noVulkanvalidationerrors. Fixtures/logs andinvalid_cases.json preserved.
- Synthetic32/64 asymmetric/localized strictnative preprocessing/CSF/masking/D/
  officialquality and13JNDstages pass.32Q=8.3328084945678711,JOD=8.9990530014038086;
  64Q=9.1295385360717773,JOD=9.5646781921386719. IndependentABI13GPUchecker
  32:62cases57,046floats exact+2,368binary64bits;64:74cases228,055floats exact+
  9,536binary64bits. Checker frozenpreviousage_ppd60 verifiedbinary,hashrecorded.
- Importantfailedtoolrun:default run_quality_validation.ps1 pointsobsolete
  fft_publish_build checkerrequiringABI12.32nativecheckscompletedthenchecker
  rejectedABI13asset. Failedsuite quality_gpu_base_fft_suite_32_selected_
  639283315003038815 retained;its completedexport spyr_spatial_17927347033377657
  passes correctABI13checker incheckpoint/synthetic32_checker13.log. No32suite
  PASSfabricated.64usesexplicit -GPUChecker age_ppd60_639280954526250977/
  verified_bin/IQM-map-gpu-test.exe andPASSatquality_gpu_base_fft_suite_selected_
  639283315931737468. Needalwayspassmatchingchecker tolegacydriver.
- Precision/layout/padding/DC unchanged:positiveFloat32PFMs,bottomupdisk/topdown
  rowmajorAPI,localIntelArc,30ppd/age24/D65. MTFsymmetric/LAreplicatepostpadding,
  inverseonce,columnmajorspectrumtranspose;rectresidual8x16/16x8no padding,
  native mean/DC/firstmax. Signedzerobitclaimunasserted. NoCPUstage substitution.
- Docs:tests/APPLICATION_LUMINANCE_VALIDATION.md,MASKING_QUALITY_VALIDATION.md.
  Noreset/clean/commit/push/blanketshaderbuild;preexistingchanges/stagedshader/
  dirtyVkFFT preserved. PublicAPIearlierchecks remainhistorical scope.

Exact next step: provide normal CLI arguments for the luminance PFM pair,
runtime package and viewing parameters instead of requiring environment setup;
route them into the same explicit API without changing numbers. Check existing
bin/IQM/args.cpp and shared Args first,keep allothermetricbehavior intact,and
test argument errors plus representative32/64/128/rectangularpairs against this
checkpoint. RGB,arbitrarydimensions,automaticruntimeinstallation andotherGPU
coverage remain separate;boundedluminance path nowworks inactualapplication.

## Latest continuation: bounded rectangular public quality API

- Accepted checkpoint:validation/rectangular_public_639282926432216545;
  active/completed pointers validation/rectangular_public_{active,completed}_path.txt.
- evaluate_quality now permits64x128/128x64 onlyat30ppd/age24. Removed private
  allow_rectangle switch;public and retained diagnostic wrapper call identical
  implementation. Profile16 selection andexplicit FFT64256/base85 behavior
  are unchanged from validated diagnostic path. Header documents public scope.
- Fresh non-diagnostic CMake build in checkpoint/build,Release,with both
  IQM_HDRVDP3_DIMENSION_DIAGNOSTIC and IQM_HDRVDP3_DIAGNOSTIC_128 OFF.
  Configure/build completed successfully via approved MSBuild escalation.
  Frozen executable in validated_binaries/;source/binary hashes preserved.
- Eight public replays retain native score/error bits and107 numerical exports
  each(total856),including asymmetric/localized,impulse/localized,HDR/localized,
  identity inbothorientations. Environment deliberately poisoned by caller.
  Prior independent native six-stage acceptance is inherited through matching
  exports;native validators were not rerun and not counted as new native pairs.
- First process stopped after4 completed64x128 cases;partial128x64 output remains
  under public_pairs/ and is excluded. Fresh public_pairs_resumed/ holds all4
  accepted128x64 cases. Root public_results.json combines exactly8 cases.
  No accepted export overwrites or reuse. Root COMPLETED.json/PASS.txt records
  final scope;noVUID/Validation Error/[ERROR] inaccepted numerical logs.
- Current public square32/64/128 regressions pass fresh score bits and
  96/107/107 numerical export hashes. Tests reject18 rectangular dimension,
  viewing andmissingasset cases,3square128 parameters,14legacy cases(total35)
  before evaluation dispatch/output overwrite. Unsupported/odd rectangles remain
  rejected. Existing MapGPU ABI rejection coverage remains previous checkpoint.
- generate_assets_rectangular.ps1 now labels the public bounded package;
  checkpoint/runtime regenerates6 shaders with unchanged pinned hashes and
  compiler/ABI/licenses metadata. First4public replays used bit-identical
  priorruntime_reproduced;resumed4used newruntime. No shader numerical edits.
- Reproduce public replays with validate_rectangular_public.ps1 -Executable
  <normal-build/IQM-quality-api-test.exe> -RuntimeDirectory <runtime>
  -OutputDirectory <fresh-dir>. Optional -Shapes h128_w64 resumes thatorientation
  in a fresh directory. Default runsboth. Existing square regression script used.
- Precision/input/viewing/layout/padding/DC equivalence remains previous8pairs:
  positive Float32 absolute luminance,30ppd/age24/D65,local Intel Arc;MTF symmetric
  andLAreplicate post-padding,inverse1/32768 once,column-major spectrum/filter
  transpose,residual8x16/16x8 no padding,native mean/DC/firstmax. Binary64 masks.
  No new signedzero-bit/device/RGB/general-size parity claim.
- Docs:tests/RECTANGULAR_PUBLIC_API_VALIDATION.md andMASKING_QUALITY_VALIDATION.md.
  No reset/clean/commit/push/blanketshaderbuild;allpreexistingchanges,stagedshader
  anddirtyVkFFT retained. Historical public-square-only statements below are
  superseded onlyfor64x128/128x64@30ppd/age24.

Exact next step: integrate this bounded API and reproducible runtime package into
the application luminance-file entry point. bin/shared/wrappers/hdrvdp3.cpp
FullSpyrTest stillaccepts onlyIQM_SPYR_SIZE32/64 and enforces squarePFMs despite
calling evaluate_quality. Preserve synthetic32/64 behavior;derive explicit file
dimensions for supported32/64/128square and64x128/128x64,selectmatchingpinned
runtimeassets,then validate normal application calls withfreshnativecomparison
andlegacyregressions. Arbitraryimages/RGB/otherdimensions anddefaultinstallation
remain separate work;no claim that the entire metric/product scope is complete.

## Latest continuation: six rectangular stress pairs and reproducible package

- Accepted checkpoint: `validation/rectangular_stress_639282921084448635`;
  active/completed pointers: validation/rectangular_stress_active_path.txt and
  validation/rectangular_stress_completed_path.txt. Fresh native_* and gpu_*
  directories per pair; stages_*/PASS.txt and results.json record all checks.
- Six new pairs: both64x128/128x64,impulse/localized,HDR-ramp/localized,
  asymmetric/identity. All36 exact validators pass (preprocessing,SPYR,CSF,
  masking,D,official quality). Native Q/Q_JOD bits match in every pair.
  Prior2 asymmetric/localized pairs remain separate:total8 unique full pairs.
- Scores (Q,Q_JOD):64x128 impulse(2.7487683296203613,3.4179229736328125),
  HDR(8.3044567108154297,8.9771928787231445),identity(10,10);
  128x64 impulse(2.3844747543334961,2.9913034439086914),
  HDR(8.4348869323730469,9.0768861770629883),identity(10,10).
  Both identity exports have five zero band errors and zero aggregate error.
- Extended generate_rectangular_quality_reference.m with optional identity
  transform;default localized behavior retained. New reusable full stress driver
  tests/hdrvdp3_evaluation/validate_rectangular_stress.ps1 requires fresh root.
- Added generate_assets_rectangular.ps1 and pinned
  REFERENCE_SHADER_HASHES_RECTANGULAR.json. runtime_reproduced/ regenerates
  all6 FFT64256/map16/base85/math shaders from repo sources with identical
  accepted SHA256 hashes,plus licenses/compiler/ABI metadata;no prior validation
  directory used as shader source. GPU stress uses frozen previous executable
  and accepted binaries,bit-identical to the regenerated package.
- Numerical engine/shaders/public API unchanged this session;no rebuilding or
  redundant square regression. Prior checkpoint's96/107/107 public exporthash
  and19 public/20map rejection acceptance remains historical,not a new run.
  All accepted GPU logs free of VUID/Validation Error/[ERROR]. Root COMPLETED,
  PASS,source/native/binary hashes preserve provenance. No failed stress cases.
- Same input/precision/viewing/layout/padding/DC equivalence as prior checkpoint:
  local Intel Arc,Float32,30ppd,age24,D65,native3threads/estimate with fresh
  single FFTW wisdom per independent full chain. HDR input range1e-3..1e4;
  localized change*single1.4+single.25. Identity pixels/PFMs equal. MTFsymmetric
  andLAreplicate post-padding,inverse1/32768 once,column-major FFT spectrum;
  residual8x16/16x8 unpadded,mean removal,DC magnitude zero,firstmax. Binary64
  maskresize exact;float signedzero bit identity remains unasserted.
- Docs:tests/RECTANGULAR_STRESS_VALIDATION.md,MASKING_QUALITY_VALIDATION.md.
  Reproduce from workspace using validate_rectangular_stress.ps1
  -OutputDirectory <fresh-root>;package using generate_assets_rectangular.ps1
  -OutputDirectory <fresh-root> -VulkanBin C:/VulkanSDK/1.4.363.0/Bin.
  No reset/clean/commit/push/submodule changes or blanket shader compilation.

Exact next step: expose only64x128/128x64 at30ppd/age24 through public
evaluate_quality using the now-reproducible explicit FFT64256/map16/base85
package. Add invalid rectangular viewing/dimension/asset tests,build a
non-diagnostic caller,compare public outputs/scores to these8 frozen full pairs,
and rerun square32/64/128 regressions. Keep other rectangles/odd sizes/RGB and
viewing parameters rejected. Public API is still square-only at this handoff.

## Latest continuation: rectangular maps ABI16 and first full-quality diagnostic

- Fresh checkpoint: `validation/rectangular_maps_639282910800030657`;
  active pointer `validation/rectangular_maps_active_path.txt`.
- Actual MapGPU ABI16 component checks pass: 128 masking cases/696,320
  Float32 values;160 float-resize cases/487,424 values;64 binary-mask resize
  cases/174,080 binary64 values. Zero numeric float differences and zero
  binary64 bit differences. Float signed-zero bits not asserted. Shapes are
  64x128/128x64 plus their half-resolution bands down to 8x16/16x8.
- Added explicit RectangularMaps=16. Pool traversal follows longer axis;
  FMA selection uses max(width,height)>=32. Explicit rectangular binary-mask
  guards allow same-aspect dyadic downsampling only. ABI13/14/15 retained;
  regenerated14/15 hashes unchanged. Map16 SHA256:
  338EB96787DFAFFC10CC2034DCE767090C4379570C86A8EA3FA6C24E9A1D4614.
- Diagnostic-only quality integration uses FFT64256, Map16 and residualFFT85,
  explicit width/height for residual spectrum/first-max. evaluate_quality_impl
  allows rectangles only for the CMake-gated diagnostic wrapper; public
  evaluate_quality passes false and retains all previous guards.
- Two independent official full references: asymmetric reference + localized
  test*1.4+0.25,30ppd/age24/D65. Native quality pairs and bottom-up Float32 PFM
  are in native_quality_h64_w128 and native_quality_h128_w64.
  GPU full_fixed_h64_w128: Q=4.2055172920227051,Q_JOD=5.0616965293884277;
  full_fixed_h128_w64: Q=3.6234698295593262,Q_JOD=4.4173946380615234.
  Both match independent official score bits. Earlier full_* launch used
  nonexistent ref.pfm (actual name reference.pfm), failed before evaluation;
  failed logs preserved and fresh full_fixed_* used.
- Important oracle finding: old tall-image preprocessing validator failed
  with94/215 retinal mismatches because earlier JND FFT diagnostics populated
  single FFTW wisdom. Fresh-process minimal MTF comparison matches exactly.
  New explicit --fresh-fft-plans resets single wisdom/sets3threads immediately
  before native preprocessing or official quality; no equality weakened.
  Tall full preprocessing then passes all stages with zero differences.
  Old quality validator also reused plans and produced Q=3.6234650611877441;
  its failed evidence is preserved separately. Canonical exact stage logs use
  exact_stages_h64_w128 and exact_stages_h128_w64 with the new six-check driver.
  Both canonical six-check runs completed successfully: strict preprocessing,
  SPYR,CSF,masking,D and official quality all exact. Root COMPLETED.json/PASS.txt
  and validation/rectangular_maps_completed_path.txt identify accepted evidence.
- Current public square32/64/128 replays pass score bits and96/107/107 numeric
  export hashes;19 public invalid cases and20 MapGPU profile/domain contracts
  pass. Exports/logs,source hashes and frozen validated executables preserved.
- Reproduction: generate_rectangular_maps_reference.m;
  generate_map16_shader.ps1;validate_rectangular_maps.ps1 (fresh outputs).
  Build map128-api-test in fft256_inverse_639281620198383169/build and
  IQM-quality-rectangular-diagnostic/IQM-quality-api-test in
  spyr_dimensions_639282601672947206/build (Release,diagnostic config).
  generate_rectangular_quality_reference.m generates independent full native
  references. tests/hdrvdp3_evaluation/validate_rectangular_quality.ps1 runs
  six exact validators with explicit native plan reset and fresh log directory.
- Precision/layout/padding: Float32 local Octave3threads/estimate;row-major
  exported maps,explicit column-major FFT spectrum/filter transpose;MTF symmetric
  post-padding,LA replicate post-padding,inverse1/32768 once. Residual8x16/16x8
  has no padding,native mean removal,DC magnitude zero and first-max selection.
  Binary comparison-mask resize uses binary64/Q20 weights and bitwise checks.
  No CPU substitution of numerical GPU stages. Scope is two unique full pairs
  on local Intel Arc;no general dimensions/RGB/other devices claim.
- Docs: tests/RECTANGULAR_MAPS_VALIDATION.md and MASKING_QUALITY_VALIDATION.md.
  No reset/clean/commit/push/blanket shader rebuild; staged shader and dirty
  VkFFT retained. Historical scope below remains historical.

Exact next step: extend full rectangular diagnostic to impulse/HDR-ramp and identity pairs
with fresh native/GPU exports before considering public rectangle support.
Keep public guards until that validation and explicit packaging work complete.

## Latest continuation:128-sample reductions and MapGPU residual ABI15/85

- Fresh checkpoint:validation/rectangular_residual_639282893861381197;
  completed pointer validation/rectangular_residual_completed_path.txt.
  Interrupted first launch left anempty reductions128.log;fresh resumed
  exports areidentified bycheckpoint/resumed_run_path.txt. No oldoutput reused.
- Isolated512-case128-sample mean/DC/base-adaptation test compares67,072
  Float32 outputs exactly. Two512-case DC->rectangularFFT->hypot/first-max
  chains (8x16 and16x8) eachcompare328,192 scalars withzero numeric differences,
  including131,072 centered interleaved values/explicit imaginaryzeros and
  197,120 FFT/magnitude/index values. Allstages runinoneGPUcommandbuffer;
  nointermediatehostreadback ornativestagesubstitution.
- Added generate_rectangular_residual_shader.ps1 ABI84 and-MapABI variant85;
  compile_rectangular_inverse.ps1 -Selection [-MapABI]. Reductiongenerator's
  -Include128 andcompile_base16.ps1 -Reductions -Include128 areexplicit;
  default64/256 shaderreproduction retainsSHA256
  822C80471F082A0F07675278610622D0FE8D75A8C55D500EB367554625B61DE9.
  GeneratedABI14 mapretains84620708FF2C431F1CFD7568FEB445248252B57DC1FCC19A149ADB7CAA8D9093.
- MapGPU nowhasexplicit RectangularResiduals=15 profile andnew
  base_spectrum_rectangular/base_selection_rectangular(input,width,height).
  Bothrequire8x16/16x8 andABI15;FFTmodule requires85. DC/baseadaptation accept
  128samples onlyin15;existing13/14 guards remain. Q40profile15 usesQ20
  coefficients insharedbinaryresize,withoutwidening rectangularresizeguards.
- ActualAPI validation:512 reductions compare66,560 values (API omitsDCmean);
  eachorientationuses64 cases(first8controls+56distributed):32,832 values
  includingcentered/DC,spectrum,magnitudes,index. Totalnew-domain132,224,
  zero numericdifferences. APIcallsreadGPUoutputsback betweenmethods;the
  isolatedchains above separatelyvalidate nointermediatereadback.
- ABI13/14 legacy64/256 andABI15 samefixtures allpass331,776 additional
  regressionvalues,keptseparatefromnewdomains. Existing256-sample
  DC->FFT16/selection chainpasses655,872 values withfinalchecker.
- Twelvelegacy and16newMapGPU profile/ABI/dimension/domain contracts reject.
  FreshcurrentpublicAPI32/64/128 replays preserveQ/Q_JOD/errorbits and
  96/107/107 numericexporthashes. NineteeninvalidAPIcases pass. NoVulkan
  ERROR/Validation Error/VUID diagnostics inacceptedruns.
- Scope:Float32,localOctave single3threads estimate;row-major interleaved,
  noFFTpadding,unnormalizedforwardFFT;nativeDCmeanremoval,selectionmagnitude
  DConlyzeroed,rawFFTretained. Residualnonzeroexponents[-90,100],span<=16;
  logdomain[-5,5],span<=16. Zero/constant/cancellation/adjacentmean/extreme
  controlsandrandominputs. Signedzerobitidentity unasserted. NoFloat64/Int64
  shadercapabilities;preserve32/RTE32 required. Nofullrectangularscoreclaim.
- Reproduction:generate_base_reductions_reference.m <fresh-native>128;
  compile_base16.ps1 -Reductions -Include128;compile_rectangular_inverse.ps1
  -Selection for84 or-Selection -MapABI for85;generate_map_rectangular_shader.ps1
  for15 (allfreshOutputDirectory,VulkanBin asrequired). Build map128-api-test
  andfft256-filter-test inexisting
  validation/fft256_inverse_639281620198383169/build,Release. Run
  tests/hdrvdp3_fft256/validate_rectangular_residual.ps1 and
  validate_rectangular_map_residual.ps1 withcheckpointfixtures/shaders.
  Publicregressions:tests/hdrvdp3_evaluation/validate_rectangular_regressions.ps1
  withfreshoutputandcurrent spyr_dimensions_639282601672947206/build/Release/
  IQM-quality-api-test.exe. Buildlogs incheckpoint;MSBuild approvedescalations.
- Acceptedmap15SHA256:2DA5ECB9C861CB395AF4933ED504D3374412F594C4117C72A662E043FBECE111;
  base85:D95DE048BF5C68C1C9C38BB7D34CDF78966EB0BB8583DBF03507032409E99EDE.
  RootCOMPLETED/PASS,isolated/map_api/public_regression manifests,source/native/
  binaryhashes andvalidatedexecutablecopies preserved. Pre-extension executables
  alsopreserved. Finalsourcehashes addcomment/docchanges onlyafterbuild.
- Docs:tests/RECTANGULAR_RESIDUAL_VALIDATION.md;MASKING_QUALITY_VALIDATION.md
  linksbothnewrectangularcheckpoints. Stagedshader,dirtyVkFFT,allpriorchanges
  andreferences preserved;noreset/commit/push/blanketshaderbuild.

Exact next step:generate native rectangularbinary64comparison-mask resize,
 Float32CSF/adaptationresize andmutualmasking/pooling fixtures for64x128/128x64
 andtheir32x64/16x32/8x16/talltransposebands. Validate underanexplicitnew
 rectangularmapcontract whilepreserving13/14/15. Currentpoolshader assumes
 squaretraversal andpc.height>=32 forFMA:width-long16x32 needslonger-axis
 traversal/FMAselection,so donotjustremoveguards. Thenintegrate rectangular
 dominantfrequency/CSF/pupil/pooling andcontinuousnativeQ/Q_JOD. Publicquality
 rectangles remainrejected;generaloddFFT/non-dyadicdoublemaskweights remainopen.

## Latest continuation:rectangular MTF/LA integrated into LocalAdaptFFT

- Evidence:validation/rectangular_filter_639282611436713337,completed pointer
  validation/rectangular_filter_completed_path.txt. Native sixfixtures cover
 64x128/128x64,asymmetric/impulse/HDR ramp1e-3..1e4,ref/test factor1.05,
 30ppd,single3threads estimate. Official staged outputs assert equality to
 fast_conv_fft/hdrvdp_local_adapt beforeaccepting native fixtures.
- Isolated24 chains pass6,488,064 GPU Float32 values and786,432 rounded host
 filter coefficients exactly. Fourcomplexstages plusfinalR32 output,noCPU
 stage substitution,noDC removal;MTF symmetric post/LA replicate post padding,
 inverseonce1/32768,column-major spectrum with explicit filter transpose.
- Added combined shader ABI64256 (forward8/9,inverse3/4). ReproductionSHA256
 5A8C4BE851DC6BE0ABE6B150397DE8B5179ED59A37A07D65756FE5FEFCD2C558.
 Float32preserve/RTE,noFloat64/Int64;oldshaderpins unchanged.
- LocalAdaptFFT nowaccepts explicitreference64x128/128x64 at30ppd withmath
 assets andABI64256. Axislength/batches andfiltertranspose areexplicit.
 DefaultVkFFT andsquarepaths retained. Privateevaluation helperacceptsexplicit
 dimensions fortheCMake-gated diagnostic caller;qualityAPI guards unchanged.
- Actualproduction LocalAdaptFFT repeatsall24 cases successfully in
 production_fixed/:same6,488,064 GPUvalues/786,432 filtervalues,zero numeric
 differences. These arenot additionaluniqueinputs. Initialproduction/failed
 testreader parsingtinyGaussian doublecoefficients asfloat;allGPUstages/output
 ofthatcase matched. Fixedreader usesdouble->float;failedartifacts retained.
- Current-source publicAPI32/64/128 regressions preserveQ/Q_JOD/errorbits and
 96/107/107 numericexporthashes. NineteeninvalidAPIcases andthreewrongFFT
 ABIcontracts rejectcorrectly. NoVulkanerrors inacceptedruns. RootCOMPLETED/
 PASS,source/native/binaryhashes,buildlogs,combinedreproduction andguards saved.
- Newtests:generate_rectangular_filter_reference.m;
 generate_rectangular_reference_shader.ps1;verify_filter_dimensions.cpp;
 hdrvdp3_fft256/validate_rectangular_filter.ps1 (-ThroughLocalAdapt);
 hdrvdp3_evaluation/validate_rectangular_regressions.ps1. Compilecombined via
 compile_rectangular_inverse.ps1 -Combined inafreshdirectory. Build actual
 caller inexisting spyr_dimensions_639282601672947206/build withCMake dimension
 optionON,targetIQM-filter-dimension-test;publiccallerIQM-quality-api-test.
- Addednative128-sample fixtureoption togenerate_base_reductions_reference.m.
 Checkpoint/native_reductions128 contains512 reductioncases andtwo512-case
 expectedFFT8x16/16x8+magnitude/first-index matrices. Referenceonly,notGPU pass.
- Docs:tests/RECTANGULAR_FILTER_VALIDATION.md. Existingstaged/unstagedfiles,
 dirtyVkFFT,oldreferences preserved;noreset/commit/push/blanketshaderbuild.

Exact next step:validate128-sample GPU DC/base-adaptation,thenconnectrectangular
 residualFFT+nativehypot+firstmax underanexplicitMapGPUcontract (retainABI13/14).
 Rectangular resize/masking/CSF/pooling andcontinuousnativeQ/Q_JOD remainopen.
 Do notliftqualityAPIguards fromthesecomponentchecks. GeneraloddFFT and
 non-dyadicdoublemaskcoefficients remainopen.

## Latest continuation:rectangular SPYR and forward/inverse/residual FFT

- Added CMake-gated IQM_HDRVDP3_DIMENSION_DIAGNOSTIC and
  IQM-spyr-dimension-test. A test-only entrypoint in evaluation.cpp uses
  actual production SPYR coefficients/run_corrdn_gpu dispatch. Default API
  has no new entrypoint orwidened guards. Enabled /EHsc in standalone
  evaluation library/test tofix missing exception-unwind build warnings.
- Fresh checkpoint:validation/spyr_dimensions_639282601672947206, completed
  pointer validation/spyr_dimensions_completed_path.txt. All16 SPYR cases
  (eight rectangle/odd shapes at30/60ppd) match339,328 native single get_band
  values exactly. Added native input exports togenerate_dimension_frontier.m.
  Inputs10+cos(x/17)+sin(y/23),reflect1,pyramid height ppd-limited.
- Initial gpu_* comparisons failed because test compared raw GPU bands with
  normalized native get_band. Fixed comparison with exact power-of-two
  normalization raw/2^band. Accepted normalized_* allpass. Initial logs/exports
  preserved/excluded;no production arithmetic fix orsilent tolerance.
- Added source-backed rectangular forward ABI256128,inverse ABI128256 and
  residual ABI83 generators pluscompile_rectangular_inverse.ps1 (-Forward,
  -Base ordefault inverse). Existing128/256/bounded shader pins unchanged.
  Checker fft256-inverse-test nowhas explicit --rectangular-forward,
  --rectangular-inverse and--rectangular-base modes;legacy modes retained.
- Native generate_rectangular_fft_reference.m calls proven Octave extension
  andasserts its output equals ordinary fft2/ifft2 beforeaccepting plans.
  Shapes128x256,256x128,8x16,16x8;single3threads,estimate. Source/native/binary
  hashes,plans andfixtures preserved. Synthetic deterministic random matrices,
  oneper shape;not actual MTF/LA inputs andnotbroad-range validation.
- GPU forward2cases/131072 components,inverse2cases/131072,residual real
  FFT2cases/512:262656 components,zero numeric mismatches. NoFloat64/Int64;
  preserve32/RTE32 required. FFT input already sized,no padding/DC processing;
  shader column-major interleaved,checker converts row-major fixture input and
  output. Inverse normalization once1/32768 inaxis2. No signed-zero bitclaim.
- Exact forward256x128 plan uses n1fv_128 for40vectors andt3fv_8+n2fv_16 for
  3tails pereach43-vector thread partition;128x256 uses t3fv_16+n2fv_16.
  Existing generators already included allneeded source-backed codelets.
- Final checker replays ofbothforward/inverse rectangles pass (4replays,
  excluded fromunique counts). Final checker residual2cases pass. Legacy256
  impulse forward+inverse regressions also pass,131072 components each.
  No Vulkan ERROR/Validation Error/VUID diagnostics acrossaccepted runs.
  Checkpoint COMPLETED.json/PASS.txt records scopes,hashes andexcludedfailure.
- Reproduce:cmake -S tests/hdrvdp3_evaluation -B <fresh-build>
  -DIQM_HDRVDP3_DIMENSION_DIAGNOSTIC=ON;build IQM-spyr-dimension-test.
  Generate native dimensions withtests/generate_dimension_frontier.m,then
  IQM-spyr-dimension-test native-case ppd band-count fresh-output.
  Generate FFT fixtures withtests/generate_rectangular_fft_reference.m out
  validation/octave_plan_probe_639277410904713098. Compile eachshader innew
  directory via tests/hdrvdp3_fft256/compile_rectangular_inverse.ps1,with
  -Forward/-Base asneeded. Build fft256-inverse-test fromexisting isolated
  validation/fft256_inverse_639281620198383169/build;call native-case shader
  ref fresh-output --rectangular-inverse/--rectangular-forward/--rectangular-base.
- CMake configure initiallycouldnotdetect compiler insandbox;approved retry
  andMSBuild builds succeeded. No compile_shaders.sh,noreset/commit/push;
  staged shader,dirtyVkFFT,oldreferences andallunrelated changes preserved.
  Docs:tests/RECTANGULAR_COMPONENT_VALIDATION.md.

Exact next step:compose rectangular MTF/LA prepare-filter-finish using the
accepted two-axis shaders,actual R32 images andnative128x256/256x128 padding;
compare allstages andfilters before LocalAdaptFFT integration. Thenextend
MapGPU viaexplicitnewcontract for128-sample residual DC/base-adaptation,
8x16/16x8 first-max/frequency andrectangular resize/masking. Current ABI14
reduction normalization andselection layouts assumesquare64/256samples;
do notmerelyremoveguards. Continuous fullquality/publicrectangles remain
unvalidated. General oddFFT andnon-dyadic doublemaskcoefficients remainopen.

## Latest continuation:128 impulse/one-ULP stress and dimension frontier

- Added impulse and competing_axes transforms to generate_quality128_reference.m
  and reusable tests/run_quality128_stress.ps1. Rebuilt current isolated128
  diagnostic successfully before running it (approved MSBuild escalation).
- Fresh evidence:validation/quality128_stress_639282594471943896, pointer
  validation/quality128_stress_completed_path.txt. Two new input pairs pass
  all six native checks each,12 total,zero numeric mismatches. Impulse Q/JOD
  6.7128562927246094/7.6113557815551758;competing_axes
  0.98083019256591797/1.2951202392578125. Final score bits exact;no Vulkan errors.
  Scope128square,30ppd,age24,D65,single luminance. FFT256 column-major,
  inverse1/65536 once,MTF symmetric/LA replicate padding;residual DC mean
  removed andselection magnitude DC zero. Signed-zero stage bits unasserted.
- Strengthened validate_difference_maps.m:assert native/GPU first-max index,
  not only radial frequency. Both pass:index32 impulse,index33 competing axes
  (one-based). Native competing maxima5018.5791015625/5018.57861328125 differ
  by exactly one Float32 ULP. ranking_audit.json independently checks this.
- Fresh32/64 public regressions pass:96/107 numeric exports hash-identical
  to golden assets,native score bits exact;14 invalid-input cases pass.
  Evidence:checkpoint/bounded_regression. Executed existing
  validation/recheck_bounded_after_map14.ps1 -Root <that fresh directory>.
- New generate_dimension_frontier.m produced16 native SPYR band fixtures
  (eight rectangular/odd shapes at30/60ppd). Native geometry only,no GPU score
  claim. Residuals include8x16,16x8,8x9,9x8,10x12,12x10,16x17,17x16.
  Evidence:checkpoint/dimension_frontier/geometry.json andband exports.
- Reproduction: powershell -NoProfile -ExecutionPolicy Bypass -File
  tests/run_quality128_stress.ps1 -OutputDirectory <fresh> -DiagnosticExecutable
  validation/map128_api_639281996762386676/diagnostic_build/Release/IQM-quality128-diagnostic.exe
  -Assets validation/map128_api_639281996762386676/runtime128
  -SpectralDirectory validation/asset_generation_639281159480285385/assets_verified/spectral
  -SourceExport validation/continuation128_639282590525981729/full128_localized.
  Current runner additionally verifies shader pins andasserts one-ULP ranking;
  it was extended during this run. Separate POSTRUN_AUDIT.json checks those
  conditions;initial hashes andfinal_source_hashes retained. Numerical
  validator changes preceded their execution. No full suite rerun implied.
- Updated HDR128_INTEGRATION_VALIDATION.md. All prior staged/unstaged changes,
  references anddirty VkFFT preserved;no commit/push orblanket shader rebuild.

Exact next step:validate rectangular64x128/128x64 SPYR GPU bands against the
new dimension_frontier fixtures,then build explicit rectangular FFT128x256
andresidual8x16 contracts. Keep public guards unchanged until continuous
preprocessing/map/score parity passes. Arbitrary ratios also need a replacement
for exact-Q20 binary mask coefficients;do not silently approximate. Start with
power-of-two rectangles beforeodd/general sizes. The previous128 stress
next-step below is now complete within the measured scope.

## Latest continuation: recovered128 integration and fresh public replay

- The previous top entry was stale: existing local code already connects
  FFT256 graphs8/9+3/4, explicit MapGPU ABI14/Q20 masks, base FFT ABI82,
  256-sample reductions and ppd-limited SPYR height. Public evaluate_quality
  permits128-square at30ppd/age24 only. Bounded ABI13 remains separate.
- Recovered historical evidence at validation/map128_api_639281996762386676:
  three full128 native stage suites and public replays with107 identical
  numerical exports each. These are previous-session results, not new runs.
- Freshly rebuilt IQM-quality-api-test successfully, then ran localized128
  input in validation/continuation128_639282590525981729. All six native checks
  passed: strict preprocessing, SPYR, CSF, masking, difference and official
  quality. Zero numerical mismatches; Q=5.024019718170166,
  Q_JOD=5.937023639678955, pooled error=2.635451316833496; API score bits exact.
  All107 numerical exports hash-identical to the prior public replay.
  No Vulkan ERROR/Validation Error/VUID diagnostics. No stage signed-zero
  bit-identity claim. This is one replay, not an additional unique input pair.
- Fresh checkpoint has COMPLETED.json/PASS.txt, input/source/binary hashes,
  gpu.log, six native logs, stage_results and replay_comparison.json.
  Pointer: validation/continuation128_completed_path.txt. Single luminance,
  D65,128square,30ppd,age24; symmetric MTF/replicate LA padding to256square,
  once inverse1/65536; residual mean removal and magnitude-only selection DC.
- Build command: cmake --build
  validation/evaluation_api_639281145664415302/isolated_build --config Release
  --target IQM-quality-api-test. Initial sandbox FileTracker E_ACCESSDENIED;
  approved elevated retry completed before execution. Native command:
  powershell -NoProfile -ExecutionPolicy Bypass -File
  validation/validate_full128_stages.ps1 -Root <fresh checkpoint>
  -CaseName full128_localized. Octave exit warning accompanies successful0.
- Added tests/HDR128_INTEGRATION_VALIDATION.md and linked it from
  MASKING_QUALITY_VALIDATION.md. No numerical code changes this session.
  Existing staged/unstaged artifacts and dirty VkFFT preserved; no commit/push.

Exact next step: broaden128 full-stage input coverage with a fresh independent
native impulse/near-tie residual case using the pinned128 assets, then verify
32/64 public regressions before any further scope change. Keep128@60ppd and
other ages rejected until separately validated. General dimensions, RGB-native,
full-frame and other-device parity remain unresolved. Historical next-step
sections below describe older checkpoints and do not supersede this entry.

## Latest continuation:128 reduction/masking/resize components

- Isolated64/256-sample mean/DC andbase adaptation:512 cases each,34,304 /
 132,608 float outputs,zero mismatches. Final reduction shader replays pass.
 512-case GPU DC->FFT16->magnitude/first-max chain compares655,872 scalars
  exactly,including131,072 explicit imaginary-zero padding values. No CPU
  intermediate readback/substitution. Declared exponent/log domains retained.
- Unchanged accepted ABI13 map shader:128 masking16cases/524,288 floats,
  pupil30cases/120 trace values,quality-band16cases/16 scalars,Float32resize
 80transitions/479,744 floats allzero numeric mismatches. Notfullscores.
- Important failure/fix:128->8 binary mask resize rejects existingQ16 weights
  asnonexact. Failed gpu.log/partialgpu exports retained. Separate experimental
  ABI141 Q20coefficients/Q40conversion matches40 outputs/174,592 binary64
  values bit-for-bit. Shader has onlyShader capability,noFloat64/Int64.
  This customshader mustnotreplace ABI13 underits existingcontract.
- Accepted map/base16 reproduction hashes unchanged:
  map0B023987B9A2C9267B8979A072574DCC04AD8F56EA56CB1B7AACF164EA041BFF;
  base16 64F99AC7EF8242219A13402416549BF94FD7BED991AC02F2E3BF2C1EE264CEDC.
  AllGPU logs have noERROR/Validation Error/VUID diagnostics. Float equality
  doesnotassert signed-zero bitidentity;doublemaskresize doescompare bits.
- Evidence roots/pointers under validation:
  base_reductions_639281814095504643 (shader,chain_shader,gpu_chain,final_gpu*);
  mask128_639281816218927514;
  scalar128_639281817346881631;
  resize_mask128_639281818201889156 (acceptedgpu_q20/q20_shader;rejectedgpu);
  resize_float128_639281818909600345.
  Eachhas COMPLETED.json,source/native hashes andnative metadata/exports/logs.
  Docs:tests/HDR128_COMPONENT_VALIDATION.md. Reproduce via native generators,
  compile_base16.ps1 -Reductions,andfft256-filter-test nameddiagnostic modes.
- Tests remainisolated;production MapGPU/LocalAdaptFFT/evaluation guards still
 32/64 or64samples asbefore. Staged shader,VkFFT andunrelated changes preserved.

Exact next step:integrate256-sample DC/base adaptation and16x16 FFT/selection
through explicit newMapGPU shader ABI;retain ABI13 and8x8 paths unchanged.
Integrate Q20 maskresize underanexplicit contract,combine FFT2563/4+8/9
graphs forLocalAdaptFFT,andapply ppd-limited SPYR height/band-count metadata.
Then runfull native128 diagnostic stage/score comparisons beforelifting public
API guards. Component passes above arenot acompleted128 metric/API claim.

## Latest continuation:residual16 FFT andfirst-max selection

- Added source-backed r2cf_16/n1_16 ordinary graph shader,existing finite_hypot
  andcolumn-first strict-greater maximum scan. Original FFTW source notices,
  GPL andarchive/member/LF hashes preserved underfftw_reference/base16.
- Two GPU batch executions cover128 matrices:64 synthetic controls/random
  and64 official sp0Filters residuals from128x128 single inputs at30ppd.
  Native SPYR residuals are normalized byget_band andmean-subtracted before
  export. GPU mean/DC subtraction isnotpart ofthis test.
-65,536 FFT components,32,768 magnitude values and128 selected indices all
  match native single Octave exactly;98,432 total scalar comparisons,no
  tolerance. Zero/constant/tie/near-tie controls included. Signed-zero bit
  identity isnotasserted. Only selection magnitudeDC iszeroed;raw FFT retained.
- Native fixturethreads3/estimate,wisdom containsr2cf_16 andn1_16. Separate
  earlier extension plan probe used22threads;notincluded inthis profile.
  SPIR-V Shader/DenormPreserve/RoundingModeRTE only;noFloat64/Int64.
  Intel Arc Pro GPU logs show noERROR/Validation Error/VUID diagnostics.
- Evidence:validation/base16_639281811300602859/COMPLETED.json,source_hashes,
  binary_hashes,native_hashes,native/native_spyr wisdom andmetadata,gpu/gpu_spyr
  exports andlogs. Pointer:validation/base16_completed_path.txt.
  Reproduce:generate_base16_reference.m [spyr],compile_base16.ps1,then
  fft256-filter-test --base16 fixture shader fresh-export.
  Docs:tests/BASE16_VALIDATION.md. Existing8x8 shader pins/production guards
  unchanged. Prior staged shader/VkFFT andall unrelated changes preserved.
- Verified filter-chain work islocal commit2d8618e,forwardFFT localf9ce823,
  inverseFFT local4bc1522;remote previouslythrough11797c1. No new push.

Exact next step:validate GPU16x16 residual mean/DC andbase adaptation (existing
MapGPU paths require64 samples),thenroute validated FFT/selection for16x16.
Apply official ppd-limited height toboth evaluationSPYR calls andband-count
metadata;128@30ppd mustuseheight3/residual16,128@60ppd height4/residual8.
Then validate128 map resize/masking,pupil/quality reductions andfull-stage
native score regression. Integrate FFT256 dispatch andlift public guards only
after complete128 parity;the current API remains32/64. General dimensions,
RGB-native processing andfull-frame parity remain outside measured scope.

## Latest continuation:128x128 MTF/LA chains

- Added isolated fft256-filter-test, using actual R32 storage images and
  existing source-generated prepare/multiply/finish shaders with accepted
  forward256/inverse256 SPIR-V. Production/public API guards remain32/64.
-12 unique chain cases:asymmetric,impulse,HDR1e-3..1e4;ref/test;MTF/LA.
  Four complex stages andfinal cropped output compare6,488,064 GPU float
  components toofficial native Octave withzero numeric mismatches/no tolerance.
  CPU-generated MTF/Gaussian filters compare786,432 Float32 values exactly;
  this doesnotassert binary64 filter identity. Generated filters are uploaded.
- Four final-source driver replays pass separately,excluded fromunique counts.
  No Vulkan ERROR/Validation Error/VUID diagnostics. Signed-zero bits unasserted.
  MTF andLA take separate native inputs;this isnot acontinuous visual-pathway
  run orfull128 score validation. No CPU stage-output replacement orDC removal.
- Native128 fixtures use official3.07 functions,30ppd,single,paddedFFT256.
  Symmetric MTF /replicate LA post padding;inverse normalization1/65536.
  R32 input/output row-major;FFT column-major with explicit export conversion.
- Evidence:validation/fft256_filter_639281807624298950/COMPLETED.json,
  records.json,source_hashes.json,native_reference_hashes.json,driver_replay/
  provenance.json,numeric exports/logs. Pointer:fft256_filter_completed_path.txt.
  New native impulse/HDR fixtures preserved undercheckpoint. Reproduce via
  tests/hdrvdp3_fft256/validate_filter.ps1. See FFT256_FILTER_VALIDATION.md.
- Crucial native shape confirmation:pyramid_shape.json from actual official
  hdrvdp_spyr.decompose:128@30ppd height3,bands128/128/64/32/16,residual16;
 128@60ppd height4,bands128/128/64/32/16/8,residual8. Current bounded evaluation
  uses size-only maximum height;before128 integration itmustpass the official
  min(ceil(log2(ppd))-2,maxPyrHt) height toboth Spyr calls andband-count metadata.
  Existing32/64 validated height behavior isunchanged. Separate base16 plan
  probe selects r2cf_16/n1_16 andreports22threads;diagnostic only,notthe3thread
  FFT256 reference profile. Probe source/log preserved undercheckpoint.

Exact next step:validate16x16 residual FFT/first-max selection andppd-limited
pyramid height for128@30ppd,then128 map resize/masking/reductions. Integrate
FFT256 intoLocalAdaptFFT diagnostically onlyafterthe base/map paths are ready;
keep public API guards until complete full-stage native128 parity is measured.

## Latest continuation: isolated forward256 GPU graph

- Added source-backed real256 composite and complex256 forward generators.
  Axis0 uses r2cf_8 halfcomplex leaves, r2cf_32 DC, paired hc2cf_32 and
  r2cfII_32 middle; axis1 uses n2fv_16 leaves and t3fv_16 SIMD twiddles.
  Original FFTW sources and archive/member/LF hashes retained.
-18 full GPU runs match native single Octave fft2 in2,359,296 components,
  zero numeric mismatches. Four actual MTF/LA matrices plus14 real synthetic
  zero/impulse/random1e-3/1/1e3/subnormal1e-40/boundary1e-38 matrices.
  First-axis exports fromthe same18 runs match1,188,864 native FFTW components.
  These are extra comparisons,not18 extra GPU executions. No tolerance,
  normalization or DC removal; signed-zero bit identity is not asserted.
- Native axis-plan fingerprints/groups match the native full256 plan.
  Four CPU reconstructions also match existing Octave exports in524,288
  components. GPU input column-major real/interleaved;axis0 compact256x129;
  final GPU column-major,exported row-major. DenormPreserve32 andRTE32 required.
- Inverse regeneration retains accepted shader SHA256
  A07F14752C10E713EB85DA51F5AE4379D9DEF03CB74B5830077CDD27EA0A1CED.
  Final-source forward,subnormal inverse andnative-oracle smoke checks pass.
- Evidence:validation/forward256_axes_639281632814118746/COMPLETED.json,
  full_records.json,independent_axis_checks.json,full_shader,full_accepted_*,
  accepted_source_hashes.json,accepted_binary_hashes.json,final_source_smoke.json.
  Local drivers:validation/run_forward256_suite.ps1 andcheck_forward256_axes.ps1.
  Preliminary probes are excluded from18 counts. Interrupted PowerShell stderr
  logging and rejected oversized metadata serialization are kept separately.
- Docs:tests/FFT256_FORWARD_VALIDATION.md andtests/hdrvdp3_fft256/README.md.
  Compiler accepts -Forward;checker --forward or--forward-axis. Public API
  guards remain32/64;production FFT dispatch/asset pins unchanged. Protected
  staged shader anddirty VkFFT plusall unrelated experiments preserved.

Exact next step:connect isolated forward256 +inverse256 into a diagnostic
128x128 MTF/LA preparation/multiply/finish chain;compare allstages toOctave.
Then extend residual FFT beyond8x8 and128 map/resize/masking/reductions.
Do not lift public API guards until complete full-stage parity is measured.

## Latest continuation: inverse256 GPU graph

- User requested GitHub publication before continuing. Four existing commits
  through11797c1 were successfully pushed to origin/feature/hdr-vdp3;local and
  remote matched. New inverse work below is a separate local checkpoint.
- Added isolated tests/hdrvdp3_fft256 target,source-backed inverse generator,
  native synthetic fixtures andchecker. Native first axis is scalar16+n1_16 /
  t2_16;second is inverse8+n1_8 /SIMD t3bv_32. Source codelet descriptors supply
  twiddle powers1,3,9,15 and1,3,9,27. FFTW original sources,archive/member/LF
  hashes andGPL notices preserved. No public API/core shader integration change.
- Accepted preserve/RTE profile:18 fresh GPU executions compare2,359,296 float
  components withzero numeric mismatches. Includes actual MTF/LA spectra,zero,
  complex impulses,random scales1e-3/1/1e3,subnormal1e-40,boundary1e-38.
  Native single ifft2 is the full inverse oracle. Normalization occurs once
  atfinal axis1/65536;input column-major/interleaved,comparison explicitly
  converts final storage torow-major/interleaved.
- The same18 GPU first-axis exports independently compare2,359,296 components
  exactly against native FFTW many-plan t2_16/n1_16. These are extra CPU oracle
  checks onthe same GPU executions,not18 extra GPU runs. Numeric float equality
  is checked without tolerance;signed-zero bit identity is not asserted.
- Critical failure/fix:initial guessed twiddles failed130,048 components.
  Descriptor-derived tables fixed that. Default-control shader then passed14
  final inverse cases butfailed21,088 LA first-axis components,max1.5046e-36.
  Partial software-denormal candidates remained inexact/one failed compilation;
  rejected code is retained under validation,removed fromcommitted generator.
  Device reports preserve32=1/flush32=1/rte32=1. Explicit SPIR-V DenormPreserve32
  +RoundingModeRTE32 fixes all18 first-axis andfinal comparisons. Checker
  rejects devices lacking those properties. No Float64/Int64 capability used.
- Evidence:validation/fft256_inverse_639281620198383169/COMPLETED.json;pointer
  validation/fft256_inverse_completed_path.txt. Accepted files:preserve_rte,
  preserve_suite/records.json,axis0_preserve_results.json,accepted_source_hashes,
  accepted_binary_hashes. Failed/default profiles kept separately,including
  DEFAULT_CONTROLS_END_ONLY.json,ref.log,axis0_la_diagnose.log andsoftware dirs.
  Drivers:run_fft256_inverse_preserve_suite.ps1 and
  check_fft256_inverse_preserve_axes.ps1 under validation. Fresh dirs required.
- See tests/FFT256_INVERSE_VALIDATION.md andtests/hdrvdp3_fft256/README.md.
  The accepted seven32/64 reference shader pins remain unchanged. Staged test
  shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,dirty VkFFT,main wrapper /
  CMake experiments andall old reference files preserved. No GPU128 metric claim.

Exact next step:translate/validate the real256 forward first-axis composite
hc2cf_32/r2cf_32/r2cfII_32/r2cf_8;then complex forward main/tail graphs. Reuse
explicit float controls andnative fixtures;do not assume one graph serves both
axes. After full FFT256 integration,extend residual FFT beyond8x8 and128 map
resize/masking/reductions. Keep API guards at32/64 until full-stage parity.

## Latest continuation: source-to-reference-shader reproduction

- Committed32 scoped generator/source files in
  ffeed634246fca6530165c122cd065b8f65eaa54. Includes seven reference shader
  generation, FFTW runtime codelets with source/member/LF-hash provenance,
  AMD sources/hash manifest/licenses and required math helpers. No push.
  FFT128 generator gained a fresh-output option;FFT64/base use vendored
  hash-checked codelets by default,optional original archive still supported.
- Fresh clean worktree under validation/asset_generation_639281159480285385/
  checkout and clean VkFFT. API built from committed sources;all seven
  reference shaders generated without old archive/binaries/downloads. All7
  SHA256s equal accepted prior binaries;seven embedded headers also equal.
- Fresh direct32/64 API runs with generated shaders preserve score bits and
  203 numerical text export hashes (96/107),plus14 negative cases.16 poisoned
  environment variables ignored;checkout/submodule remain clean. These compare
  prior native-validated baselines,not another fresh Octave/GPU checker run.
- Added default hash-pin verification,license copying and compiler metadata
  to generation;fresh final_manifest_check reproduces all7 binaries unchanged.
  Tampered FFTW source andexisting output directory guards pass,old binary
  preserved. Spectral CSVs andPFMs remain explicit external input data.
- Evidence:validation/asset_generation_639281159480285385/COMPLETED.json;
  pointer validation/asset_generation_completed_path.txt. Includesclean source,
  build,generation/run logs,comparison.json,assets_verified/manifest.json,
  generator_negative_checks.json andfinal_manifest_check. Local drivers:
  validation/validate_generated_api.ps1,check_asset_generator_guards.ps1.
  Initial premature API launch did not execute;initial_launch_note.txt preserves
  this. Verified retry uses assets_verified;all old artifacts retained.
- See tests/REFERENCE_ASSET_VALIDATION.md. Existing staged shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved. New commits
  are local;origin remains6b18daa. Main wrapper/root CMake remain mixed.

Next implementation area:general dimensions. Start from the actual native
256-point FFT plans before translating larger graphs;do not replace the native
graph with an arbitrary mathematically equivalent FFT. Existing bounded API
guards remain until new FFT,base-residual andresize/masking stages pass.

### Native256 frontier started

- Extended only diagnostic tests/generate_fft_size_reference.m to128-pixel
  inputs andtests/inspect_fftw_plan.cpp to256-point FFT. Public API/GPU guards
  remain unchanged. Fresh asymmetric ref/test native MTF/LA fixture generated;
  staged MTF matches official fast_conv_fft exactly at30ppd,single inputs,
  double filters,symmetric post padding,inverse normalization1/65536.
-16 native DLL/Octave plan comparisons:all8 unaligned forward/inverse cases
  (threads1/3,ref/test) match1,048,576 float components exactly.8 aligned
  alternatives differ;they remain rejected evidence,not tolerance passes.
- Native estimate planner uses3 threads. Observed real256 graph is composite
  hc2cf_32/r2cf_32/r2cfII_32/r2cf_8;complex forward includes
  t3fv_16_avx2_128/n2fv_16_avx2_128,and inverse t3bv_32_avx2_128/n1_8.
  Changing a shader size constant or choosing an arbitrary scalar256 FFT
  would not reproduce these native operation orders.
- Evidence:validation/fft256_frontier_639281615622631080/COMPLETED.json;
  pointer validation/fft256_frontier_completed_path.txt. Includes actual
  wisdom,plan trees,all candidate spectra,comparison/build/run logs andsource
  hashes. See tests/FFT256_FRONTIER.md. No GPU128 metric parity claim.

Exact next step:translate the observed native256 real/complex/inverse graphs
as an isolated shader experiment andcompare intermediate results against these
fixtures. Then extend base residual FFT beyond8x8 and128 map reductions/resize;
only widen API guards after isolated andfull-stage tests pass. Existing32/64
regressions andoriginal source/reference assets must remain preserved.

## Latest continuation: clean committed-source build

- Scoped18 API/runtime/build files committed as
  `7c7f4736c324ce2d648151b6f21003f519132f95`,English message
  `Expose bounded luminance quality API with isolated shader build`. This
  promotes the previously verified bounded runtime,not the mixed main wrapper
  or root/source CMake integration. No push;origin still at6b18daa.
- Detached clean worktree at
  validation/clean_api_639281156569491989/checkout. Its VkFFT is clean at
 539be29f30ef40384f3ccd24d7884166f82cbef8,cloned from local Git objects using
  a per-command submodule URL override;no dirty files or configuration changes
  were copied from the original submodule. Original dirty VkFFT is preserved.
- Fresh independent configure/build passes from committed sources,using
  tests/hdrvdp3_evaluation,with build and exports outside the checkout. Seven
  embedded shader headers regenerated from GLSL match prior headers' hashes.
  No root build cache,workspace shaders_out,wrapper,other metrics or dependency
  downloads. Clean checkout and submodule remain clean after all tests.
- Two fresh direct32/64 photo-localized API calls return exact error/Q/Q_JOD
  bits and203 matching numerical text export hashes (96/107),despite16 poisoned
  environment variables. Fourteen negative cases pass. This compares against
  prior native-validated baselines;it is not another fresh Octave/GPU checker
  execution. Scope remains30ppd/age24/D65 positive square luminance.
- Evidence:validation/clean_api_639281156569491989/COMPLETED.json;pointer
  validation/clean_api_completed_path.txt. Includes checkout,build,source and
  binary hashes,configure/build/run/negative logs andassets/manifest.json.
  Local driver:validation/validate_clean_api.ps1;use a fresh work path on rerun.
  Fourteen runtime assets are copied and hashed inassets (FFT2,map/base2,
  math3,spectral3,PFM4);golden files are comparison evidence. Original reference
  files and earlier failed/successful artifacts are preserved.
- Remaining distinction:external reference FFT/math/map/baseFFT assets were
  NOT regenerated from clean committed generator inputs. The clean checkout
  build with pinned assets is verified;the full self-contained source-to-assets
  package is not. See tests/CLEAN_CHECKOUT_VALIDATION.md for exact scope.
- Preserve staged shader index blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7.
  Root/source CMake,wrapper,other existing experiments remain local and mixed.

Exact next step:package external reference shader generation and its source
dependencies/provenance,then reproduce the14 runtime assets from a fresh
location andcompare hashes/numerical results. Audit FFTW graph source/archive,
reference math source manifests andpower/map/base FFT generators before
promoting them. Do not reclassify copied binaries as regenerated assets.
General dimensions,RGB-native encodings andfull frames follow separately.

## Latest continuation: library quality API

- Added `include/IQM/hdrvdp3/evaluation.h` and
  `src/hdrvdp3/evaluation.cpp`, compiled into `IQM-HDRVDP3` with strict FP.
  `evaluate_quality(instance,reference,test,options,trace)` accepts two
  `LuminanceImage` objects and explicit viewing/age, FFT/math/map/baseFFT
  assets and raw spectral directory or sampled data. Returns `QualityScores`
  plus per-band errors. No environment reads or synthetic generation in the
  evaluator. Native arithmetic, padding/layout/DC processing and shaders are
  unchanged. Only validated square32/64 positive luminance inputs are accepted.
- Wrapper now prepares inputs/configuration and calls the library. The old
  preliminary non-FFT/CPU quality modes are rejected explicitly; isolated
  corrDn/SPYR probes remain. Historical constant-image comparison code was
  removed from this path; its source snapshot/reference files are preserved.
- New independent target `IQM-quality-api-test` links the library without the
  wrapper. With16 poisoned legacy environment variables, direct32/64 calls
  return bitwise exact error/Q/Q_JOD and4/5 band errors.203 API-generated
  numerical text exports match baseline hashes. Fourteen invalid shape/count,
  luminance, viewing/age/FFT/map and trace-directory cases are rejected before
  dispatch/overwrite. Diagnostic export to a fresh explicit directory remains
  required by this first API; the shared VulkanInstance must not be concurrent.
- Ten fresh full regressions pass:6 photo luminance pairs +2 synthetic controls
  at30ppd/age24, and2 HDR ramp localized-stronger at60ppd/age60.001, both32/64.
  Official Q/Q_JOD, CPU JND, raw/centered SPYR, CSF/masking/D and independent GPU
  stages pass.1,425,505 floats and59,520 binary64 masks compare with zero
  differences;1,218 full-suite text export hashes remain identical.
- Root: `validation/evaluation_api_639281145664415302/COMPLETED.json`;
  pointer `validation/evaluation_api_completed_path.txt`. Contains before
  snapshots, source/binary hashes, per-case suite paths/logs, direct/extended
  results and14-rejection log. Drivers in validation:run_evaluation_api_regression,
  run_evaluation_api_direct,run_evaluation_api_extended and
  finalize_evaluation_api_checkpoint.ps1. Use fresh work/export paths on rerun.
  Preserve initial failed build (missing transpose helper) and initial direct
  comparison attempt:the latter wrongly expected Octave-added files in API
  output, although the initial direct32 returned scores were exact. Corrected
  direct attempts use fresh directories and compare all actual API files.
- Follow-up CLI error propagation:wrapper rethrows full-quality exceptions;
  main already converts them to nonzero exit. A32-PFM/64-selected rejection
  returns-1 (`cli_reject_dimension.json`). The numerical API checkpoint keeps
  its pre-CLI-change executable/source snapshot; CLI status verification is a
  subsequent build, recorded separately in `CLI_COMPLETED.json`. One additional
  fresh32/asymmetric-localized full strict success regression passes on that
  build:57,046 floats and2,368 double masks exact,116 unchanged text exports.
  The final executable is `IQM_cli_status.exe`,with separate source/hash files.
- Added independent `tests/hdrvdp3_evaluation/CMakeLists.txt` and README.
  Fresh configure/build passes without the root build cache, other metrics,
  FetchContent or workspace shaders_out. Generates exactly seven embedded
  headers from current GLSL into isolated_build/generated;all seven SHA256s
  equal the previously verified embedded headers. Uses SDK discovery rather
  than the main build's hardcoded SDK library paths. Independent32/64 direct
  calls then pass bitwise scores and203 numerical export hashes with poisoned
  environment. Evidence:ISOLATED_BUILD.json,isolated_source_inventory_hashes.json,
  isolated_shader_header_hashes.json,isolated_binary_hash.json and
  isolated_direct_results.json under the API root. This builds the current
  source snapshot,not a clean committed checkout. VkFFT tracked header changes
  are empty;its existing dirty changes affect CMakeLists.txt and an untracked
  glslang directory. The isolated project does not use that modified CMake.
- Current runtime/API integration remains local/uncommitted with existing
  mixed changes. Do not blindly commit whole CMake/wrapper/shader files.
  Published HEAD remains `6b18daa`;earlier11 verified commits were successfully
  pushed, origin and HEAD matched at that checkpoint. Staged shader blob
  `1cf7baedb1d54be0ca8ad02d14485e510b0434e7`,dirty VkFFT and old artifacts preserved.

Exact next step:review and promote the minimal runtime dependency set used by
the new isolated target into a scoped commit,then validate from a fresh clean
checkout with complete external reference shader generation/assets. The
independent build already regenerates all embedded headers;reference FFT,
math,map/baseFFT assets still arrive as explicit pinned runtime inputs.
Resolve shared bin Vulkan resource ownership as needed for packaging.
Then generalize image/FFT dimensions and validate additional official RGB/HDR
encodings and full frames. Those remain unfinished; a measured overall
completion percentage is not available. Do not label square32/64 luminance
validation as full HDR-VDP-3 input-domain parity or a fully GPU-only pipeline.

## GitHub publication checkpoint

- User explicitly requested publication on 2026-10-20. Fetched
  `origin/feature/hdr-vdp3`; the remote has no divergent commits. Prepared
  the ten verified local commits through `d03c2c3`, plus this handoff update,
  for a normal push to `origin/feature/hdr-vdp3` (no force push).
- Publication scope: scalar numerical corrections, spectral calibration/raw
  loading, lossless luminance IO, isolated validation tools and documented
  numerical checkpoints. Commit messages and technical documents are English.
- Full runtime integration, including the validated FFT config boundary,
  remains local with mixed existing changes. The remote branch must not be
  described as a complete standalone implementation of the validated local
  runtime. Existing untracked artifacts and dirty `lib/VkFFT` are preserved;
  the staged corrdn_test shader is excluded. Next implementation step remains
  the reusable quality API described below.

## Latest continuation: explicit FFT configuration boundary

- Committed the independently verified luminance input module, standalone
  tools and validation documents as `d03c2c3931c616b3f38399de7bf109bff392a59a`
  (`Add lossless luminance file input and validate real image crops`). No push.
  Mixed main application integration was excluded; staged shader preserved.
- Added `HDRVDP3FFTConfig` and an explicit `compute_fft_filter_gpu` overload
  in the local wrapper. Axis order, twiddle mode, reference shader path and
  math shader directory are passed explicitly to FFT execution. The test
  harness parses its environment once for the four MTF/adaptation calls.
  The old overload remains a compatibility adapter. Arithmetic, shaders,
  padding, layouts and DC processing are unchanged. This is the first
  extraction boundary, not a complete public quality API.
- Release build passed. Eight fresh full strict native-reference regressions
  passed: six photo-derived luminance pairs and two synthetic controls at
  32/64,30ppd,age24,D65/raw spectral input. GPU checks cover 1,140,404 float
  comparisons and 47,616 binary64 mask values with zero differences.
  Before/after hashes match for 976 text exports (116 per32 run,128 per64
  run; three asset-path metadata filenames excluded from the comparison).
  Full raw/centered SPYR checks, native JND traces and official Q/Q_JOD pass.
- Evidence: `validation/fft_explicit_config_639281136637103838/COMPLETED.json`;
  pointer `validation/fft_explicit_config_completed_path.txt`. Preserves old
  wrapper/header/executable, build log, per-case logs, authoritative suite
  paths from completed logs, verified executable/source hashes, and isolated
  continuation patches against the before snapshots. Driver:
  `powershell -NoProfile -ExecutionPolicy Bypass -File
  validation/run_fft_explicit_config_regression.ps1`. Before rerunning, create
  a fresh timestamped work directory with before snapshots and update
  `validation/fft_explicit_config_work_path.txt`; preserve existing logs.
- Wrapper/header changes remain local with pre-existing mixed integration;
  do not commit their entire contents indiscriminately. The staged shader
  index blob remains `1cf7baedb1d54be0ca8ad02d14485e510b0434e7`.

Exact next step: continue extracting the validated evaluation into a reusable
quality API accepting two `LuminanceImage` objects, explicit viewing/age,
spectral data and shader configuration, returning `QualityScores`. Move
remaining environment parsing and synthetic generation into the harness;
use the explicit FFT overload internally. Preserve the same stage arithmetic
and repeat the six file/two synthetic before/after comparisons. General image
sizes, full original frames, zero-input policy and RGB-native parity remain
unfinished; no measured overall completion percentage is available.

## Latest continuation: file-based absolute luminance input

- New standalone input API:include/IQM/hdrvdp3/luminance.h and
  src/hdrvdp3/luminance.cpp,load_luminance_pfm. Grayscale Pf,three-line
  header,both byte orders,bottom-up raster ->top-down row-major float32;
  header scale magnitude applied once. Requires positive finite luminance.
  No IO resize,gamma normalization or tone mapping. RGB/zero/generalformats
  remain outside this contract. Existing8-bit shared reader is unchanged.
- Independent tests/hdrvdp3_input:25 cases (18 valid,7 rejected),30,810
  float bit values exact;CTest passes. Nonsquare3x5 IO tested separately from
  fixed32/64 full-pipeline support. Photo-content files add15,360 bit values.
- Validation-only prepare_hdr_luminance uses bundled stb float HDR decoding,
  explicit binary32 Rec709 numeric Y andone cd/m2 perderivedY unit assumption.
  Source primaries/photometric calibration are not independently verified.
  External source C:/HDR-VDP-3_Ref/examples/nancy_church.hdr is768x1024.
  Center crops32@(368,496),64@(352,480);no resizing. This is shared luminance
  input validation,not full RGB-native metric parity or whole-frame validation.
- Full wrapper accepts IQM_LUMINANCE_REFERENCE / IQM_LUMINANCE_TEST;both required
  andmust match selected32/64 shape. Synthetic generation/transforms bypassed
  in file mode;input_source.txt records routing. Runner options
  -LuminanceReference/-LuminanceTest hash inputs,use _file prefixes andclear
  stale file flags for synthetic runs. Separate native PFM decoder proves
  30,720 input/export float bits identical across six photo pairs.
- Six photo pairs at30ppd/age24/D65/raw spectral loading:localized1.5 patch,
  global1.05 brighter andidentical at32/64. All official scores,strict stages,
  CPU JND traces,raw/centered SPYR andGPU intermediates match. Photo-only:
  855,303 float comparisons,35,712 binary64 masks,zero differences.
  Two synthetic regressions keep prior scores exact andprove stale flags
  clear. Combined8 fresh runs:1,140,404 floats,47,616 double mask words exact.
- Photo Q/Q_JOD:32localized8.0709753036499023/8.7933588027954102;
  64localized9.2685623168945312/9.6516733169555664;32brighter
  9.9970340728759766/9.999699592590332;64brighter9.9958648681640625/
  9.9995403289794922;both identical10/10. Source crop ranges are recorded
  precisely inmetadata andtests/LUMINANCE_INPUT_VALIDATION.md.
- Expected dimension mismatch is rejected without resize. Legacy wrapper can
  returnexit0 after printing its error;runner correctly treats that as failure.
  rejected_dimension_suite.txt preserves the expected rejection artifact.
- Root:validation/luminance_input_639280968498275868/COMPLETED.json;pointer:
  validation/luminance_input_completed_path.txt. Includes before/source,binaries,
  fixtures,nancy metadata/derivedfiles,all logs,loader/CTest results andnegative
  test. Local finalizer:validation/finalize_luminance_input_checkpoint.ps1.
  No shader changes inthis continuation;corrected ABI13 hash remains0B023987...
- Important export-path race:negative test replaced the shared64 _file latest
  pointer while64-identical wasinflight. Provisional parent suite metadata
  was savedas suites64_from_mutable_latest.json. Canonical validated_suites.json
  parses the unique completed Suite PASS lines;corrected suites64.json now
  agrees. latest_pointer_audit.json records this. No numerical result changed;
  never use mutable global latest pointers as evidence under shared prefixes.
- Preserve staged corrdn_test shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  dirty lib/VkFFT andall pre-existing changes. Scoped input module/tools/docs
  may be committed;main pipeline integration remains local. No push.

Exact next step:extract the validated evaluation from hdrvdp3_test_run into
a reusable quality API accepting a LuminanceImage pair,explicit viewing/age
parameters andasset configuration,without global environment dependence.
Keep the harness thin andcompare before/after against these six file pairs
andtwo synthetic controls before changing arithmetic. Full original frames,
RGB-native/other encodings,zero-input handling andgeneral dimensions remain
unfinished. No measured overall completion percentage exists.

## Previous continuation: raw spectral loading and corrected power rounding

-2026-10-20:raw C++ D65/cone/rod table loader andnonuniform pchip are complete.
  420 native wavelengths,2,520 sampled double values exact;feeding those into
  calibration preserves16,822 double trace values across13 ages. Independent
  scalar CTests spectral_age_double andspectral_loading_double pass2/2.
  New source src/hdrvdp3/spectral_loader.cpp;header/API andstandalone test target
  updated. Raw original data files remain external,unmodified inputs.
- Raw evidence:validation/raw_spectral_639280955944105771/COMPLETED.json;
  pointer validation/raw_spectral_completed_path.txt. COMPONENT_COMPLETED.json
  retains the isolated checkpoint. Two full raw-input runs at32/64,60ppd,
  age60.001,HDR ramp/stronger localized match official scores andall stages:
  285,101 GPU floats and11,904 double mask words,zero differences.218 common
  text export SHA256 hashes equal corrected resampled-input controls (103/115).
- Harness now accepts IQM_SPECTRAL_DATA / -SpectralDataDirectory for the three
  original files;raw andresampled inputs are mutually exclusive. Exports
  spectral_source.txt='raw_d65';runner hashes all three source tables.
  The tested runtime path no longer needs an Octave-produced sampled file.
- Four cross-domain60ppd/new-age HDR ramp stress pairs pass at32/64 andages
  60/60.001:570,202 floats and23,808 binary64 mask words exact. Evidence:
  validation/age_ppd60_639280954526250977/COMPLETED.json;pointer
  validation/age_ppd60_completed_path.txt. Original application binary is
  preserved in its verified_bin;raw-loading build is a later executable.
- Important genuine failure retained:first64/60ppd/age60 HDR ramp failed one
  final D pixel by1 ULP (5.5511151231257827e-17),raw D/earlier stages exact.
  abs(rawD)=0x30473d44,pf=0x3f7e96d5;old power0x305fbd82,native0x305fbd83.
  initial_failed64_age60_suite.txt pins failure;do not label it a pass.
- Higher polynomial degree was rejected after breaking4 random native cases.
  Accepted fix retains native-style cubic log/exp anduses triple-float
  expansion only near ambiguous normal-output rounding. No per-pixel/input
  lookup or tolerance. Generator emits additional table/constant residuals;
  helper tests/math_reference/power_triple_reference.glsl is currently local.
- Accepted power probes:65,536 random +65,536 boundary +65,536 neighboring
  counterexample pairs exact. Old shader fails exactly the neighborhood center.
  Ten old full-age exports replay on the corrected map shader with zero
  GPU differences (1,254,496 floats,52,352 double masks);this is replay,not
  ten fresh full application runs. All accepted/rejected artifacts preserved.
- Corrected map ABI13 SHA256:
  0B023987B9A2C9267B8979A072574DCC04AD8F56EA56CB1B7AACF164EA041BFF.
  Shader-only SPIR-V capability;no Float64/Int64. Original ABI13 SHA andABI12
  artifacts remain historical. Do not substitute hashes across checkpoints.
- Commands:finalize_full_age_checkpoint.ps1 -CrossPPD and
  finalize_raw_spectral_checkpoint.ps1 under validation/. Raw root stores
  full32_path/full64_path,verified_bin,build logs andnative oracle comparisons.
  See tests/RAW_SPECTRAL_VALIDATION.md andtests/FULL_AGE_VALIDATION.md.
- Mixed pipeline/GPU/power integration remains local/uncommitted;scoped raw
  loader implementation andverified tools/docs may be committed. Preserve
  existing staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,dirty
  lib/VkFFT,all old references anddirty/untracked files. No push performed.

The explicit32/64 float luminance input next step is completed above.
Normal public metric API,full RGB/color andgeneral dimensions remain open.

## Previous continuation: full age pipeline ABI13

- Calibrated L/M/R coefficients now reach CPU JND,GPU adaptation_input and
  GPU JND;observer age also reaches pupil,band CSF LUTs andbase CSF/noise.
  Native validators/checkers read pupil.txt age and spectral_lmr.txt instead
  of assuming24. Runner adds -ObserverAge and -SpectralSamples,with age-specific
  prefixes and input hashing. Accepted harness ages are24,60,60.001.
- Ten full pairs at30ppd,32/64,D65 synthetic luminance pass native Q/Q_JOD,
  strict preprocessing,CPU JND traces,raw/centered SPYR and GPU intermediates:
  1,254,496 float values,52,352 binary64 mask words,zero differences.
  Includes localized cases atall3 ages/bothsizes,stronger32 at60/60.001,
  andHDR edge phase shifts at60.001/bothsizes. Scope is pinned native
  resampled spectra,not independently loaded raw spectral data.
- Age24 regression:216 common text-export SHA256 hashes equal the previous
  accepted binaries (102 at32,114 at64). Detailed scores/equivalence and
  remaining work:tests/FULL_AGE_VALIDATION.md.
- Root:validation/full_age_639280300414897968/COMPLETED.json;pointer:
  validation/full_age_completed_path.txt. Finalizer:
  validation/finalize_full_age_checkpoint.ps1. Contains before/source snapshots,
  verified_bin executables,suite JSON,all cases/logs,age_source_hashes.json,
  age24_text_regression.json and fresh shader source/binary. Preserve these.
- Map shader ABI13 SHA256:
  871E3916717BFFCC49B4E59AD9E52EB311A148E277D99F32DCEAA8FE1C2FC0FE.
  Adaptation mode receives two appended coefficient words;JND's existing
  coefficient payload is parameterized. New host rejects oldABI12 shader.
  Base FFT ABI81 unchanged;SPIR-V valid with Shader capability only.
  Old ABI12 shader/executables remain saved;no blanket shader rebuild.
- Subsequent cross-domain60ppd/new-age tests andraw loading are completed
  above;the original10-case30ppd checkpoint retains its earlier shader hash.
- Mixed pipeline integration remains local/uncommitted. Preserve staged shader
  blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,dirty lib/VkFFT and all other
  changes. Scoped verified tools/docs can be committed;no push.

The cross-domain/raw-resampling next steps are completed above. Real HDR/color,
arbitrary full dimensions andpublic API remain unfinished.

## Previous continuation: exact age-dependent spectral calibration

- New independent C++ component:include/IQM/hdrvdp3/spectral.h and
  src/hdrvdp3/spectral.cpp. Input is native-resampled double wavelength,
  D65 emission and LMSR sensitivity. Computes optical-density nodes,pchip,
  transmission,filtered emission,trapz integrals and normalized LMSR matrix.
  Not connected to the main metric yet;no production defaults/shaders changed.
- All six stages match native v3.0.7 numerically in double across13 ages
  (15,19.999,20,24,28,32,45,60,60.001,75,83,83.001,100):16,822 compared
  values,zero differences. Includes the age60 lens branch and age32 zero
  correction. Age24 L/M/R coefficients equal the old constants exactly.
  Strict non-FMA standalone target spectral_age_test passes CTest.
- Evidence:validation/spectral_age_639280297796803042/COMPLETED.json;
  pointer validation/spectral_age_completed_path.txt. hashes.json pins code,
  native visual/loader/data files,reference samples and final executable.
  Initial/final stage logs,ctest_final.log and official_integration.log kept.
- Official loader actually emits420 wavelengths over360..780,not421 at1nm.
  Preserve the exact linspace grid. Raw spectral file interpolation has NOT
  been ported;new API consumes the official resampled fixture. Do not claim
  independent raw spectral loading or full observer-age metric parity.
- Native-only official hdrvdp3 quality calls at13 ages check26 printed LMSR
  matrices at15 decimal places and104 single coefficients;identical-image
  Q/Q_JOD are10. This is an oracle consistency check,not C++ full-age parity
  or full-double validation. The separate16822-value trace is the exact proof.
- Tools:generate_spectral_age_reference.m,verify_spectral_age.cpp,
  validate_spectral_age_official.m;independent tests/hdrvdp3_scalar target.
  Configure IQM_HDRVDP3_SPECTRAL_FIXTURE to the absolute fresh fixture path,
  build spectral_age_test and run CTest -R '^spectral_age_double$'.
  Details and source attribution:tests/SPECTRAL_AGE_VALIDATION.md.
- Preserve staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  dirty lib/VkFFT and all mixed changes. Only verified new component/tools/docs
  are eligible for this scoped commit;no push and no blanket shader rebuild.

Exact next step:pass calibrated L/M/R coefficients into CPU JND and GPU
adaptation_input/JND operations (currently fixed age24);retain exact default
regression. Propagate age into pupil and CSF/AESL plus validators,then run
fresh full metric cases at24,60,60.001 before broadening. A pinned native
resampled spectral fixture can bootstrap integration,but raw spectral data
loading/resampling remains an explicit separate task. Use fresh shader/export
directories and preserve ABI12 and all prior binaries if shader inputs change.

## Previous continuation: ordinary HDR60 and isolated age components

- Eight fresh ordinary HDR60 pairs are complete at32/64: ramp localized,
  edges localized, ramp brighter and edges stronger. All strict native
  stages, raw/centered SPYR, official Q/Q_JOD and GPU intermediate comparisons
  are exact. New comparisons:1,140,404 float values and47,616 binary64 mask
  words, zero differences. Eight actual exported input geometry checks pass.
  Root:validation/ppd60_hdr_standard_639280292107529786/COMPLETED.json;
  pointer:validation/ppd60_hdr_standard_completed_path.txt. Run the local
  validation/finalize_ppd60_checkpoint.ps1 -HDRStandard to recheck.
- All three viewing checkpoints now cover22 fresh pairs (20 at60ppd,two
  controls at30ppd),2,623,084 floats and109,440 double mask words. This is
  distinct from the older stress32 checkpoint. Do not count reruns as unique
  cases. tests/PPD60_VALIDATION.md and portable JSON retain separate records.
- Application and original checker hashes match the prior verified viewing
  checkpoint throughout HDR runs; neither executable was rebuilt. New pupil
  checker was built separately under the new age root. No shaders changed.
- Isolated pupil ages:0,15,19.999,20,24,28,45,60,60.001,75,83,83.001,100;
  sizes32/64,ppd30/60,five constant/ramp/random adaptation maps. Official
  geomean/pupil_d_unified vs GPU:260 cases,1,040 single trace values exact.
  Out-of-range ages exercise the pupil20..83 clamp,not full observer support.
- Scalar CSF/AESL across the same13 ages,on/off,12 frequencies,16 luminances:
  4,992 double-luminance plus4,992 single-luminance queries;both compared in
  double,all9,984 exact. Default24/45/70 generator regressions also pass
  1,152+1,152 double comparisons;these repeats are not new coverage.
- Age evidence:validation/pupil_age_639280292892783820/COMPLETED.json;
  pointer:validation/pupil_age_completed_path.txt. Saved source snapshots,
  final_hashes.json,build/configure logs,reference metadata and GPU/CSF logs
  pin the implementation and native sources. Accepted CSF fixture is
  csf_precise_age;first csf fixture retained,not counted twice. The generator
  now accepts optional comma-separated ages and exports ages at17 digits.
  Default historical age list remains unchanged. New generator is
  tests/generate_pupil_age_reference.m;local GPU checker adds --pupil-age.
- Read tests/OBSERVER_AGE_VALIDATION.md before extending full age support.
  Native quality enables do_aod/do_aesl/do_slum and disables do_aslw. The
  lens-density formula branches at60,then pchip/transmission/trapz/normalizing
  produces an age-dependent LMSR matrix. Existing C++ single JND and GPU
  shader still hardcode age24 D65 coefficients. CSF defaults also remain24.
  Full metric observer-age parity is NOT established by these component tests.
- Preserve existing mixed changes,dirty lib/VkFFT and staged shader blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7. Scoped commits cover only verified
  reference tools/docs;main integration and GPU checker remain local. No push.

The spectral calibration next step from this checkpoint is completed above
for native-resampled inputs. Full age integration,raw spectral loading,real
HDR/color,public API and arbitrary full-pipeline dimensions remain unfinished.

## Previous continuation: 32x32 stress/HDR and 60 ppd

This section supersedes older remaining-work statements below. Historical
measurements and failures are retained; do not treat old tolerance passes as
exact parity or combine repeated runs into unique case coverage.

- The 32x32 stress/HDR checkpoint is complete:
  `validation/stress32_639280137550795829/COMPLETED.json` (selected by
  `validation/stress32_completed_path.txt`). Seventeen fresh pairs cover six
  stress32, four HDR32, two HDR stress32, one corrected impulse32 and four
  HDR64 controls. All strict native stages, raw/centered SPYR and official
  scores match. Independent GPU comparisons: 1,653,818 floats and 68,928
  binary64 mask words, zero differences.
- Shape-aware test input generation fixes HDR ramp normalization, eight-level
  HDR edge tiling and off-center/right-edge impulse coordinates at 32x32.
  Eleven exported input geometry checks pass; eight 64x64 HDR luminance file
  hashes equal the earlier accepted controls. Canonical32 coverage is now
  18 labels across baseline/shape-aware binary groups, not one 18-case run.
  The obsolete one-impulse32 case is replaced. See tests/FFT32_VALIDATION.md.
- The new full-metric parameter path accepts IQM_PIX_PER_DEGREE=30/60;
  runner option -PixelsPerDegree sets it. It updates MTF, local adaptation,
  CSF and physical residual frequencies, and visual area=width*height/ppd^2.
  pixels_per_degree.txt is exported; the shared helper defaults old exports
  to30. Age24/D65 remain fixed. Size caps still produce four/five bands.
  60ppd requires FFT preprocessing and Vulkan maps; other values are rejected.
- Evidence root: `validation/ppd60_639280267907904462`, selected by
  validation/ppd60_work_path.txt. suite_paths.json records four localized
  size32/64 x ppd30/60 controls plus the six-case stress32/60ppd suite.
  PASS.txt and COMPLETED.json were issued by
  validation/finalize_ppd60_checkpoint.ps1: ten pairs, 912,478 float values,
  38,016 double mask words, zero differences. Both 30ppd controls retain
  the earlier scores exactly. tests/PPD60_VALIDATION.md records parameters,
  scores, layout, precision and DC equivalence.
- Commands: build IQM and the separate checker32_build target successfully,
  then tests/run_quality_validation.ps1 -InputSize 32/64 -PixelsPerDegree
  30/60 -CaseName asymmetric_localized, plus -Stress at32/60 (no CaseName).
  Use the same pinned math/map/base shaders as the earlier32 checkpoint and
  fft32_native_639280099461894780/shader64/fft64.spv for32. For64 use
  validation/fft_forward2d_initial.spv. Full commands and output logs remain
  in the root; saved candidate executables/source snapshots/hashes preserve
  this build. Do not rerun older binary-hash finalizers against overwritten
  executable paths; stress32 finalizer already maps to its saved binaries.
- Tests/summarize_quality_stress.m now records actual dimensions, band count,
  ppd and actual mask denominator. tests/validate_input_geometry.m independently
  checks HDR endpoints/levels and three impulse positions. These are reference
  tools; mixed application/GPU integration still remains local/uncommitted.
- Additional 60ppd HDR stress checkpoint is complete:
  validation/ppd60_hdr_639280272206510764/COMPLETED.json, selected by
  validation/ppd60_hdr_completed_path.txt. HDR edges/phase shift and HDR
  ramp/stronger localized distortion pass at both sizes (four pairs), adding
  570,202 exact float comparisons and 23,808 bitwise double mask comparisons.
  Four exported input geometry checks pass. Combined viewing checkpoints:
  14 fresh pairs, 1,482,680 floats, 61,824 double mask words, zero differences.
  Run validation/finalize_ppd60_checkpoint.ps1 -HDRCheckpoint to recheck.
  Actual scores, selected physical frequencies and commands are recorded in
  per-suite logs, all_cases.json and tests/PPD60_VALIDATION.md.
- Preserve staged corrdn_test shader index blob
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7 and dirty lib/VkFFT. No reset,
  clean, blanket shader rebuild or push. Only scoped verified tools/docs
  should be committed; earlier user authorization permits those commits.

The next-step statement from this checkpoint is superseded above:ordinary
HDR60 transforms and isolated pupil/CSF age checks are now complete. Full age,
real HDR/color loading,arbitrary full-pipeline dimensions,public API and
remaining CPU preparation remain unfinished. No measured total completion
percentage is available.

## Current local checkpoint: six full32x32 pairs plus64x64 regressions

-2026-10-19:implemented and validated32x32 MTF/local adaptation using64x64
  reference FFT,then extended the full test pipeline to four bands. Six32
  pairs andsix64 regressions pass direct official Q/Q_JOD and strict stages.
  No push. Mixed GPU/application integration remains local/uncommitted;
  reference tools/docs are eligible for a separate scoped checkpoint.
-32 pair Q/Q_JOD:asymmetric_localized8.3328084945678711/8.9990530014038086;
  stronger9.9952077865600586/9.9994449615478516;brighter
  9.9999408721923828/9.9999980926513672;diagonal9.9999361038208008/
  9.9999980926513672;impulse andidentical10/10. All equal official outputs.
  Independent32 GPU checks:6*57046=342276 float values,6*2368=14208 double
  masks,zero differences.64 regressions:1368330 float,57216 double,zero.
  Raw/native SPYR and centered residual logs also explicitly checked.
- Isolated native scalar FFT64 shader ABI6401:four forward/inverse transforms,
  32768 float components exact. Graphs r2cf_64/n1_64 from pinned FFTW3.3.10
  GPL sources;original C files and COPYING retained in artifact shader folder.
  SHA256=8F339ADF6FF03BB5E1D44EF1D767682ABDF136C7E82EF1C9FD30C719CAAD5460.
  Only Shader SPIR-V capability;no float64/int64. Same graph selectors8/9 and
  3/4 as reference128,with64 dimensions and inverse division by4096 once.
- Native DLL inspection accepts optional FFT side64/128,default128. '-' wisdom
  argument means fresh estimate. Native scalar/unaligned one/three-thread
  plans match eight comparisons;aligned AVX2 plans differ in eight. Native
  wisdom import failed with registry-header checksum mismatch,even after
  initializing thread count first. Loaded DLL path/version verified against
  the running Octave process. No wisdom rewrite/forced import. Root cause
  remains unresolved;direct scalar numerical comparisons establish the path.
- LocalAdaptFFTInput width/height default64,explicit32/64 square guards;
  32 requires reference backend. Scratch/dispatch/crop/trace/kernel sizes now
  use dimensions. Main test compute_fft_filter_gpu infers32/64 square input.
  IQM_FFT_REFERENCE_SHADER64 is separate from original128 shader path.
- Isolated MTF+local adaptation now exact for asymmetric/impulse/HDR ramp at32
  and asymmetric64:1075200 float comparisons total,zero differences. Shared
  binary32 luminance,MTF symmetric post padding,LA replicate post padding,
  native L+M/log/exp,sigma10^-0.781367*30 and column-major interleaved FFT.
  Full input32 uses area1024/900 and residual8x8 with7.5ppd base sampling.
- Full wrapper IQM_SPYR_SIZE=32/64;localized patch x=width/2..width/2+7,
  retaining y13..20. band_count.txt records4/5. Validators/checker discover
  this count;old exports default5. Quality score indexing/pooling and physical
  base frequency are dynamic. Runner adds-InputSize/-GPUChecker,32 prefixes,
  and clears isolated probe flags. Default64 behavior passed six regressions.
- First32 application matched official Q but checker retained literal band4
  filename;fixed to dynamic b. Preserved failure,then fresh checker build and
  fresh six-case32 suite. Concurrent64 checker was not overwritten. New32
  suite:quality_gpu_base_fft_suite_32_639280130799228493;64:
  quality_gpu_base_fft_suite_639280129214275004 under validation/.
- Evidence:validation/fft32_native_639280099461894780/COMPLETED.json,PASS.txt,
  full_suite_paths.json,final_source_and_binary_hashes.json,all failed/success
  logs and prior/accepted binaries. validation/fft32_completed_path.txt selects
  it. finalize_fft32_checkpoint.ps1 checks all counts,raw/DC SPYR logs,score
  equality,source/binary hashes and Vulkan logs before completion. Updated
  portable JSON retains older18-pair/dimension checkpoints separately.
- New tools:generate_fft_size_reference.m,prepare_fft_size_gpu_reference.m,
  generate_fft64_reference_shader.ps1,validate_fft_size_plans.m,
  validate_fft_size_pipeline.m,hdrvdp_validation_band_count.m;native inspector
  and codelet checker add64 support. Root artifacts include exact commands and
  metadata;generation requires fresh folders. See tests/FFT32_VALIDATION.md.
- Added independently publishable tests/hdrvdp3_fft64 CMake project and
  verify_fft64_reference.cpp (one64x64 matrix,strict equality). A clean HEAD
  archive plus only these selected sources and updated native inspector builds
  both tools and passes all four GPU transforms/eight native scalar comparisons.
  CANDIDATE_PASS.txt and fft64_candidate_* folders retain that proof. This
  avoids committing the pre-existing mixed changes in fft_codelet_gpu_test.cpp,
  the main wrapper,LocalAdaptFFT and the general test CMake project. Published
  tools do not require those uncommitted integration changes.
- Exact next step:add32 phase_shift/localized_stronger/threshold comparisons
  using the new runner,keeping fresh exports. Before full-HDR32/three-impulse
  metric coverage,fix main wrapper's315 HDR ramp denominator and64-specific
  impulse coordinates;separate preprocessing generators already scale bysize.
  Then viewing parameters,real HDR/color,public API/other modes and CPU double
  preparation remain.32 VkFFT and arbitrary rectangles remain unsupported.
- Preserve staged shader1cf7baedb1d54be0ca8ad02d14485e510b0434e7,all pre-existing
  changes/untracked artifacts and dirty lib/VkFFT. Existing scalar commits and
  e65799b docs/tools remain local;no push in this continuation.

## Historical local checkpoint: generalized raw SPYR and32x32 comparison masks

- Continued without requesting approval. Existing local commits51525e0,
  7ce32a0 andf0ce345 are preserved;no push in this continuation. New verified
  documentation/standalone test tools are scoped separately from mixed GPU
  integration. Do not commit whole wrapper/CMake/map sources without reviewing
  their dependencies;the committed wrapper still has the old callback API.
- Nine raw-SPYR shapes at30ppd:32x32,48x48,64x64,96x96,128x128,49x65,64x96,
  65x49,96x64. Three inputs:asymmetric modulo,edge/off-center impulses and
  deterministic random. All27 cases/360816 raw float values match official
  Octave numerically exactly;3918 separately DC-centered residual values also
  match. Float signed-zero bits are not checked. No Vulkan errors/VUIDs.
- Important findings:official pyramid height is min(ceil(log2(ppd))-2,
  maxPyrHt(...)),not solely maxPyrHt. Add optional maximum_height to Spyr::compute;
  probe explicitly passes3 at30ppd. Raw128x128 now produces five bands,not six.
  At32x32/48x48 there are four bands;at49x65 residual is13x17.
- Native installed single conv2 traverses the longer image axis inside the
  tap loop. FMA applies to32-element vector blocks;the remaining coordinates
  use separate product/add rounding. Prior level2-only non-FMA logic was valid
  only for64x64. Diagnostics measure these paths independently of GPU output.
- Keep FMA/non-FMA in SEPARATE compiled pipelines. Three mixed/dual-accumulator
  shader candidates broke even64x64 despite explicit fma/precise;their failed
  exports/SPIR-V are retained. Accepted common shader compiles normally and
  with-DIQM_CORRDN_NON_FMA. New generated include:
  shaders_out/hdrvdp3/hdrvdp3_corrdn_native_tail.inc. Existing test shader source,
  its index blob and prior generated test include are preserved. Source CorrDn
  now includes native_tail.inc. Use tests/compile_corrdn_reference.ps1 with a
  fresh OutputDirectory,then explicitly install the two includes after keeping
  prior binaries. The blanket shader script does not generate this new include
  and deletes shaders_out;do not run it.
- Wrapper run_corrdn_gpu runs a second GPU pass for vector tails and selects
  already-rounded outputs on the host by pre-subsampling coordinate. No CPU
  convolution arithmetic. Staging buffers now size to actual dimensions with
  unsigned-allocation bound checks. Full metric remains64x64;only the isolated
  IQM_SPYR_DIMENSION_PROBE=1 branch exercises these nine shapes. Set
  IQM_SPYR_PROBE_PATTERN=asymmetric/impulse/random and unset probe for metric runs.
- Raw evidence:validation/spyr_dimensions_17923555764323481,
  spyr_dimensions_17923555832003632,spyr_dimensions_17923555942649684.
  compare with tests/validate_spyr_dimensions.m;arithmetic probes are
  tests/diagnose_spyr_dimensions.m and diagnose_spyr_rectangular_order.m.
- After accepted SPYR pipelines,eight fresh full64x64 metric pairs pass
  preprocessing,JND,CSF,masking,D,noise and direct official Q/Q_JOD:original6
  plusHDRstress2. Independent checks:1824440 float values,76288 binary64 mask
  values,zero differences. Suites under validation/:
  quality_gpu_base_fft_suite_639279523931956826 and
  quality_stress_hdr_gpu_base_fft_suite_639279524032176925. Same application/
  original checker binaries;no Vulkan errors/VUIDs. Earlier18-pair evidence
  remains historical and separate;do not claim18 new pairs after this change.
- Next bounded component completed:MapGPU comparison-mask resize accepts
  square32x32 binary input ->32/16/8,retains64->64/32/16/8. Replace fixed4096
  plan offset with actual h*w. Unit normalization,Q16,L1<=2 and signedQ32
  guards remain. Unchanged map ABI12 shader. A separate checker build passes
  192 cases/86016 binary64 outputs at32 and256 cases/348160 at64,all bitwise
  equal. New checker also independently rechecks all eight full-case exports:
  again74/228055 float and5/9536 double per pair,zero differences.
- Mask evidence:validation/mask32_639279525250877995. Application used for eight
  full regressions predates mask32 host change;new checker is separate under
  this evidence directory. Do not confuse it with validation/fft_publish_build
  checker or claim a32x32 full-metric result. Original IQM/checker binaries
  and source-before-mask32 hashes are preserved;main IQM has NOT been rebuilt
  to link mask32 support yet. Next build must preserve accepted executables.
- Aggregate evidence:validation/spyr_dimension_checkpoint_639280047991477467/
  COMPLETED.json and PASS.txt;spyr_dimension_completed_path.txt selects it.
  Includes source snapshot/hashes and accepted IQM/new checker executables.
  validation/finalize_dimension_checkpoint.ps1 validates all counts/logs and
  binary manifests. Cast Get-Content strings before nested ConvertTo-Json:
  otherwise PS5 serializes extended path/drive metadata very slowly. The first
  metadata attempt was interrupted;its partial directory is retained.
- Updated tests/SPYR_DIMENSION_VALIDATION.md and portable validation summary
  with a separate dimension_checkpoint,retaining previous18-case scope/hashes.
- Exact next step:implement32x32 MTF/local-adaptation with64x64 FFT. Current
  LocalAdaptFFTInput lacks dimensions;local_adapt.cpp fixes128 FFT and64 crop/
  preparation. Capture native64 FFT plans/codelets and intermediate maps first;
  parameterize dimensions with explicit guards,update reference math shaders,
  then make validators/checker discover four bands instead of fixed five.
  Existing8x8 base FFT/DC remains applicable to32x32/30ppd. General rectangular
  masking and non-8x8 residual FFT/DC remain unsupported. Raw SPYR success
  alone does not validate their full metric. Other pending work:real HDR/color,
  viewing parameters,CPU double preparation,public API and other outputs.
  Overall approximately75/100 remains an engineering estimate,not coverage.
- Preserve staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  its working-tree edits,all untracked artifacts and dirty lib/VkFFT.

## Historical checkpoint: committed scalar parity and eighteen fresh pipeline pairs

2026-10-18: user authorized scoped commits of verified work. Two local code
commits now exist; neither was pushed in this session:

- `51525e01f1f4230ba8f6e6b7d0e323f8d4fdf4b6`: direct CSF/resize parity,
  official generators, strict scalar checks and standalone CMake project.
- `7ce32a01f814c3f925f5f85314aedaf7333161e6`: exact double CSF LUT grid,
  strict double LUT test and independent knot/boundary/random generator.
- Reviewed candidate files were copied into an isolated source tree before
  commit. Both actual Git commits were then exported using git archive and
  built independently. Final committed-source CTest:8/8 pass. The scalar
  project has no Vulkan or mixed-workspace integration dependency.
- Direct CSF equation division order removes all15675 sampled double CSF/noise
  differences; age24/45/70 and AESL on/off queries also pass. CPU resize has
  45 cases/32643 values exact in both single and double.
- LUT log exponents now use the symmetric fixed256-node grid, rather than
  single-endpoint interpolation. This removes539 double node and3194 double
  query differences. Current3840 nodes/22530 queries have zero double/float
  differences. Additional42300 boundary,neighbor,clipping and random queries
  also have zero differences using the executable built from the commit.
- Eighteen fresh full-pipeline pairs all pass with the SAME application and
  independent GPU-checker binaries after the LUT fix. Four separate suites
  cover original6,HDR4,stress6,HDRstress2. All Q/Q_JOD equal official outputs;
  4104990 float values and171648 binary64 mask words match exactly. This is
  exact sampled parity,not a tolerance pass or a claim of general coverage.
- No Vulkan errors/VUIDs in eighteen application or GPU-checker logs. Shader
  map ABI12/base FFT ABI81 and their hashes are unchanged. Source snapshots
  verified unchanged during all runs;all suite binary manifests match.
- Current evidence:validation/scalar_checkpoint_639279508489310995 and
  validation/lut_grid_regression_639279511058157010,each with COMPLETED.json.
  completed_path pointers select them;the latter suite_paths.json selects
  all four fresh suites. Existing map/stress historical checkpoints remain.
- Input equivalence:both paths use the same exported binary32 luminance;
  strict preprocessing/layout,padding,parameters,DC and native column-major
  frequency selection checks remain enabled. Binary masks compare IEEE
  binary64 words;DC removal is confined to the reference-defined residual
  processing,not applied to hide errors in arbitrary bands.
- Commands:cmake build/ctest for tests/hdrvdp3_scalar;four existing
  run_quality_validation.ps1 invocations with ReferenceShader,MathDir,
  MapShader,BaseFFTShader and optional HDR/Stress. Exact artifacts/logs and
  hashes are retained. validation/finalize_scalar_lut_checkpoint.ps1 checks
  all eighteen case logs,source/binary manifests and committed scalar logs
  before generating the current metadata. PowerShell uses process-local
  ExecutionPolicy Bypass for local scripts;no machine setting was changed.
- GPU/integration work remains mixed and uncommitted;these code commits only
  publish the independently buildable scalar subset locally. Preserve staged
  corrdn_test shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7,
  its unstaged edits,all untracked artifacts and dirty lib/VkFFT.
- Next exact step:generalize the full-pipeline dimension harness beyond64x64,
  starting with one square size supported by native pyramid depth. First
  inspect hard-coded64/8 assumptions and explicit backend guards in
  map_gpu.cpp,verify_map_gpu.cpp and the reference shader. Keep unsupported
  dimensions guarded;record official band shapes/padding/DC before changing
  dispatch or FFT/resize code. Retain the eighteen-case suite as regression.
  Remaining CPU preparation includes double nCSF/MTF/AESL/noise/power and
  physical frequency;calibration/plans and readbacks also remain on CPU.
  Real HDR/color,viewing parameters,public API and other output modes are
  unvalidated/incomplete. Overall completion is only an engineering estimate,
  approximately75/100;no measured whole-metric coverage percentage exists.

## Historical local checkpoint: ABI12 plus eight stronger/threshold comparisons

2026-10-18: continued reference comparison with unchanged metric/map ABI12
and base FFT ABI81. Only test input branches,Stress runner and native summary
were added. Original ten accepted pairs remain a separate core checkpoint.

- Eight new pairs:sinusoidal_x/y phase_shift,asymmetric localized_stronger,
  asymmetric threshold_below/nominal/above,hdr_edges phase_shift and hdr_ramp
  localized_stronger. Phase shift wraps x/y by8;localized_stronger multiplies
  the existing8x8 patch by2. Threshold gains are nextafter neighbors of1.001f.
  C++ single exported luminances are the common reference input.
- All eight pass strict preprocessing/JND/CSF/masking/raw/final D,double noise,
  bitwise double masks and direct official Q/Q_JOD. New GPU comparisons:
  1824440 single values,76288 double mask bit patterns,zero differences.
  No Vulkan errors/VUIDs in eight run or independent GPU logs.
- Example exact Q/Q_JOD:sinusoidal_x5.3645796775817871/6.2898049354553223;
  sinusoidal_y5.3645696640014648/6.2897944450378418;asymmetric localized2x
  3.6115429401397705/4.4040126800537109;HDR edges shift
  3.4347231388092041/4.204805850982666. Quality validation now includes low Q.
- Native base selections:x/y periodic indices17/3 (one-based column-major),
  both0.9375cyc/deg;threshold cases index33/frequency1.875;HDR shift index10/
  frequency0.66291260736238833. This broadens end-to-end frequency coverage.
- Native active masks for threshold below/nominal/above:0/4056/4096 pixels.
  Nominal gain does not imply each rounded ratio is exactly0.001.
- Fresh canonical asymmetric/localized control matches58 selected numeric text
  files from the earlier ABI12 export by SHA256. Original core binary is
  preserved in the new stress evidence directory;shader was not regenerated.
- Evidence:validation/quality_stress_639279178899870989/COMPLETED.json,PASS.txt,
  case_summary.json/log,CONTROL_PASS.txt,COMMANDS.txt,source/binary hashes.
  New suites:quality_stress_gpu_base_fft_suite_639279179397567819 and
  quality_stress_hdr_gpu_base_fft_suite_639279179398732111 under validation/.
  quality_stress_completed_path.txt selects new evidence;map final/completion
  pointers still select the original ABI12 core checkpoint,not the stress set.
- Updated tests/HDRVDP3_VALIDATION_SUMMARY.json locally to ABI12,18 cases with
  separate original_10/stress_8 groups,relative paths and remaining-work list.
  Combined groups:4104990 single values,171648 double masks,zero differences.
  These are separate accepted runs/binaries;do not describe18 fresh runs of one
  binary. Published remote summary remains ABI8;no commit/push performed.
- Source changes:input-only wrapper branches,two extra sinusoidal patterns,
  phase/strong-localized/threshold transforms;run_quality_validation.ps1 adds
  -Stress with separate prefixes;new summarize_quality_stress.m;validation docs
  and portable JSON. A malformed JSON-array append was detected and corrected
  before acceptance;the earlier diagnostic manifest is retained. All final
  source hashes and native eight-case summaries were validated explicitly.
- Remaining CPU and metric work unchanged:double CSF/MTF/AESL/noise/power,
  physical frequency,calibration/plans,diagnostics;general dimensions/viewing,
  real HDR/color,public API and other outputs. Next:continue double preparation
  port or parameter/dimension generalization using these stronger regressions.
  Full metric scope is still64x64/30ppd/age24/D65 positive synthetic luminance.
- Preserve mixed work,staged shader1cf7baedb1d54be0ca8ad02d14485e510b0434e7
  and dirty VkFFT. End-of-session handoff updated;no permission is needed for
  routine authorized continuation.

## Historical local checkpoint: ABI12 GPU exact double comparison-mask resize

2026-10-18: supported64x64 binary comparison masks now resize on GPU for
square64/32/16/8 outputs. Horizontal Q16 and vertical signed Q32 integer
convolution reproduce native double results exactly; output is IEEE binary64
low/high words,bitcast on host. CPU still prepares bicubic/symmetric plans.
Guards require unit normalization,exact Q16 weights and L1<=2 per destination.
Horizontal magnitude<=2*2^16,vertical<=2^34:all native double products/sums are
exact within53-bit precision. No approximate float resize/pair/tolerance used.
Negative bicubic lobes/overshoots are retained. General double resize remains
CPU code; full-metric scope still64x64/30ppd/age24/D65 positive luminance.

- Independent native fixture:64 fields x4 sizes,256 cases/348160 binary64
  outputs,0 bit differences. Covers zero/one,corners/center impulses,checker,
  directional edges/stripes and random densities. Final explicit count is in
  mask_resize_bitwise.log (initial log used a generic float label).
- Ten fresh full chains match official Q/Q_JOD and all earlier strict stages,
  including exact double noise. GPU checks retain74 float comparisons/228055
  values per pair and add5 binary64 comparisons/9536 values per pair. Totals:
  2280550 single values and95360 binary64 mask bit patterns,zero differences.
  Both highpass/orientation first bands are64x64,explaining the mask count.
- Actual pipeline quality_mask_bandN.txt exports use17 digits and are compared
  with native imresize before quality pooling. The independent checker also
  compares GPU masks bit-for-bit with native exports. New --maps checks require
  fresh quality_mask_native_bandN.txt files; do not reuse older incomplete exports.
- Fresh CPU maps/default DFT fallback passes raw/final D,exact double noise,
  double masks and official Q/Q_JOD. Unsupported shape/nonsquare/nonbinary masks
  and old ABI11 shader reject with exit2. No Vulkan errors/VUIDs in ten run or
  GPU-checker logs. Shader:200 NoContraction,no Float64/Int64;base FFT still81.
- Evidence:validation/mask_resize_gpu_639279174670035352/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source/binary hashes,shader metadata,all native/bitwise/guard/
  fallback/full-suite logs. Previous ABI11 executables are preserved there.
  Suites:quality_gpu_base_fft_suite_639279175960765914 and
  quality_hdr_gpu_base_fft_suite_639279175961596220 under validation/.
  Current final shader/completion pointers select ABI12.
- Changed:map_gpu.h/.cpp,shader template,wrapper(GPU branch plus templated
  float/double export preserving9/17 digits),native quality validator,checker,
  suite backend guard,new generate_binary_mask_resize_reference.m and docs.
  State::execute_words preserves raw integer/double payloads;float execute is
  a memcpy wrapper. Numerical sources remained unchanged during final suites.
- Remaining CPU:double nCSF/MTF/AESL evaluation,noise reciprocal/scalar double
  power,physical double frequency,calibration/filter/resize plans,diagnostics.
  Next:port remaining double preparation or generalize dimensions/conditions
  while retaining strict native/bitwise checks. Real HDR/color,public metric
  API and other outputs remain unfinished. No percentage/completion claim.
- No commit/push performed;published summary JSON still ABI8,refresh it on
  publication. Existing staged shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved. Handoff updated.

## Historical local checkpoint: ABI11 GPU CSF parameters; double order fixed

2026-10-18: the remaining measured double CSF/noise mismatch was caused by
reciprocal/multiply rewrites. evaluate_ncsf now preserves the reference's
term1/pow(term2,p3) and p4/denominator divisions. Before/after binaries on the
same15675-case grid demonstrate5146->0 double CSF differences and4933->0
double noise differences. This is exact equality,not a tolerance/float-only pass.

- Map ABI11 mode22/base_csf_parameters moves single log10,clamp,upper-bound
  lookup and four parameter interpolations onto GPU. CPU prepares the fixed
  7-node/28-coefficient/24-slope LUT. Input luminance domain[1e-5,1e5],finite.
  Trace is5 floats/query. GPU fixture:1045 queries/5225 values,zero differences;
  downstream15675 double noise and single neural coefficients also exact.
- Strict regression modes:IQM-csf-reference-test --single-luminance-strict and
  IQM-base-noise-test --strict-double fail on any double difference. Existing
  1152-case age24/45/70,AESL on/off double-luminance and single-luminance fixtures
  both match exact double results. This does not broaden full-metric scope.
- Ten fresh full chains match official Q/Q_JOD,all maps and exact base double
  noise. Native D validation now asserts noise equality (expected Inf at zero
  frequency included). Independent GPU:74 comparisons/228055 values per pair,
  2280550 total,zero differences. Pipeline trace/backend also checked.
- Fresh CPU maps/default DFT localized fallback passes raw/final D,double noise
  and official Q/Q_JOD. Unsupported zero/below/above luminances and obsolete
  ABI10 shader reject with exit2. No Vulkan validation errors/VUIDs in ten run
  logs. Shader:200 NoContraction,no Float64/Int64; base FFT remains ABI81.
- Evidence:validation/base_csf_order_639279166321142223/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source_hashes.json,binary_hashes.json,strict/before-after/GPU/
  age/fallback/guard logs,old csf_before.cpp and preserved ABI10 executables.
  Suites:quality_gpu_base_fft_suite_639279167705195338 and
  quality_hdr_gpu_base_fft_suite_639279167713649747 under validation/.
  Final map/completion pointers now select ABI11. Previous artifacts preserved.
- Changed:csf.h/.cpp(CPU operation order plus calibration/evaluation helpers),
  map_gpu.h/.cpp,map shader template,wrapper,verify_map_gpu.cpp,native CSF/D
  generators and validator,strict CSF/noise checkers,suite runner and validation
  document. Numerical source hashes remained unchanged during final suites.
  No commit/push performed; portable published JSON still describes ABI8 and
  should be refreshed when publishing. Staged shader/VkFFT untouched.
- Remaining CPU:double nCSF/MTF/AESL evaluation,noise reciprocal/scalar double
  power,physical double frequency,calibration/plans,double mask resize and
  diagnostics. Next: independently port the remaining double preparation or
  tackle parameter/dimension generalization without losing these strict checks.
  Full scope remains64x64/30ppd/age24/D65 positive luminance;real HDR/color,
  public API and other outputs remain unfinished. No completion percentage.
- End-of-session handoff updated per latest AGENTS. Preserve mixed work and
  staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7.

## Historical local checkpoint: ABI10 GPU rounded norm and first maximum

2026-10-18: finite_hypot uses exact96-bit integer-limb squared/midpoint
comparisons to reproduce binary32 nearest/ties-to-even norm. GPU mode21
suppresses DC,stores magnitudes row-major and chooses the first maximum in
column-major order. ABI81 FFT + ABI10 maps now remove CPU hypot/index selection
from the optional GPU FFT route. Physical frequency remains double CPU work.
No Float64/Int64 capability;183 NoContraction. Domain:8x8 spectrum,zero or finite
normal components of unbiased exponent[-90,100]; unsupported inputs throw.

- Independent final fixture:1024 arbitrary complex spectra,65536 magnitudes
  and1024 indices exactly match Octave. Includes exact Pythagorean midpoint
  ties,rounded unequal-norm ties,DC-only/zero,signs and broad exponents.
- Existing64 native FFT cases pass8192 spectral values and GPU-selected
  indices/frequencies;512 adaptation cases still pass1024 scalar values.
  Exponent-91/101 and obsolete ABI9 shader guards reject with exit2.
- Ten fresh full chains match official Q/Q_JOD exactly. Independent GPU:
  73 comparisons/228050 values per pair,2280500 values total,zero differences.
  Native magnitude and actual pipeline index/backend are checked explicitly.
  No Vulkan validation errors/VUIDs in the ten run logs. CPU fallback passes.
- Additional base CSF/noise audit:15 frequencies x1045 luminances at age24,
  15675 cases. CSF single differences0,double differences5146. Noise double
  differences4933,max finite delta1.3322676295501878e-15. Final single neural
  coefficient differences0. Double CSF/noise parity remains UNRESOLVED; do not
  describe the entire scalar preparation chain as double-exact.
- Evidence: validation/map_gpu_abi10_639279156150953405/COMPLETED.json,PASS.txt,
  COMMANDS.txt,source/binary hashes,shader metadata and all fixture/guard/logs.
  Suites: quality_gpu_base_fft_suite_639279157199426535 and
  quality_hdr_gpu_base_fft_suite_639279157317562109 under validation/.
  Final map/completion pointers select ABI10; base FFT remains ABI81.
  Earlier ABI9 executables are preserved in the ABI10 directory.
- New sources:tests/math_reference/hypot_reference.glsl,
  tests/generate_base_selection_reference.m,tests/generate_base_csf_reference.m,
  tests/verify_base_noise.cpp. Existing map generator/template/host,quality
  helper,wrapper,checker,native D validation and suite runner were updated.
  Only a diagnostic CMake target was added after full suites; the earlier
  source hash snapshot is preserved separately. Numerical sources did not
  change after the full-suite builds. No commit/push performed.
- Next: resolve/port base CSF/noise/scalar double power while retaining exact
  final float coefficients. CPU frequency/calibration/plans/double mask resize
  and diagnostics remain. General dimensions/viewing/color/real HDR,public API
  and other outputs remain unfinished. Scope is still64x64/30ppd/age24/D65.
- Preserve all mixed work/staged shader/dirty VkFFT. Shader staged blob:
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7. Published portable summary still
  describes ABI8; refresh on publication. End-of-session handoff updated.

## Historical local checkpoint: ABI9 base geometric adaptation

2026-10-18: base mean/power10 now execute on GPU (mode20), with separately
rounded single mean and source-backed native power. The GPU branch no longer
computes their CPU equivalents. Shader ABI9 rejects obsolete ABI8 maps; the
optional base FFT remains ABI81. No Float64 capability;183 NoContraction.

- Independent native fixture:512 cases/1024 values,zero differences. Tests cover
  constants, adjacent floats, signed cancellation, small and random fields.
- Ten fresh full-chain synthetic pairs match official v3.0.7 Q/Q_JOD exactly.
  Independent GPU checks:72 comparisons/227986 values each,2279860 total,zero
  differences. Includes native mean-log and geometric adaptation outputs.
  Existing64x64/30ppd/age24/D65 positive-luminance scope remains unchanged.
- Fresh CPU maps/default DFT localized fallback also matches official scores.
  Out-of-range/span/tiny inputs and old ABI8 shader reject with exit2. No Vulkan
  validation errors or VUIDs occur in the ten completed pipeline logs.
- Local evidence: validation/map_gpu_abi9_639279140029974208/COMPLETED.json,
  PASS.txt,COMMANDS.txt,source_hashes.json,binary_hashes.json,shader_metadata.json.
  Suites: quality_gpu_base_fft_suite_639279141141654008 and
  quality_hdr_gpu_base_fft_suite_639279141707759061 under validation/.
  Current final shader/completion pointers select ABI9. Previous ABI8 executables
  were preserved in the ABI9 evidence directory before rebuilding.
- Domain:64 finite single logs in[-5,5],nonzero exponent span<=16,minimum>=-90;
  zeros allowed. Unsupported inputs throw. The initial native fixture included
  outward +/-5 neighbors; corrected to inward neighbors,old failure retained.
  Native stderr informational output required Continue during one fixture run;
  the final retry completed successfully. All initial logs remain preserved.
- Changed: map_gpu.h/.cpp,map shader template,wrapper base-mean branch,
  verify_map_gpu.cpp,validate_difference_maps.m,new native fixture generator,
  tests/MAP_GPU_VALIDATION.md. All implementation remains local/uncommitted.
  No commit or push was performed. Staged corrdn-test shader blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7;dirty VkFFT preserved.
- Next: independently validate/port base hypot and first-max selection or base
  CSF/noise/scalar double power. CPU calibration/plans,double mask resize and
  diagnostics also remain. Then generalize dimensions/viewing/color/real HDR,
  expose the public API and implement remaining outputs. Published summary JSON
  still documents the historical ABI8 checkpoint; refresh it when publishing.
- Per the latest AGENTS instructions (2026-10-18),this handoff is updated at the
  end of the session. Preserve historical evidence and mixed working changes.

## Historical published checkpoint: ABI8 maps and ABI81 native-matching base FFT

This section supersedes the ABI7 checkpoint below. This publication contains
English documentation and a portable summary only. Implementation, test tools,
generated shaders and full logs remain local/uncommitted.

- Ten fresh 64x64/30ppd/age24/D65 synthetic pairs match official v3.0.7
  Q/Q_JOD exactly. Independent GPU checks: 71 comparisons /227984 values per
  pair, 2279840 values total, zero differences; native base index/frequency
  assertions also pass. See tests/MAP_GPU_VALIDATION.md and the summary JSON.
- GPU preparation now includes guarded64-sample residual DC, separately
  normalized band subtraction and original binary comparison mask. Optional
  ABI81 computes the native-matching8x8 base FFT; magnitude/index/frequency
  selection still runs on CPU. No shaderFloat64 is required.
- Local accepted checkpoint: validation/map_gpu_abi8_639278691970167183/.
  Combined suites: quality_gpu_base_fft_suite_639278697831326240 and
  quality_hdr_gpu_base_fft_suite_639278700680928595 under validation/.
- Short publication checks: two fresh localized repeats each match53 numeric
  files from the accepted export by SHA256. Fresh localized and identity scores
  match official Octave exactly; identity band/total errors are0 and Q/Q_JOD10.
  Three incompatible shader configurations reject with exit2. No Vulkan errors
  or VUIDs occur in the fresh pipeline logs. These were sequential runs.
  Evidence: validation/abi8_quick_checks_639278706524535142/.
- DC domain: exactly64 samples; finite nonzero exponents[-90,100], span<=16,
  zeros allowed. Comparison-mask reference must be positive finite. Do not
  extrapolate this validation to black/exceptional inputs or arbitrary sizes.
- Remaining CPU: calibration/plans, base hypot/first-max/physical frequency,
  base geometric adaptation/CSF/noise/scalar double power, double bicubic mask
  resize and diagnostics/readbacks. Next: independently validate remaining
  preparation ports, then dimensions/viewing/color/real HDR generalization,
  a usable public metric API and other required output modes.
- Preserve staged corrdn-test shader and dirty lib/VkFFT. Publish implementation
  separately after reviewing mixed changes. Update this note at publication.

## Historical checkpoint (superseded): ABI7 GPU quality validation

This section supersedes the historical progress notes below. This publication
contains documentation and a compact validation summary only. The implementation,
new test tools, generated shaders and full local logs remain uncommitted/local;
checking out this documentation commit does not reproduce the ABI7 implementation.
See `tests/MAP_GPU_VALIDATION.md` and `tests/HDRVDP3_VALIDATION_SUMMARY.json`.

- All ten fresh synthetic pairs match official HDR-VDP-3 v3.0.7 Q/Q_JOD exactly.
  Scope:64x64,30ppd,age24,D65 luminance; local Octave11.3 and Intel Arc Pro Vulkan.
- GPU now evaluates adaptation input/mean/log10, pupil, all13 JND stages,
  resize/CSF, raw/pool/neighbor masking, N_mask, both D transforms, quality
  reduction and scores. CPU remains responsible for calibration/resize plans,
  residual DC, normalized difference preparation, base-frequency/adaptation/noise
  preparation and the original/double-resized comparison mask.
- Ten fresh exports pass2140960 independent GPU intermediate comparisons with
  zero differences. JND knot checks pass258048 values using native/production LUTs.
  Power, log10, resize, CSF, pool and clipping fixtures also pass. No tolerance
  substitution or Vulkan validation errors were accepted.
- Final shader: `validation/map_gpu_abi7_639278683626481099/map.spv`.
  Read `validation/map_gpu_final_shader_path.txt` and
  `validation/map_gpu_completed_path.txt` for current local pointers. ABI7 rejects
  older map shaders. Generate from `tests/generate_power_shaders.ps1`; do not
  compile the map template directly. Generated math does not read Windows DLLs
  or require shaderFloat64. Default CPU maps remain available and verified.
- Baseline: `validation/quality_gpu_maps_suite_639278684479473340/PASS.txt` plus
  `GPU_INTERMEDIATES_PASS.txt`. HDR:
  `validation/quality_hdr_gpu_maps_suite_639278686318506540/PASS.txt`.
  Full logs, sources and binary hashes remain local; the committed summary
  records their identifiers/hashes and scores without uploading large artifacts.
- Next: finish the remaining CPU preparation ports with independent native
  fixtures, generalize dimensions/viewing conditions and real HDR/color inputs,
  then expose a usable metric API and implement other required output modes.
  This is not a completed general-purpose implementation or a percentage claim.

Preserve the existing staged corrdn-test shader and dirty `lib/VkFFT`. Publish
implementation separately only after reviewing the mixed local changes. Keep
commit messages and published documentation in English. Per the user's latest
instruction, update this handoff when publishing to GitHub, not every small step.

## Objective and constraints

Reimplement official HDR-VDP-3 v3.0.7 in C++/Vulkan, prioritizing numerical
and algorithmic parity over speed. Preserve all existing changes, staged
files, untracked artifacts and dirty `lib/VkFFT`. No resets/cleaning. User
authorized publishing verified changes; the scoped test checkpoint below is
committed/pushed. Other unfinished work remains local.
The earlier every-session handoff request was superseded by updates on GitHub publication.

Original handoff read from `C:/Users/DenizAksoy/Downloads/AGENTS.md`.
Official local reference: `C:/HDR-VDP-3_Ref` (`hdrvdp_version()` reports 3.07).
Do not edit official reference files or overwrite old `C:/temp` references.

## Validated so far

- SPYR (`sp0Filters`, reflect1, height 3) has five bands for 64x64:
  64x64, 64x64, 32x32, 16x16, 8x8 = 9536 values per image.
- Three patterns (diagonal modulo, asymmetric modulo, edge/off-center impulses),
  each with reference and 5%-brighter test images: all shared-JND SPYR values
  match Octave single exactly, including raw and DC-removed residuals.
- `corrDn` requires reverse column tap order. Standard shader uses explicit
  FMA; level 2 (16x16 in this harness) uses separate multiply/add. This is
  validated for the current size/Octave build, not arbitrary dimensions/MEX.
- Native single pupil/LMSR/JND is implemented in `src/hdrvdp3/jnd_single.cpp`,
  compiled with `/fp:strict`. On identical GPU adaptation maps, all six images'
  pupil coefficients, 13 per-pixel intermediate values and final JND match.
  The double API remains. Spectral constants apply to age-24 D65 luminance.
- Separate docs: `tests/SPYR_VALIDATION.md`, `tests/JND_PATH_VALIDATION.md`.

## Current preprocessing integration

- Harness: `bin/shared/wrappers/hdrvdp3.cpp`.
- Default `IQM_PREPROCESSING=none` preserves isolated MTF-off tests.
- Explicit `IQM_PREPROCESSING=fft` runs GPU optical MTF -> native single L+M
  adaptation input -> GPU FFT adaptation -> CPU native pupil/JND -> GPU SPYR.
- Optical MTF defaults to symmetric post-padding (`surround='none'`). The old
  standalone MTF test used replicate padding; its result is not default parity.
- Adaptation uses log/clamp, replicate post-padding, Gaussian FFT, crop/exp.
  Gaussian filter construction uses double then casts to float. The FFT path
  rejects sigma <4.3 (reference then uses spatial filtering).
- Experimental FFT preprocessing uses `LocalAdaptFFT`, complex float32.
  Default backend remains VkFFT; explicit `IQM_FFT_BACKEND=reference` loads
  the verified parity SPIR-V via `IQM_FFT_REFERENCE_SHADER`, requiring column
  axis order and native twiddle option. No VkFFT plan is created in this mode.
  Four optional trace snapshots: prepared, forward, filtered, inverse.
  Radial filters and buffers are fixed at 128x128; image input/output at 64x64.
- Every run writes a unique `validation/spyr_spatial_*` directory, with JND,
  luminance, adaptation, metadata, raw SPYR bands and optional FFT snapshots.
- SPYR exports now have explicit ref/test prefixes; fragile per-band counter
  naming was removed. Values are row-major; Octave band exports column-major.

## Full-chain baseline (before FFT axis-order experiment)

Full original-luminance comparison still fails exact parity. Maxima over each
reference/test pair, from `validation/fft_*_comparison.log`:

| Pattern | Retinal MTF max | Adaptation max | JND max | Raw residual max |
|---|---:|---:|---:|---:|
| Diagonal | 5.7220458984375e-6 | 8.58306884765625e-6 | 0.0003662109375 | 0.001953125 |
| Asymmetric | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.001953125 |
| Impulse | 3.814697265625e-6 | 1.049041748046875e-5 | 0.000244140625 | 0.0029296875 |

Same-input JND/SPYR tests pass for all six FFT-preprocessed images. GPU runs
had no VUID/validation errors. Full-chain `--strict` test correctly fails.
Existing ramp FFT integration test passed its old tolerances only:
max=0.00016022, RMSE=0.00002903. See `tests/FFT_PREPROCESSING_VALIDATION.md`.

## Latest session: source-backed math passes; direct CSF age correction validated

- Replaced Windows binary/table dependency with original MIT-licensed AMD
  win-libm sources, commit6121d02aef0c775947d5dc655d7fc4ccfe2144ab:
  https://github.com/amd/win-libm. Preserved logf.asm, expf.asm and3 original
  table files with full notices in tests/math_reference/amd_win_libm/.
  SOURCES.json records LF-normalized hashes; generator verifies them. No DLL
  load/read required by tests/generate_reference_math_shaders.ps1.
- tests/math_reference/native_math.glsl uses the source FMA graphs and existing
  compensated helpers. A fresh seed exposed1 near1 log difference from relaxed
  GPU division. Finite compensated-division fix failed at1-eps(single); final
  normal-operand integer long division rounds ties-to-even exactly. Failed
  intermediate shaders/logs preserved, not parity passes.
- Final source-backed shader passes3 datasets,65536 pairs each:196608 tested
  pairs /393216 function evaluations, zero error. Includes repeated image pairs
  and distinct random seeds71439,20261017,121339; not all unique random samples.
  Scope log[1e-6,1e10], exp[-80,80]; no special/subnormal-log/overflow guarantee.
  Shader feature usage is float32/uint32 only; only current device/reference
  environment tested. Source-backed does not mean universal libm equality.
- Generator output validation/source_math_exact_division_639278581939061846:
  math/prepare/finish/multiply .comp/.spv. Tests check_*.log there plus
  validation/source_math_seed121339_final.log. Source generator's added hash
  check rerun: all4 .comp files byte-identical to tested final sources.
  Manifest regeneration under source_math_manifest_check_*.
- Fresh integrated source-math runs all pass strict stage checks, full
  preprocessing strict maps, native JND: validation/source_math_<pattern>_
  {run,stages,full,jnd}.log. Exports spyr_spatial_17922614797262557 (asymmetric),
  spyr_spatial_17922615112177470 (diagonal),17922615425028076 (impulse).
  No Vulkan validation errors. Main build source_math_csf_integration_build.log.
- CSF inspection found substantive AESL formula bug: old inner
  10^(slope*log2(...)) differs from official 10^slope * log2(...). Fixed in
  src/hdrvdp3/csf.cpp, also matching interpolation/reciprocal/power order.
  csf.cpp now gets strict FP/no contraction in main CMake, like jnd_single.cpp.
- New isolated IQM-csf-reference-test, tests/verify_csf_reference.cpp and
  generate_csf_reference.m:1152 paired queries across ages24/45/70, AESL on/off,
  12freqs and16lums. Before:320 float mismatches,444 double mismatches,
  max1.2169949371813404. After: **0 float mismatches**, but283 double differences
  remain, max2.2204460492503131e-16. --float-output passes exact float comparison;
  default strict double still exits1. Do NOT claim exact double API equality.
- Initial scalar-rho/vector-lum low-cutoff fixture hit reference helper's scalar
  logical-index shape quirk; final paired rho/lum vectors use point-query cutoff
  semantics. See tests/CSF_VALIDATION.md. Final reference dir
  validation/csf_paired_reference_639278581311962498; logs csf_paired_before_fix.log,
  csf_corrected_comparison.log, csf_float_output_comparison.log.
- Documentation tests/math_reference/README.md and tests/CSF_VALIDATION.md;
  ADAPTATION_MATH_VALIDATION.md points to source-backed follow-up. Prior pinned
  binary probe preserved as historical local evidence, not used by final generator.
- No new commit/push; last remote checkpoint b46666d. Shared prior work, dirty
  VkFFT, original staged shader and all references preserved.
- Exact next step: validate256-entry CSF LUT/grid interpolation and full CSF maps
  with reference/test mean adaptation, matching log-domain Octave imresize
  (bicubic/antialias/boundaries/precision). Current bilinear map is still an
  approximation. Direct float nCSF pass does not validate LUT/maps or masking.

## Previous session: pinned math probe makes all current strict preprocessing pass

- Investigated remaining adaptation log/exp differences with65536 isolated pairs
  (24576 actual image pairs + synthetic log[1e-6,1e10], exp[-80,80]). Added
  tests/float_math.comp and generate_float_math_reference.m. Initial series
  experiments failed: FMA residual36930 total differences; split-product
  compensation24 differences (21log/3exp), including5 actual-log differences.
  Some series outputs equal double-to-float rounding but not native logf.
- Actual liboctave-13.dll imports logf/expf from Windows UCRT math API, not the
  generic MinGW wrapper. Read-only import/export/arithmetic inspection under
  validation/octave_math_imports.txt, ucrt_math_exports.txt,
  ucrt_logf_disassembly.txt, ucrt_expf_disassembly.txt.
- Local version-pinned generator validation/generate_ucrt_log_probe.ps1 rejects
  any ucrtbase.dll hash except5C52E3A303BAAAC0E0AF8BD9B96134993DA34BC9D834A31EF37E1D2CDC7FE192
  (version10.0.26100.9444). Native log graph plus compensated native exp graph
  matches all65536 pairs /131072 function outputs exactly. No float64 feature
  required. Binary-derived tables/probe artifacts stay LOCAL, not portable math.
  Evidence validation/ucrt_log_exp_probe_comparison.log. Ref dir
  validation/float_math_reference_639278571099031674; earlier failed logs preserved.
- Added original tests/float_multiply.comp (integer24x24 significand product,
  explicit ties-to-even, gradual underflow) and generate_float_multiply_reference.m.
  All262144 finite pairs match, including196608 actual spectrum products plus
  randomized and subnormal/tie/signed-zero cases. Evidence float_multiply_comparison.log;
  reference validation/float_multiply_reference_639278575349483287.
- LocalAdaptFFTInput.referenceMathDirectory is an explicit runtime override for
  prepare.spv, finish.spv, multiply.spv, reference FFT backend only. Harness
  IQM_FFT_MATH_SHADER_DIR sets it; exports fft_math_shader_dir.txt. Empty retains
  existing math. Config/path changes after initialization rejected. Defaults
  unchanged. Full probe folder validation/ucrt_complete_math_639278575887831116.
  Earlier math-only folder ucrt_math_shaders_639278574065405048 lacks multiply;
  not usable with the current complete-override loader.
- **All3 patterns /6 images now pass --strict-all-stages and full preprocessing
  --strict with zero mismatches**, including every filter/prepare/FFT/multiply/
  inverse/finish comparison and full retinal/L+M/adaptation/JND maps. Pupil
  coefficients equal; all57216 raw SPYR values and centered residuals equal.
  Native single JND13-stage checks also pass. No Vulkan validation errors.
  Scoped64x64,30ppd,age24,D65 luminance fixtures on this UCRT/device environment.
- Final exports: validation/spyr_spatial_17922608381270175 (asymmetric),
  spyr_spatial_17922608432949170 (diagonal), spyr_spatial_17922608486247470 (impulse).
  Logs validation/ucrt_complete_<pattern>_{run,stages,full,native_jnd,spyr}.log.
  Build ucrt_complete_math_build.log completed exit0. Default VkFFT regression
  passes old tolerance, max0.00015259/RMSE0.00002840, ucrt_complete_default_regression.log.
- First log/exp-only asymmetric probe spyr_spatial_17922606838939870 already
  yielded exact exported maps, but subnormal products still differed internally;
  final integer-product probe removes those differences too. Do not conflate scopes.
- tests/diagnose_fft_stages.m adds --strict-all-stages; shared GPU harness adds
  --float-math/--float-multiply with per-function diagnostics. Full documentation
  tests/ADAPTATION_MATH_VALIDATION.md; no production math shader defaults edited.
- No commit/push: version-specific binary-derived probe tables and shared files
  remain local; last verified remote checkpoint b46666d unchanged. Existing
  staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7, other work and VkFFT preserved.
- Next: replace the pinned binary probe with maintainable portable source-backed
  math behavior (preserve exact target/reference precision), broaden its domain
  tests, then CSF LUT/imresize/masking/pooling/score parity. Current pass is
  preprocessing/JND/SPYR on fixtures, not full HDR-VDP or entirely GPU completion.

## Previous session: reference FFT integrated; MTF/JND/SPYR now exact on6 images

- Added explicit opt-in referenceShaderPath in LocalAdaptFFTInput and runtime
  parity pipeline/scratch buffer in LocalAdaptFFT. Four immutable descriptor
  sets route work->scratch->work forward and filtered->scratch->work inverse.
  Uses proven graph8/9 and3/4 with barriers, no VkFFT plan for reference mode.
  Defaults preserved. Backend/path cannot change after initialization.
- Harness options: IQM_PREPROCESSING=fft, IQM_FFT_BACKEND=reference,
  IQM_FFT_AXIS_ORDER=column, IQM_FFT_TWIDDLES=native,
  IQM_FFT_REFERENCE_SHADER=<absolute validated SPIR-V path>. Used existing
  validation/fft_forward2d_initial.spv. Trace backend and inverse storage
  metadata exported; reference inverse stays column-major, crop honors it.
- diagnose_fft_stages.m and diagnose_fft_order.m now honor inverse storage
  contents, not just metadata file existence. New --strict-transforms asserts
  identical-input forward/inverse equality while reporting other differences.
- Completed build validation/fft_reference_backend_build.log.3 GPU runs with
  Vulkan validation: asymmetric spyr_spatial_17922599150543331,
  diagonal spyr_spatial_17922599673027534, impulse spyr_spatial_17922599723530707.
  No Vulkan validation errors found. Each covers ref/test,6 images total.
- **Every MTF stage, full retinal and L+M input match exactly** (24576 pixels
  each). Same-input forward/inverse match for both MTF/adaptation, all6 images.
  Native pupil/JND coefficients and24576 JND pixels match full official output.
  All57216 raw SPYR coefficients match, including raw and DC-removed residual.
- **Full preprocessing still fails strict parity due to adaptation maps**:
  asymmetric ref/test max6.67572021484375e-6 /5.7220458984375e-6,4426 mismatches;
  diagonal5.7220458984375e-6 /1.049041748046875e-5,4885 mismatches;
  impulse8.58306884765625e-6 /6.67572021484375e-6,4437 mismatches.
  First difference is prepare log (max2.384185791015625e-7). Same-input exp
  finish differs by up to1.9073486328125e-6; spectrum multiplication has
  subnormal differences up to about1.36e-38. Filters and FFTs themselves equal.
  Identical JND/SPYR on mild stimuli does not waive adaptation-map failure.
- Evidence validation/fft_reference_backend_<pattern>_{run,stages,strict_transforms,native_jnd}.log;
  full-chain logs asymmetric_full_absolute.log, diagonal_full.log, impulse_full.log
  with same fft_reference_backend_ prefix (strict exit1 expected/documented).
  Initial asymmetric_full.log used a relative path and failed after nested
  Octave run changed cwd; invalid numerical run, corrected with absolute path.
- Existing default VkFFT regression rebuilt/passes old tolerance:
  fft_reference_backend_legacy_build.log /legacy_regression.log, max0.00015259,
  RMSE0.00002840. Do not describe it as exact or assert unchanged numerical error.
- New documentation tests/REFERENCE_FFT_INTEGRATION.md. This integration remains
  LOCAL: shared files contain pre-existing uncommitted work and full adaptation
  still fails strict parity. No new commit/push. Last remote verified checkpoint
  remains b46666d. Existing staged shader1cf7bae..., dirty VkFFT and other work
  preserved. No files deleted or references overwritten.
- Exact next step: match native single log/exp rounding and subnormal spectral
  products, rerun all3 strict comparisons, then CSF/imresize/masking validation.

## Previous session: full forward128x128 matches and verified tests pushed

- Composed forward transform in test shader graph8/9. First dispatch applies
  real128 per column and writes65 transposed frequency vectors per matrix.
  Second dispatch uses n2fv_16/t3fv_8 forfreq0..62 and distinct n1fv_128 for63..64.
  Writes column-major full output, completing partner F(-x,128-k)=conj(F(x,k))
  for0<k<64. Both axes reflected; no normalization/DC removal. Compute barrier
  and distinct SSBOs, completion fence. Real modes reject nonzero imaginary input.
- CPU --forward2d <new directory> [GPU export directory] uses actual Octave
  single DLL,22-thread estimate/unaligned R2C2D with compact offset8064.
  Verifies all expected graphs including distinct tail. Shared CPU transform2d
  helper preserves inverse fixture generation; inverse regressions rerun.
- New tests/validate_forward2d_reference.m independently compares every expected
  matrix with native single fft2 and the native probe, asserting actual22 threads.
- **19 matrices /311296 full-spectrum complex values /622592 float components:
  max=0, RMSE=0, mismatches=0** on GPU and independent Octave checks. Includes
  conjugates;158080 compact bins computed independently. Seven synthetic plus
  twelve ref/test MTF/adaptation stage0 snapshots across3 spatial patterns.
- Evidence under validation/: fft_forward2d_initial_comparison.log,
  fft_forward2d_asymmetric_comparison.log, fft_forward2d_diagonal_comparison.log,
  fft_forward2d_impulse_comparison.log. Oracle dirs fft_forward2d_reference_639278561661925639,
  fft_forward2d_asymmetric_639278562033305292,
  fft_forward2d_diagonal_639278562208629720,
  fft_forward2d_impulse_639278562273635020 (each includes Octave PASS log).
  Shader fft_forward2d_initial.spv; build logs fft_forward2d_build.log,
  fft_forward2d_final_build.log (completed exit0).
- Final rebuilt tools generated fresh forward2d/inverse2d fixtures, both pass;
  existing R2C/main/tail also pass: validation/fft_forward2d_final_639278562885963009/.
  No Vulkan validation errors found. Production code/settings unchanged.
- Following user's authorization to publish only verified changes, committed
  nine test/doc files as **b46666df138c5b2770f004ad59603a2d0eaf28ab**,
  `Validate complete Vulkan forward 128x128 FFT against Octave`, and pushed
  origin/feature/hdr-vdp3. ls-remote confirms remote matches local HEAD.
  Existing staged shader blob1cf7baedb1d54be0ca8ad02d14485e510b0434e7 preserved;
  all other dirty/untracked production work and VkFFT preserved. Handoff local.
- Exact next step: integrate proven forward/inverse into an explicit opt-in
  preprocessing test path, preserve existing defaults and axis/layout traces,
  then compare full MTF/adaptation stages against official Octave. Same-input
  FFT success does not settle filter/log/exp/subnormal differences or full metric.

## Previous session: verified test checkpoint committed and pushed

- User explicitly requested sending only verified changes to GitHub.
  Published commit **2a333e615055daa0ebb4838c9009811891c86ceb**,
  `Add isolated Vulkan FFT parity validation tools`, to origin/feature/hdr-vdp3.
  Push succeeded and ls-remote confirms remote HEAD equals local commit.
- Commit contains24 test/source/license/documentation files only. No production
  metric/shader/preprocessing/submodule changes, binary outputs, local validation
  artifacts or root build changes included. No full metric parity claim.
- Added standalone tests/fft_parity/CMakeLists.txt and README so tools build
  without pending main-project/JND changes. MSVC /EHsc explicitly enabled.
  Build validation/fft_publish_final_build.log succeeded; no warnings/errors.
- Regenerated/compiled shipped shader and generated fresh synthetic fixtures
  using standalone binaries: all8 groups PASS (leaf8/16/128, normalized inverse2d,
  R2C, inverse second-axis6/2, forward second-axis65/3, forward tail2). No Vulkan
  validation errors. Evidence validation/fft_publish_checks_639277456979720179/.
  Public summary tests/fft_parity/VALIDATION.md; reproduction instructions in README.
- Git commit --only with explicit24-path list excluded pre-existing staged
  shaders/hdrvdp3/hdrvdp3_corrdn_test.glsl. Its staged blob remains
  1cf7baedb1d54be0ca8ad02d14485e510b0434e7, exactly as before. Unstaged changes,
  other untracked files and dirty VkFFT preserved. Pre-publish staged backup:
  validation/prepublish_staged.patch; selected list validation/fft_publish_paths.txt.
- This handoff remains local; broader unverified production changes are not
  published. Next algorithm step unchanged: full128x128 forward composition,
  correct tail placement and 2D Hermitian completion against native Octave fft2.

## Previous session: forward second-axis main and distinct tail match

- Test-only generator now translates ordinary n2fv_16/t3fv_8 to graph6.
  Eight decimated16 transforms read j+8*l; twiddle8 writes k+16*j. Powers1,3,7;
  AVX2 complex multiplication and constant FMA operations preserved. STM2
  float-offset2*k maps complex indexk; STN2 is no-op for AVX2-128.
- **Native forward tail correction:** 65 compact frequencies under22 threads
  form21 groups3 and a final group2. The group2 uses n1fv_128_avx2_128, NOT
  n2fv_16/t3fv_8. Existing native forward plan confirms it. Added graph7 for
  this ordinary SIMD128 routine. Original three C files preserved unchanged
  in tests/fftw_reference/ with existing GPL attribution/license.
- CPU --forward-second <new directory> [65|3|2] uses actual Octave single DLL,
  in-place one-thread estimate/unaligned, stride=group distance1. Plan asserts
  composite for65/3 and n1fv_128 for2. GPU fixtures contiguous per transform;
  2D compact layout/dispatch and full fft2 remain next, not yet validated.
- **Group65:520 transforms /66560 complex; group3:513 /65664; group2:512
  /65536. Total1545 transforms /197760 complex outputs. All max=0, RMSE=0,
  mismatches=0**, graph6/fused0 for65/3, graph7/fused0 for2. Constant, complex
  impulse, ramp and seeded random at four scales. No Vulkan validation errors.
- Oracle dirs validation/fft_forward_second65_639277453522684322,
  fft_forward_second3_639277453764368273, fft_forward_tail2_639277454446772600.
  Each contains codelet_plans.txt and input128.txt/forward128.txt.
  Earlier fft_forward_second2_639277453829417292 rejected unexpected tail plan;
  not a GPU parity failure and not a usable fixture. Preserve as diagnostic.
- GPU logs under validation/: fft_forward_second65_comparison.log,
  fft_forward_second3_comparison.log, fft_forward_second3_final_comparison.log,
  fft_forward_tail2_comparison.log. Shaders fft_forward_second_initial.spv and
  fft_forward_tail_initial.spv (latter adds tail graph, used for final3/tail).
  Completed builds fft_forward_second_build.log, fft_forward_tail_build.log.
- R2C and full synthetic inverse2d regressions pass with final extended shader:
  fft_forward_simd_r2c_regression.log, fft_forward_simd_inverse_regression.log.
  Production code/settings untouched; existing staged/unstaged/untracked work
  and dirty VkFFT preserved. No commits or destructive commands.
- Exact next step: compose full128x128 forward R2C; scalar real128 per column,
  compact65 frequencies; graph6 on freq0..62, graph7 on63..64; 2D Hermitian
  completion includes conjugating both axes. Compare native22-thread fft2 on
  synthetic and all three patterns' MTF/adaptation stage0 snapshots. Then
  experimental preprocessing integration and strict full-chain reruns.

## Previous session: forward first-axis R2C128 matches

- Added original FFTW r2cf_128.c (same archive/license, notices preserved) in
  tests/fftw_reference/. SHA256:
  9333D9DC8E068ABE1CA1C6F48558C42ADAA2B1B86EB0E60E0A220C249C65192F.
  Test-only generator translates ordinary graph to graph5, with precise float
  intermediates and separate product/add rounding. R0/R1 map to even/odd samples.
- GPU outputs65 bins (imaginary DC/Nyquist zero) then completes to128 by
  Hermitian conjugation. No normalization/DC removal. This is a 1D first-axis
  test, not a complete forward2d implementation. SSBO input imag components0.
- Added CPU --r2c <new directory> [GPU export directory], actual Octave single
  DLL, one-thread estimate/unaligned; rejects unexpected r2cf_128 plan.
  Optional stage0 imports honor row/column metadata and record source files.
- Added tests/validate_r2c_reference.m: independently compares every vector
  against native single Octave fft(real(input),[],1), with strict assertions.
- Four sets of512 transforms (synthetic; asymmetric/diagonal/impulse ref/test
  MTF/adaptation stage0 columns): **2048 transforms / 133120 computed bins /
  262144 full-spectrum complex values including conjugates; max=0, RMSE=0,
  mismatches=0**. Vulkan and Octave checks passed. No Vulkan validation errors.
- Artifacts validation/fft_r2c_initial.spv; build logs fft_r2c_build.log and
  fft_r2c_final_build.log (completed exit0). GPU logs fft_r2c_initial_comparison.log,
  fft_r2c_asymmetric_comparison.log, fft_r2c_diagonal_comparison.log,
  fft_r2c_impulse_comparison.log. Initial synthetic log says inverse for graph5;
  reporting label corrected in later harness build (arithmetic unchanged).
- Oracle dirs under validation/: fft_r2c_reference_639277449506445875,
  fft_r2c_asymmetric_639277450118078253, fft_r2c_diagonal_639277450256072226,
  fft_r2c_impulse_639277450390040066. Each includes r2c_plan.txt, source.txt,
  input128.txt, forward128.txt, octave_reference_check.log (PASS).
  Snapshot sources same three existing spyr_spatial directories as below.
- Full synthetic inverse2d regression passed with extended shader:
  validation/fft_r2c_inverse2d_regression.log. Production path/settings unchanged;
  existing staged/unstaged/untracked changes and dirty VkFFT preserved.
- Next: isolate forward SIMD n2fv_16_avx2_128 and t3fv_8_avx2_128, including
  native batch-tail choices. Compose full128x128 R2C/Hermitian completion and
  compare against native22-thread fft2, then experimental preprocessing
  integration and full-chain strict reruns. log/exp/subnormal issues remain.

## Previous session: full normalized 128x128 Vulkan inverse matches

- Extended test-only generator/harness with two dispatches: graph3 scalar
  first axis writes transposed intermediate, compute write/read barrier,
  graph4 SIMD second axis transposes back and divides once by 16384.
  SSBO layout is interleaved complex float32, column-major per matrix.
  Distinct input/intermediate/output buffers and descriptor sets; fence before
  readback. Both complex components compared; no DC removal/crop/tolerance.
- CPU tool `--inverse2d <new directory> [GPU export directory]` uses the actual
  Octave single FFTW DLL, 22-thread estimate/unaligned 2D plan; verifies expected
  scalar/SIMD decomposition. Optional export imports ref/test mtf/la stage2,
  honoring fft_axis_order.txt, and records exact source files.
- New `tests/validate_inverse2d_reference.m` independently asserts every CPU
  expected matrix equals native Octave ifft2 and native single probe; actual
  single thread count asserted22. All four reference sets passed (exit0).
- **19 matrices / 311296 complex outputs / 622592 float components match
  exactly: max=0, RMSE=0, mismatches=0.** Seven synthetic (constant, off-center
  complex impulse, asymmetric ramp, random four scales) plus twelve snapshots
  (ref/test MTF/adaptation for asymmetric, diagonal, impulse).
- GPU comparison logs under validation/: fft_inverse2d_initial_comparison.log,
  fft_inverse2d_asymmetric_comparison.log, fft_inverse2d_diagonal_comparison.log,
  fft_inverse2d_impulse_comparison.log. No Vulkan validation errors found.
  Prior size8/16/128 regression passes: fft_inverse2d_leaf_regression.log.
- Oracle dirs under validation/: fft_inverse2d_reference_639277446909181448,
  fft_inverse2d_asymmetric_639277447266540857,
  fft_inverse2d_diagonal_639277447313943663,
  fft_inverse2d_impulse_639277447340166724. Each includes inverse2d_plan.txt,
  input128.txt/inverse128.txt, source.txt and octave_reference_check.log.
  Snapshot sources: spyr_spatial_17921430489879733, 17921430997117838,
  17921431152989965. Octave exit diagnostic after PASS is the previously known
  execution_exception message; assertions and actual exit0 confirmed.
- Built test targets successfully (reviewed MSBuild escalation):
  validation/fft_inverse2d_build.log. Shader validation/fft_inverse2d_initial.spv.
  Production preprocessing code/settings unchanged; GPL-derived routines still
  test-only. Existing dirty/staged/untracked changes and VkFFT preserved.
- Next: forward R2C first-axis and SIMD second-axis/tail graph parity with
  native fft2. Then integrate proven transforms into an explicit experimental
  preprocessing path and repeat full-chain strict comparisons. Same-input
  inverse success does not prove input spectra, log/exp or full metric parity.

## Previous session: second inverse axis and native batch tails match

- Extended test-only shader generator with the ordinary `t3bv_16` SIMD graph,
  preserving AVX2 explicit FMA in complex multiplication and constant FMA
  operations. The generic FMA-oriented graph failed and is not selected.
  Original t3bv_16.c and simd-avx2-128.h preserved under tests/fftw_reference/
  with original notices and existing GPL COPYING; no production integration.
- Added CPU `--second-axis <new directory> [128|6|2]` oracle mode. Must plan
  **in place**, complex stride128, distance1; out-of-place chooses scalar
  t2_8 and was an invalid oracle for this target. Final oracle rejects a plan
  lacking t3bv_16_avx2_128 or n1_8. Reference still uses actual Octave DLL.
- SIMD twiddle powers are 1,3,9,15. Sixteen decimated n1_8 transforms read
  j+16*l; eight twiddle16 groups write k+8*j. Host-generated float twiddles;
  no GPU trig, DC removal or normalization. GPU fixture transforms contiguous;
  this tests arithmetic/within-transform layout, not cross-axis 2D dispatch.
- **Group128: 512 transforms / 65536 complex outputs; group6: 516 / 66048;
  group2: 512 / 65536. All max=0, RMSE=0, mismatches=0**, selected graph2/fused0.
  Total 197120 complex outputs. Groups6/2 match main/tail batch sizes of the
  native 22-thread inverse plan; not a full 2D parity claim.
- Final build: validation/fft_second_axis_strict_build.log (completed exit0).
  Shader: validation/fft_second_axis_ordinary.spv. Comparison logs:
  fft_second_axis_ordinary_comparison.log, fft_second_axis_group6_comparison.log,
  fft_second_axis_group2_comparison.log under validation/ (each passed).
  Earlier size8/16/128 selected-path regressions also pass with the extended
  shader: validation/fft_second_axis_leaf_regression.log. No Vulkan validation
  errors found in successful runs.
  Oracle dirs: fft_second_axis_inplace_639277442984315744,
  fft_second_axis_group6_639277444124781670,
  fft_second_axis_group2_639277444365317727 under validation/.
- Earlier initial/out-of-place/FMA-graph failures are preserved as diagnostics,
  not parity passes. Existing staged/unstaged/untracked work and VkFFT preserved.
- Next: compose both proven 128-point axes into a full GPU 128x128 inverse,
  with explicit column/row dispatch and normalization. Compare arbitrary complex
  fixtures and real MTF/adaptation stage2 snapshots against the proven native
  22-thread Octave plan. Forward FFT/log/exp/subnormal issues remain afterward.

## Previous session: composed 128-point inverse matches on Vulkan

- Extended the existing test-only generator, CPU oracle and Vulkan harness to
  size 128; production FFT code and settings remain unchanged.
- Preserved original FFTW `t2_8.c` in `tests/fftw_reference/` with its existing
  GPL attribution/COPYING. Generator translates both original graph variants.
- Important layout: first eight decimated n1_16 transforms read `j+8*l`, store
  `j+8*k`; sixteen t2_8 groups then write `k+16*j`. Twiddle multiplication is
  before the radix-8 butterfly. Compact twiddle powers are **1,3,7**, not 1,2,4.
  Host double trig is rounded once to float32 and embedded in the test shader;
  no GPU sin/cos. Inverse uses real/imag swap, with no normalization/DC removal.
- Actual local single FFTW DLL prints 128-point plan `dft-ct-dit/8`,
  `t2_8`, `n1_16`, matching the native first inverse axis decomposition.
- **512 composed transforms / 65536 complex outputs: max=0, RMSE=0,
  mismatches=0**, selected graph=1/fused=0. Size-8/16 regressions also pass;
  combined total 77824 complex outputs / 155648 float components. Strict test
  now requires the selected path for all three sizes, not any matching variant.
  Fixtures retain constant/impulse/ramp and seeded complex random values over
  four scales. This establishes current fixture parity, not arbitrary inputs.
- Completed build log `validation/fft_inverse128_build_reviewed.log` (exit 0).
  Initial sandbox build hit the known MSBuild FileTracker error; reviewed
  escalation succeeded. No machine script policy changes.
- Oracle directory: `validation/fft_inverse128_reference_639277431765714130`.
  Shader: `validation/fft_inverse128_initial.spv`; comparison:
  `validation/fft_inverse128_initial_comparison.log` (exit 0). No Vulkan
  validation errors found. See `tests/fftw_reference/README.md` for reproduction.
- Existing dirty/staged/untracked changes and VkFFT preserved; no commits.
- Exact next step: isolate buffered `t3bv_16_avx2_128` plus `n1_8` second-axis
  arithmetic, including batch tails; then compose 128x128 inverse with explicit
  axis layout and normalization and compare with proven native 22-thread plan.
  1D success does not yet validate full inverse, forward FFT, or full metric.

## Previous session: isolated inverse codelets match on Vulkan

- Added test-only `IQM-fft-codelet-test`, `tests/fft_codelet_gpu_test.cpp`,
  `tests/generate_fft_codelet_shader.ps1`, and the `--codelets` mode of
  `IQM-fftw-plan-test`. No production FFT/shader arithmetic was changed.
- CPU expected values come from the actual installed Octave single FFTW DLL,
  with printed inverse plans `n1_8` and `n1_16` (one-thread estimate/unaligned).
  These isolate leaf routines; they do not change the native 22-thread 2D oracle.
- Selected GPU graph=1/fused=0 matches exactly: 512 transforms per size,
  4096 + 8192 = **12288 complex outputs / 24576 float components**, max=0,
  RMSE=0, mismatches=0. Fixtures include constant, impulse, ramp and seeded
  complex random inputs at scales 1e-3, 1, 10 and 1e4. Inverses are unnormalized.
- Initial radix-16 translation had 257 differing float components. Explicit
  separate `precise` products in graph-1 product pairs eliminated them.
  Other arithmetic variants are diagnostic only; strict success requires the
  selected graph=1/fused=0 for both sizes. No universal input-equivalence claim.
- Strict completed build: `validation/fft_codelet_strict_build.log`; completed
  run (exit 0): `validation/fft_codelet_strict_comparison.log`. No Vulkan
  validation errors found. Oracle: `validation/fft_codelet_reference_639277418751685599`;
  final SPIR-V: `validation/fft_codelet_products_639277420167425265/fft_codelet_test.spv`.
  Earlier failed comparison logs are preserved and are not parity passes.
- Official FFTW 3.3.10 archive SHA256:
  `56C932549852CDDCFAFDAB3820B0200C7742675BE92179E59E6215B340E26467`.
  Original n1_8.c/n1_16.c and COPYING are preserved in `tests/fftw_reference/`;
  generated shader carries attribution and GPL-2.0-or-later. These remain
  test-only, not integrated into production. See its README for reproduction.
- Windows generator required an approved process-only ExecutionPolicy Bypass;
  no machine policy changed. Existing staged/unstaged files and VkFFT preserved.
- Next: validate `t2_8` twiddles and stride/layout composition with `n1_16`
  for the first inverse 128-point axis, then buffered `t3bv_16_avx2_128` plus
  `n1_8` for the second axis and final normalization. Compare intermediates
  against the proven native plan before integrating. Full FFT/metric parity
  remains unresolved; forward R2C, log/exp and subnormals still need work.

## Previous session: exact native FFTW plan identified

- Production shader/FFT arithmetic was not changed in this session. Added only
  diagnostic tools, a Windows-only CMake target, documentation and inspection
  copies of official source under `validation/`. No commits or cleanup.
- **Correction to earlier thread notes:** `fftw('threads') == 3` reports the
  double planner. Installed Octave 11.3.0's native single planner actually uses
  **22 threads on this host**. Query via the native probe, not that getter.
  Do not silently set the reference to 3; it changes single codelet selection.
- Confirmed exact release source: `validation/oct-fftw-release-11-3-0.cc`,
  SHA256 `DB730C34E2C6DDB5FB078D569FA8A6A259903FA14DE278B8374668658B7545B6`.
  Default-branch inspection copy had the same hash. Source URL is recorded in
  `tests/FFTW_PLAN_VALIDATION.md`. Official C:/HDR-VDP-3_Ref was untouched.
- Added `tests/inspect_fftw_native.cc`: builds as an Octave extension, calls
  native public single FFT wrappers, prints their exact plans and thread count.
  `tests/validate_native_fft_probe.m` verifies returned results equal ordinary
  fft2/ifft2 before accepting plans. All 8 asymmetric MTF/adaptation transforms
  passed (131072 complex values). Probe binary/plans:
  `validation/octave_plan_probe_639277410904713098/`; log
  `validation/fftw_native_probe_threads.log`.
- Added `tests/inspect_fftw_plan.cpp` / target `IQM-fftw-plan-test`: independent
  diagnostic loading the same local FFTW DLL. Mirrors native R2C offset 8064
  complex values, compact output/Hermitian reconstruction, and normalized C2C
  inverse; production remains Vulkan (no CPU fallback introduced).
- With 22-thread estimate/unaligned planning, **all 3 patterns, 6 images,
  forward+inverse = 196608 complex values match exactly** (max=0, RMSE=0,
  mismatches=0). `validate_fftw_plan.m --strict-native` asserts equality for
  the same native candidate on both images and directions.
- Logs: `validation/fftw_true_single_strict.log` (asymmetric),
  `validation/fftw_native_diagonal_comparison.log`,
  `validation/fftw_native_impulse_comparison.log`.
  Result directories: `fftw_true_single_639277411128626625`,
  `fftw_native_diagonal_639277412003115239`,
  `fftw_native_impulse_639277412261117616` under validation/.
- Native forward: r2cf_128 -> buffered t3fv_8_avx2_128 + n2fv_16_avx2_128;
  operation counts (adds,muls,FMAs)=(140658,38106,19584).
  Native inverse: t2_8 + n1_16 -> buffered t3bv_16_avx2_128 + n1_8;
  counts=(408576,108544,47104). Thread partitions affect tail codelet selection;
  GPU must mirror arithmetic/batch-tail choices, not run 22 CPU threads.
- Earlier 3/1-thread experiments are preserved but are not forward parity
  passes. Their residual differences on rows 64/66 were not a Hermitian bug;
  correct thread-dependent plan selection eliminated them. Importing wisdom
  alone also did not establish a matching plan (all cache-key probes failed).
- Diagnostic runtime notes: Windows LoadLibraryEx requires the correctly
  formed backslash absolute path here. FFTW sprint_plan text uses ordinary
  malloc/free, not FFTW's aligned allocator. Early failed probe directories
  have incomplete artifacts and are not validation results.
- Full Vulkan preprocessing parity remains unresolved; prior GPU error
  measurements below are unchanged. Next: isolated inverse radix-8/16 GPU
  arithmetic tests against the now-proven native plan.

## Previous session: FFT ordering and twiddle investigation

- Added `tests/diagnose_fft_order.m`. Local Octave reports estimate planner
  and 3 double threads; actual single=22 (see correction above).
  FFTW wisdom reports 3.3.10/AVX2 codelets.
- Current row-first GPU forward spectrum is much closer to Octave row-first
  sequential FFT than to official fft2. Asymmetric reference spectrum:
  GPU vs real fft2 max=0.007682190371777677; vs row-first max=4.464988674438334e-5.
- Important diagnostic correction: `fft2(real_array)` and
  `fft2(complex_array_with_zero_imaginary_values)` are different numerical
  paths. The full reference uses real input. Stage diagnostic now uses
  `fft2(real(gpu0))`, and separately reports complex-input comparison.
- Added optional `LocalAdaptFFTInput.columnFirst` and harness
  `IQM_FFT_AXIS_ORDER=column` (default `row`). Prepare stores transposed padded
  data. VkFFT inverse reverses the forward traversal; transpose filtered data
  back using transfer-only element copies before inverse, then finish reads
  row-major. Radial square filter is unchanged. Prepare/finish push constants
  now have four uint32 values (16 bytes).
- Shader binaries rebuilt individually and previous binaries backed up in
  `validation/fft_shaders_before_axis_*`. No blanket shader rebuild.
- Initial prepare/finish-only transpose worsened the full chain; preserve its
  `validation/fft_column_*` artifacts as a rejected experiment. Both-direction
  alignment (`fft_aligned_*`) improved asymmetric full MTF maxima from
  3.814697265625e-6/3.814697265625e-6 to
  2.86102294921875e-6/1.9073486328125e-6 (ref/test). JND mismatches 1171/882
  fell to 444/433. Maximum JND error remained 0.000244140625.
- Added `IQM_FFT_TWIDDLES=lut` / `LocalAdaptFFTInput.useTwiddleLUT` (default
  `native` / false, VkFFT useLUT=-1). A reused plan rejects twiddle mode changes.
- LUT exposed a latent null-fence bug: VkFFT was given a pointer to a null
  fence handle. LUT upload submits/waits/resets this fence. Fixed with an owned
  RAII fence that outlives the plan. Do not edit `lib/VkFFT` to work around it.
- Successful asymmetric LUT run: `spyr_spatial_17921430489879733`.
  Forward MTF spectrum max vs native real fft2 = 2.750817318316642e-5 (ref),
  3.075507258719844e-5 (test). Full MTF maxima unchanged vs aligned-native,
  JND mismatches 434/411, max JND error still 0.000244140625.
- Build completed successfully. Post-fix LUT GPU runs had no validation errors.
  All three patterns (six images) passed identical-map native JND/13-stage
  trace checks and identical-JND SPYR checks after the fence fix.
- Full-chain maxima for column-aligned + LUT: diagonal retinal max
  2.86102294921875e-6/1.9073486328125e-6, impulse retinal max
  1.9073486328125e-6/1.9073486328125e-6 (ref/test). All six full reference
  pupil coefficients now match. All six full JND maxima remain
  0.000244140625 (not exact parity). Full JND mismatches are asymmetric
  434/411, diagonal 470/374, impulse 460/389.
- Existing default/non-LUT FFT integration regression passed its old tolerance
  after the fence fix, unchanged max=0.00016022, RMSE=0.00002903.
  Evidence: `validation/fft_lut_postfix_legacy_integration.log`.
- Invalid artifacts: the first `fft_lut_asymmetric_*` ran an old executable
  before link completed, causing LNK1104; subsequent `fft_lut_verified_*`
  exposed the null-fence crash, so neither is a numerical LUT pass. See
  `validation/fft_lut_asymmetric_INVALID.md`. Valid post-fix prefix is
  `validation/fft_lut_fixed_*`.
- Always wait for a successful completed build before launching IQM. For a LUT
  run verify `fft_twiddles.txt` actually contains `lut`; do not compare an old
  executable's output or an incomplete/crashed run directory.
- No default settings changed: preprocessing=none, FFT axes=row, twiddles=native.
  Full exact preprocessing parity remains unresolved.

## Commands and tools

PowerShell, workspace root. CMake may require sandbox escalation due to MSBuild
FileTracker E_ACCESSDENIED. Use the normal approval tool flow when necessary.

```powershell
glslc shaders/hdrvdp3/hdrvdp3_fft_prepare.glsl -o shaders/hdrvdp3/hdrvdp3_fft_prepare.spv --target-env=vulkan1.2
glslc shaders/hdrvdp3/hdrvdp3_fft_finish.glsl -o shaders/hdrvdp3/hdrvdp3_fft_finish.spv --target-env=vulkan1.2
cmake --build build-vs2026 --config Release --target IQM IQM-jnd-single-test
$env:IQM_PREPROCESSING='fft'
$env:IQM_SPYR_PATTERN='asymmetric' # diagonal, asymmetric, impulse
$env:IQM_FFT_AXIS_ORDER='column' # row is the baseline/default
$env:IQM_FFT_TWIDDLES='lut' # native is the baseline/default
& './build-vs2026/bin/Release/IQM.exe' --method HDRVDP3 --enable-validation
# Use this run's printed export directory, not a guessed old reference:
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/diagnose_fft_stages.m '<directory>'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_fft_preprocessing.m '<directory>' --strict
& './build-vs2026/bin/Release/IQM-jnd-single-test.exe' '<directory>'
& 'C:/Program Files/GNU Octave/Octave-11.3.0/mingw64/bin/octave-cli.exe' --quiet --no-gui tests/validate_spyr_spatial.m '<directory>'
```

The full-chain script without `--strict` reports mismatches (exit 0 is NOT
full parity). Its shared-input native checks still assert equality. Octave
often prints `ignoring const execution_exception& while preparing to exit`
even with successful exit code 0; check actual assertions/status.

## Remaining project work

Roadmap review (2026-10-17): FFT passes are isolated same-input checks, not
full metric parity. Implement quality/Q/Q_JOD end to end first; add other
reference task modes only if included in product scope. Native pupil/JND and
CSF/pooling currently include CPU work; an entirely Vulkan implementation
requires separately validating those shader ports after a trusted C++ baseline.
The 2023 paper is a high-level overview; pin executable behavior to official
3.0.7 options, precision and input/viewing parameters. Current CSF and masking
map resizing still explicitly use bilinear approximations to Octave imresize.

1. Source-backed reference FFT/math experiment now passes all current strict
   preprocessing/JND/SPYR checks. Windows binary dependency removed; broaden
   domains/devices without assuming universal equality to all libm builds.
   Keep current GPU experimental modes opt-in; no tolerance-based parity claims.
2. Native log/exp behavior and finite subnormal products match in the local
   probe; validate broader ranges/devices without assuming universal equivalence.
3. Direct CSF float outputs now pass1152 cases after age fix; double exactness
   still pending. Validate CSF LUT/maps, adaptation-map resizing (currently
   bilinear approximation to Octave imresize), masking, pooling and final scores.
4. Extend beyond current 64x64/30-ppd/age-24/D65 scope to other dimensions,
   luminance ranges, color/spectral parameters and real HDR images.

### 2026-10-25 follow-up: production JND HDR replay completed

production_cpu_jnd_replay_639285090794799166/replay/PASS.txt confirms every
stage and native Q/Q_JOD bits for HDR80x96/96x80, using --cpu-lut and actual
production JND source. Session37113 COMPLETED. Source snapshots, executable/
shader hashes and761 reference file hashes are in the root. Strict-double
negative control rejects one binary64 ULP while Float32 checks still pass.
This supersedes the running status in the checkpoint above.

Next production parity gap addressed: quality.cpp frequency coordinates now
preserve native create_cycdeg_image modulo/scale operation order. Direct DFT
helper delegates to the same coordinate function. New verify_frequency_grid.cpp
checks240 binary64 values against native10x12/12x10 grids (3.75ppd), PASS.
Experimental masking checker now calls actual production frequency function,
removing its private implementation. General public geometry still disabled.
Active root validation/production_frequency_639285092485366527; pointers saved.
Build session98332 for pathway-masking-test; CLI regression session80377 runs
11pairs against pre-existing goldens. Exact next step: poll both; after build
passes run masking checker against the completed HDR replay's GPU inputs, then
independent final score validation; finish hashes/docs. Earlier application
build executable was rebuilt for this new fix; use newroot/IQM-hdrvdp3.exe as
frozen executable for the frequency regression, and its recorded hashes.

Frequency follow-up: build98332 completed. HDR quality session62682 completed,
126 stage checks/393384 values and independent final Q/Q_JOD bit validation
PASS in production_frequency.../quality and /validated. CLI80377 stillrunning.
JND report fixed: GPU_adaptation uses argc>=7 (CPU-LUT mode has argc8), and
CPU_model is now recorded explicitly. Old replay metadata had false
GPU_adaptation=0 despite actually loading GPU adaptation; source/input checks
and64/471060 checks verified the actual path. Historical files left unchanged.
New production-linked asymmetric/impulse JND profiles running session85776.
Next: poll80377/85776; freeze final snapshots/hashes and completed pointers.

JND session85776 COMPLETED: asymmetric andimpulse actual production model,
64checks/471060 values each, GPU_retinal=1/GPU_adaptation=1/CPU_model=1.
Combined with HDR replay, production JND has all three contents/two orientations.
Do not claim six complete CPU-LUT replays: only HDR full replay used CPU-LUT;
other two latest checks are upstream JND component runs using prior GPU filters.

## Final checkpoint (2026-10-25): production arithmetic fixes accepted

All sessions above COMPLETED; none should be polled again.
- production_frequency_639285092485366527/PASS.txt:240 native double grid
  words exact; HDR quality126checks/393384values and independent scorebits
  PASS; asymmetric/impulse production JND64checks/471060values each PASS.
- CLI session80377 COMPLETED:11 PFM pairs,1166 exact numerical exports and
  exact quality files. cli_regression/PASS.txt and hashes.json saved. Frozen
  executable root/IQM-hdrvdp3.exe; checkpoint_hashes.json,source_snapshot/,
  jnd_profile_hashes.json and validation_scope.json record exact scope.
- production_cpu_jnd_replay_639285090794799166/replay/PASS.txt: production
  CPU JND model + CPU filter coefficients HDR end-to-end, both orientations,
  final scorebits identical.761 reference hashes and source/asset snapshots.
- Completed pointers: production_frequency_completed_path.txt and
  production_cpu_jnd_replay_completed_path.txt. Original bad-folder strict
  test, old CPU rod-slope failures and old artifacts remain preserved.
- Public guards unchanged. No commit/push/reset/clean; stagedcorrdn hash still
  03DCAC86ED49ADE71C804061C66BB5F382BC754B69B0BD9F0354B6D9024450C6.
  Dirty VkFFT and unrelated prior changes preserved. Targeted diff checks pass;
  whole-tree diff check still finds pre-existing whitespace in mtf_gpu_test.cpp.

Exact next implementation step: extract the accepted7-dispatch buffer-based
mixed filter runner from tests/verify_mixed_filter_chain.cpp into a reusable
runtime component that accepts original Float32 data and explicit forward/
inverse/IO shader assets, without native fixture reads. Validate its returned
MTF/LA buffers against the frozen80x96/96x80 asymmetric/impulse/HDR fixtures.
Keep evaluate_quality guards until mixed FFT,SPYR,residual120,pool/map contracts
are integrated and full six-pair API checks pass. Existing LocalAdaptFFT uses
image descriptors/combined reference ABI; do not route separate mixed-axis
shaders through that incompatible contract. Preserve column-major2H x2W layout,
MTF symmetric/LA replicate POST padding, exact30720 inverse normalization and
native DC ordering. No need to rerun already passing arithmetic tests unless
new integration changes affect them. Broader PPD/age/RGB/device coverage and
resident command-buffer transfer reduction remain separate work.

## Active continuation (2026-10-25): reusable mixed filter runtime

Accepted seven-dispatch implementation extracted into
include/IQM/hdrvdp3/mixed_filter.h and src/hdrvdp3/mixed_filter.cpp.
run_mixed_filter accepts abstract IQM::VulkanInstance, original Float32 vector,
80x96/96x80 dimensions, mode and explicit forward/inverse/IO shader assets.
Only shader files are read; native fixtures stay in checker. CPU model filter
creation and all dispatch/padding/layout/normalization orders preserved.
Trace optional, output vector always returned; synchronous exclusive command
buffer use. Public evaluate_quality guards unchanged. Added source to main
and bounded-evaluation libraries, strict FP compilation. No full API integration.
Checker now calls runtime, verifies actual runtime model words vs native, and
requires BOTH numeric and bit equality for all5 stages. First80x96 MTF ref
also verifies repeat call/nontrace output bit identity on same instance.
Fresh root validation/mixed_filter_runtime_1792912954173, active pointer saved.
Old checker source retained root/checker_before.cpp. Old binaries untouched.
24case matrix running session20209:3contents x2orientations x2modes x2images.
Fresh bounded library build session2049. Exact next step: poll both; inspect
all24PASS markers and validation logs; record results/hashes/scope, then use
this runtime checker in a fresh full CPU-LUT HDR replay before next extraction.
Python unavailable; initial attempted extraction made no changes. Node used.

Runtime follow-up: bounded library build2049 completed successfully. Matrix20209
passed asymmetric/impulse and is finishing HDR. All five stages now have zero
bit mismatches, including runtime-generated model coefficients; first MTF ref
per content confirms repeated same-instance call with trace disabled.
Reusable audit/runner tests/validate_mixed_filter_runtime.ps1 added; after20209
completes run it with -ResultsOnly over the root to gate all24 traces/logs.
Sources/snapshots/hashes saved. Fresh replay assets in root/replay_assets.json
select new runtime filter checker + latest production JND/masking checkers,
--cpu-lut mode. Run tests/run_actual_gpu_composition.ps1 -OutputDirectory
<root>/full_hdr_replay -AssetsManifest <root>/replay_assets.json aftermatrixPASS.

Runtime matrix20209 COMPLETED. -ResultsOnly audit passed all24cases,6082560
stage values and737280 model words bitwise, no Vulkan validation errors.
Full HDR CPU-LUT replay now running session51379 in root/full_hdr_replay using
new runtime filters, latest production JND and production-frequency masking.
Next: poll51379,inspect final independent Q/Q_JOD bits, snapshot all reused
binary/shader assets and preserve previous stage provenance. Documentation:
tests/MIXED_FILTER_RUNTIME_VALIDATION.md. root/PASS.txt currently certifies
matrix only; full_hdr_replay/PASS.txt must exist before claiming full replay.

## Latest completed checkpoint: mixed filter runtime (2026-10-25)

Sessions20209,2049,51379 are ALL COMPLETED. Do not poll them again.
Root validation/mixed_filter_runtime_1792912954173; completed pointer saved.
- New reusable mixed_filter.h/.cpp, integrated into both production CMake and
  bounded evaluation library. Original image vector and explicit shader paths
  are sufficient for filter execution; no native model/intermediate reads.
- Matrix24cases:6082560 stage values +737280 model coefficients bitwise exact;
  all5stages/24cases and three same-instance repeat/nontrace cases pass.
- Fresh bounded library compile PASS. Existing quality geometry guards unchanged.
- full_hdr_replay/PASS.txt: original images ->newruntimeMTF/LA ->production
  CPU-LUT JND ->accepted SPYR/pool/masking ->native Q/Q_JOD bit identity,
  two orientations PASS. No Vulkan validation errors in replay/matrix logs.
- source_snapshot/,source_hashes.json,runtime_asset_hashes.json,
  replay_asset_hashes.json, runtime_results.json, replay_assets.json preserved.
  Reused HDR reference hashes are explicitly linked via previous frozen manifest.
- Sources still match snapshot hashes. Staged corrdn and dirty VkFFT preserved;
  no commit/push/reset/clean. No blanket shader rebuild; only fresh build's
  required generated shader includes. No broad regression rerun needed because
  the new component is not called by guarded evaluate_quality yet.

Exact next step: extract tests/verify_spyr_lowpass.cpp --all-chain runtime into
reusable component accepting original GPU P pair in memory, explicit FMA/
separate/mean/difference/residual shader assets and fixed sp0 filter data.
Derive buffer sizes/dispatch geometry from80x96/96x80 dimensions, never native
expected vectors or manifest files. Keep expected-file reads/comparisons in
checker. The existing checker uses golden residual/base values for numerical
range guards (lines around79-80); runtime guards must use actual data instead,
without changing mean/DC/normalization ordering or claiming unchecked domains.
Validate8bands +mean/difference/residual against frozen sixpairs; then replace
SPYR replay stage with newcomponent before integrating pool/maps and finally
opening geometry guards. Preserve old checker/source/binaries and use fresh
run roots. Current mixed runtime remains synchronous host-vector interface,
not GPU-buffer-resident cross-stage integration. Broaderage/ppd/RGB/devices open.

## Active continuation: SPYR runtime (2026-10-25)

New pathway_spyr.h/.cpp exposes run_pathway_spyr with in-memory Float32 P pair,
explicit fixed sp0 filter words and FMA/separate/mean/difference/residual shaders.
No native fixture reads; geometry derives only from80x96/96x80 dimensions.
Eight SPYR dispatch stages retain both FMA/separate variants; mean/DC/residual
order unchanged. Actual GPU base checked BEFORE mean with exponent bounds
[-90,100],spread<=16; actual FFT complex words checked for finite/norm range
before return. Two synchronized submissions, CPU reads for validation only;
not a resident pipeline and not a no-host-transfer claim. Previous checker kept.
New tests/verify_spyr_runtime.cpp uses same --all-chain CLI as replay, independent
native expected reads/comparisons. Six pairs pass113526values/content,340578total;
all66 stage rows have zero bit errors (including signedzero),no Vulkan errors.
Historic checker acceptance policy retained; aggregate observed exactbits recorded.
Fresh root validation/spyr_runtime_1792913354201; active pointer saved.
Initial Node CMake template error affected target append only; corrected before
build. No invalid GPU runs: source review fixed split location before testing.
Build55447 and test9649 COMPLETED. Full HDR replay now session92468, root/
full_hdr_replay,using reusable mixedfilter and newSPYR with production JND/
frequency masking. Fresh bounded library configure succeeded; build next.
Exact next step: build application_build target iqm_bounded_evaluation,poll92468,
validate independent finalscorebits,freeze source/binary/assets,update docs.
Public quality geometry guards unchanged; no commit/push/reset/clean.

SPYR build36657 COMPLETED (bounded library PASS). Durable
validate_spyr_runtime.ps1 -ResultsOnly also PASS with strict bit checks onall66
rows/340578values; source checker retains historic acceptance but audit tightens
observed matrix tobit identity. Docs SPYR_RUNTIME_VALIDATION.md. Full HDR92468
still running. No overall API integration or broaderdomain/device claim.

## Latest completed checkpoint: SPYR runtime (2026-10-25)

All SPYR sessions55447/9649/36657/92468 COMPLETED; do not poll again.
Root validation/spyr_runtime_1792913354201; completed pointer saved.
- New run_pathway_spyr runtime: in-memory pair + fixed filter words + shaders;
  no native fixture/model reads. Eight bands +mean/DC/difference/residual stages.
- Actual GPU base range checks before mean;actual residual range check before
  return. Two synchronized submissions; host validation only, not residentGPU.
- Sixpaired component chains,66rows/340578values observed bitwise exact; durable
  validate_spyr_runtime.ps1 additionally rejects any bit error in these records.
- Fresh bounded evaluation library PASS. full_hdr_replay/PASS: both extracted
  mixed filter and SPYR runtimes +production JND/frequency helpers ->native
  Q/Q_JOD bit identity for HDR80x96/96x80. All22replaySPYRrows zero bit errors.
  No Vulkan validation errors. Public quality guards remain unchanged.
- Source snapshots,source_hashes.json,asset_hashes.json,audit_hashes.json,
  runtime_results.json,replay_assets.json and full replay assets retained.
  Old SPYR checker remains untouched; source snapshot checker_before.cpp kept.
- No commit/push/reset/clean; dirtyVkFFT/stagedcorrdn/unrelatedchanges preserved.
  Corrdn SHA256 still03DCAC86ED49ADE71C804061C66BB5F382BC754B69B0BD9F0354B6D9024450C6.

Exact next implementation step: extract tests/verify_pathway_pool.cpp compute
path into reusable in-memory resize/CSF/pooling runtime. Inputs must be actual
SPYR band outputs and GPU log adaptation data; expected/native files only in
checker. Derive resize geometry/dispatch sizes from dimensions; retain accepted
integer coordinate arithmetic and FMA/separate contracts. Validate all six
content/orientation pairs,then use component in fresh full HDR replay with
both extracted filters/SPYR. Only after downstream comparison/masking/base-noise
integration and full six-pair API checks should evaluate_quality geometry guards
be opened. Preserve fixed model defaults and actual-data domain checks; do not
substitute tolerance for exact parity. Broader PPD/age/RGB/devices remain open.

## Active continuation: pooling runtime (2026-10-25)

Inspection clarified verify_pathway_pool.cpp covers mutual masking/pooling
ONLY. Resize/CSF already use production MapGPU; do not claim newly extracted
resize/CSF code. New pathway_pool.h/.cpp exposes run_pathway_pool on abstract
instance,in-memory normalized/DC-processed band pair and explicit FMA/separate
shaders. Returns raw mutual mask and3x3 pooled vector. Native reads stay solely
in new verify_pool_runtime.cpp; old checker unchanged and snapshot retained.
Geometry,source-axis tap FMA selection,zero-edge convolution,and ordering
preserved. API accepts eight validated pyramid band geometries,finite Float32
pairs. Public evaluation guards remain unchanged. Main and bounded-library
CMake includes source under strict FP. Runtime poolchecker requires bit equality.
Fresh root validation/pool_runtime_1792913701963; active pointer saved.
Session79007 runs3contents x10bands using frozen GPU-serialized band inputs
(replay_cpu_filters/bands/pool.txt); then automatically full_hdr_replay with
latest extracted mixedfilter/SPYR/pool +production JND/frequency checkers.
Fresh bounded library configure succeeded; build application_build next.
Exact next step: poll79007,build iqm_bounded_evaluation,inspect all30bandrows
and final native scorebits; freeze sources/assets and record actual scope.

Pooling follow-up: matrix all3contents completed (30bands/214560raw+pooled words,
zero bit errors and Vulkan errors). validate_pool_runtime.ps1 -ResultsOnly PASS.
Fresh bounded library build70357 COMPLETED successfully. Full HDR session79007
still running,root/full_hdr_replay. Source snapshots,source_hashes.json,
asset_hashes.json,audit_hashes.json and runtime_results.json saved. Docs
POOL_RUNTIME_VALIDATION.md precisely distinguish pooling from existing MapGPU
resize/CSF. Next: poll79007,inspect native Q/Q_JOD bit validation and all10pool
rows in newreplay,then freeze final checkpoint and plan comparison/masking
runtime extraction. Public guard unchanged; no extra broad regression required
until new components are actually wired into evaluate_quality.

## Latest completed checkpoint: pooling runtime (2026-10-25)

Sessions79007 and70357 COMPLETED; do not poll again.
Root validation/pool_runtime_1792913701963;completed pointer saved.
- New run_pathway_pool in pathway_pool.h/.cpp,strict-FP production and bounded
  library source. Original per-band vectors +explicit shaders only; no native
  model/intermediate reads. Raw mutual mask and3x3 pooled vector returned.
- Threecontents/twoorientations/fivebands:30cases,214560words bitwise exact,
  no Vulkan errors. Each10-bandrun reuses sameinstance. Durable audit PASS.
  Inputs from previous frozen GPU-serialized bands,precise paths/hashes saved.
- Fresh bounded library PASS. FullHDR with extracted filters/SPYR/pool PASS:
 10poolrows bitwise exact,126maskingchecks/393384values zero numerical errors,
 independent native final Q/Q_JOD words identical bothorientations. This rerun
 overlaps previous content scope; do not add counts as new unique coverage.
- Resize/CSF already production MapGPU; no newly extracted resize/CSF claim.
  Host vectors/file serialization between stages remain. Public guards unchanged.
- Source/asset snapshots/hashes,audit hashes,runtime_results.json,full replay
  assets preserved. Old checker/native references preserved. No commit/push/
  reset/clean; stagedcorrdn anddirtyVkFFT unchanged.

Exact next implementation step: extract tests/verify_general_comparison.cpp
into reusable comparison-mask runtime taking original reference/test luminance
vectors plus source/target dimensions and explicit shader, returning binary
mask and exact double resized mask. Preserve Q20 axis-plan checks: unit
normalization,finite/integer weights,abs tap weight bound and absolute sum<=2;
never infer input/output sizes from expected files. Native expected reads stay
in checker; validate all90cases per content before replacing replay comparison
stage. Then extract remaining masking/base-noise composition using existing
MapGPU andproduction frequency/CSF/quality helpers. After full six-pair in-memory
API acceptance, wire evaluate_quality and open only validated geometry guards.
Broader PPD/age/RGB/devices and GPU residency remain separate.

## Active continuation: comparison mask runtime (2026-10-25)

New pathway_comparison.h/.cpp exposes run_pathway_comparison: original finite
Float32 reference/test vectors,source/target dimensions and explicitshader;
returns binary mask and double resized mask. Positive reference required;
finite test may include zero/negative as original contract. Source80x96/96x80,
uniform1/2/4/8 downsample only. All Q20 plan guards and3dispatch Q20/Q40 order
preserved. Native expected/model files remain only in checker; CPU makes axis
plans,not image intermediates. Synchronous exclusive instance use.
Main/bounded CMake and new comparison-runtime-test use strict FP. Old checker
untouched;snapshot saved. Build55047 COMPLETED. Matrix script
validate_comparison_runtime.ps1 session75938 runs90cases percontent withbit
checks and Vulkan error scan. Fresh root comparison_runtime_1792913994936,
active pointer saved;matrix underroot/matrix. Asymmetric completed PASS.
Fresh bounded library configured; build application_build next. After matrix
PASS,set replay_assets.json comparisonChecker to newexe (base previouspool
runtime assets),run fresh full_hdr_replay. Then inspect finalscorebits/freeze.
No public guard changes orcommit/push/reset/clean.

Comparison matrix75938 COMPLETED:270cases,2073600binarywords and965520double
words bitwise exact,no Vulkan errors. Bounded library94614 COMPLETED PASS.
FullHDR session30765 running. Source/asset snapshots saved. No public geometry
change. Next poll30765 and independently inspect resized-double masks/final
scorebits before concluding this extraction.

Comparison continuation COMPLETED:75938/94614/30765 all complete. Matrix270
cases bitwise;fullHDR independent finalQ/Q_JOD bit identity passed. Completed
pointer comparison_runtime_completed_path.txt saved.150 fixed masking scalars
are invariant across3contents/2orientations (audit saved),neural excluded.

## Active continuation: remaining masking/quality runtime

New pathway_masking.h/.cpp accepts in-memory5bandpairs,self pooled maps,double
comparison masks,original GPU log adaptation andinteger GPU residual selection.
Default p/pf/q/self/neighbor scales computed from same production pow10 constants,
not read native files. Production MapGPU handles resize/CSF/neighbor/D/error/
quality; existing explicit product/base shaders retained; existing CPU double
NCSF uses GPU adaptation/frequency. Actual base exponent/range checks retained.
New verify_masking_runtime.cpp full126check profile reads GPU outputs as inputs
andnative data asgoldens only; requires defaultmodel doublebit identity,base
noise doublebits and finalscorebits. Oldchecker left untouched.
Root validation/masking_runtime_1792914241203;activepointer saved. Build3045
completing;next run3content component profiles against frozen GPU replay inputs,
then freshfullHDR using all extracted runtimes,buildboundedlibrary andfreeze.
Comparison root source CMake snapshots predate this added source; preserve
original hashes,do not overwrite them to pretend older build used newCMake.
Public guards unchanged;no commit/push/reset/clean.

Masking build3045 COMPLETED; fresh bounded library80294 COMPLETED PASS.
Component session75780:asymmetric/impulse PASS sofar,each126checks/393384values;
asymmetric all126bit-error counters zero. C++ defaultmodel doublebits also match
native for allchecked scalarfields (not native-provided runtime parameters).
validate_masking_runtime.ps1 added; after75780 completes run -ResultsOnly to
strictlyaudit all378rows/1180152values,then start full_hdr_replay with root/
replay_assets.json selecting newruntime masking andallprior extracted stages.
Existing checker/nativegoldens preserved. No arbitrarydomain/device claim.

Masking component75780 COMPLETED:all3contents pass. Durable strict audit PASS,
378rows/1180152values zero bit errors;150 defaultmodel double checks also pass.
Fresh fullHDR session26517 running with ALL extracted runtimes (mixedfilter,
SPYR,pool,comparison,masking) andproduction JND/frequency code. Upstream through
comparison completed;quality pending. Boundedlibrary80294 already PASS.
Exact next step: poll26517,independent finalscorebits and126qualityrows strict
bit audit,freezecompletedpointers/docs. Next project phase is a single original-
image-to-score in-memory API wiring thesecomponents,then sixpair acceptance.

## Latest completed checkpoint (2026-10-25): comparison +masking runtime

All current sessions55047/75938/94614/30765/3045/75780/80294/26517 COMPLETED.
Do not poll again. Completed pointers: comparison_runtime_completed_path.txt
andmasking_runtime_completed_path.txt. Roots comparison_runtime_1792913994936
andmasking_runtime_1792914241203.
- Comparison270cases,2073600binary+965520doublewords bitwise exact;freshHDR
  final native scorebit identity PASS. Q20/Q40 guards preserved. BoundedlibPASS.
- Masking378checks/1180152values acrosssixpairs strictbitaudit PASS;150 repeated
  defaultmodel scalars binary64 exact. CPU pow10 model factory uses existing
  production constants. ActualGPU log/CSF/selection ->CPUdoubleNCSF neural;
  no native image-dependent intermediates in runtime. BoundedlibPASS.
- Final fullHDR replay in maskingroot:ALLextracted components,independent
  native Q/Q_JOD bits identical bothorientations.126qualityrows zero biterrors;
  no Vulkan validation errors. Still file/host composed between components.
- Source/asset/audit hashes,snapshots,replay manifests andhistorical goldens
  preserved. EarliercomparisonCMake snapshot predates addedmaskingsource;
  do not replace historic hash with currentCMake. Actualalgorithm source matches.
- Public evaluate_quality guards unchanged. No commit/push/reset/clean; staged
  corrdn SHA25603DCAC86ED49ADE71C804061C66BB5F382BC754B69B0BD9F0354B6D9024450C6
  anddirtyVkFFT preserved. Only freshbuild requiredshaders generated.

Exact next implementation step: add isolated evaluate_pathway_quality entry
point that wires original positiveFloat32 pair ->run_mixed_filter(MTF each)
->MapGPU.adaptation_input(D65_AGE24_LMR) ->run_mixed_filter(LA each)
->pupil_trace(area=w*h/900,age24) ->jnd_response(create_jnd_lut(),trace[3],D65)
->P=response[i].response;mean_adaptation(LAref,LAtest) ->log_adaptation.
Pass Ppair to run_pathway_spyr with explicit fixedsp0 filter words/shaders.
Unpack bands from SPYRstages4,5,6,7 (paired n+n),base stage8 (ref1..120,
 test123..242 in1-basedterms; zero-based ref offset1,test122). Runpool perband;
comparison original ref/test to target[w,w,w/2,w/4,w/8]x[h,h,h/2,h/4,h/8].
GPU selectedindex is floatword360 in SPYRstage10 (361words); validatefinite,
integer0..119 then call run_pathway_masking with vectors/log/index. Return
QualityScores plus optionaldiagnostictrace. Allassets explicit,no environment,
no intermediate files,no native dynamicdata. Use abstractVulkanInstance with
exclusive synchronouscommandbuffer ownership; stillCPUdoubleNCSF/hostvectors.
Validate sixoriginal pairs (asym/impulse/HDR x2orientations),independent native
P/log/bands/masks/baseNoise/finalscore bits,then integrateboundedpublic API/CLI
andonlythen open validated80x96/96x80 guards. Currentfilecomposition success
mustnot be presented as this single API acceptance. BroadPPD/age/RGB/devices
andGPU-resident commandgraph remain separate work.

## Active continuation: single in-memory quality API (2026-10-25)

New pathway_quality.h/.cpp implements evaluate_pathway_quality on original
positive Float32 pair,explicit shader/model assets andoptional diagnostictrace.
Wires MTF/adaptation/JND/log ->SPYR/DC/residual ->pool/comparison ->masking/
baseNoise/finalQuality through host vectors,NO intermediate files/environment.
Fixed30ppd age24 D6580x96/96x80;CPU model/LUT,doubleNCSF retained. Publicbounded
API guard unchanged. Trace is committed only after a successful full call.
New verify_pathway_quality.cpp independently compares81stages per originalpair
against native retinal/adaptation/P/log/SPYR/masks/baseNoise/scores. Firstpair
also repeats score-only call on sameinstance (82checks). Expected327735words
perpair,+2 repeatwords. Native inputonlyoriginalimages+fixedsp0 modelbeforecall;
all image-dependent goldens are read after API returns. No proxy filepipeline.
Fresh root validation/pathway_quality_1792914761769;activepointer saved.
Build28013 running. tests/validate_pathway_quality.ps1 runs sixpairs with strict
bit and Vulkan log audit,configs saved explicitly. Exact nextstep: pollbuild,
run script with fresh <root>/matrix; diagnose any mismatchwithouttolerance.
Then freeze source/assets,validate aggregate,preserve oldartifacts,update docs.
Do not claim public CLI integration/generalgeometry based solely on new API.

### Report request interruption (2026-10-25)

User requested weekly English report in supplied format; next week midterms,
no committed project work,optional continuation if time permits. Report must
NOT claim single in-memory API validation passed. Build28013 completed PASS,
but six-pair matrix has NOT started: validate_pathway_quality.ps1 parser found
invalid comma-list splat $config,$manifest,@originals in Get-FileHash arguments.
Exact next action: replace with explicit array combination (@($config,$manifest)
+ $originals),parse script again,then fresh matrix run. No GPU numerical test
failed; no matrix output directory created by the rejected parse. Existing
component/full file-composition results remain accepted. No commit/push.

### Resumed in-memory API validation (2026-10-25)

User asks continue +whether successful work was pushed. Answer:NO recent
commit/push;read-only git ls-remote verified feature/hdr-vdp3 still at
777e6b6cdacca6e089fbeae911d01b61aa34b455. User did not instruct a new push.
Fixed PowerShell comma-list splat using explicit array concatenation. Parser
passes;matrix session30157 now running. Firstthreepairs passed (asymmetric
bothorientations,impulse80x96). Firstasymmetric pair82checks/327737words incl
same-instance score-only repeat;otherpairs81checks/327735words. No tolerance.
New independent contract test13invalid inputs,NoGPU instance prevents anyGPU
access,and trace sentinel preserved. Build72004 completed;contract.log PASS.
Contract target added to boundedCMake after numerical executable build; only
new target compiled,shared numerical library source/flags unchanged.
Next: poll30157 to complete sixpairs,inspect everybitcounter/Vulkanlog/final
nativeQ/Q_JOD,freeze source/assets andcompletedpointer,document API scope.
No commit/push/reset/clean. User's GitHub question is not a push authorization.

Matrix30157 progress:firstfivepairs PASS,including82checks firstpair trace-off
repeat and81each others. Last HDR96x80 running. Shader/model/module-source/
binary hashes saved;fixedsp0file hashes match across3contents. Contract13cases
PASS. Documentation PATHWAY_QUALITY_API_VALIDATION.md drafted;add finalscope
only after sixpairaggregate PASS. GitHubremote verified777e6b6;no newpush.

## Latest completed checkpoint (2026-10-25): single in-memory API accepted

Matrix30157 andcontractbuild72004 COMPLETED;do not poll them again. Root
validation/pathway_quality_1792914761769,completed pointer saved.
- evaluate_pathway_quality links all accepted stages on originalFloat32 pair
  through vectors,NO intermediatefile IO orenvironment. Fixed80x96/96x80,
 30ppd age24 D65 sp0;CPUmodel/LUT,doubleNCSF andhosttransfers remain.
- Sixpairs completed:487checks/1966412words bitwise exact. This includes81
 stages/pair plus1extra score-only repeat check(2words) onfirstasym80x96.
 Same-instance secondcallwithouttrace returns identicalnativeQ/Q_JOD.
- Strict -ResultsOnly aggregate audit rerun PASS (noGPU repetition). NoVulkan
 validationerrors. Contract13invalid inputs rejected beforeGPUaccess,output
 trace untouched. API commits trace only after fullsuccessfulcall.
- Native image-dependent goldens read AFTER call. Runtime reads onlyexplicit
 shaders;originalimages +fixedsp0 model are callerprovided. Allmodel files
 samehash acrosscontents. Originalinput/config/exe/source/shader/model hashes
 andsnapshotssaved. CMake contracttarget addedafter numericalexebuild;newtarget
 only,common numericalsources/flags unchanged. No oldreference deletion.
- Public evaluate_quality/CLI guards unchanged. No commit/push/reset/clean;
 remote feature/hdr-vdp3 read-onlyverified777e6b6. Userasked whetherpushed,not
 instructednewpush. Successful work remainslocal;stagedcorrdn/dirtyVkFFT kept.
- Docs PATHWAY_QUALITY_API_VALIDATION.md andACTUAL_GPU_COMPOSITION_VALIDATION.md
 updated. WeeklyreportpreviouslysaidAPIvalidationpending;that was accurate
 then,butnowthischeckpoint supersedes it.

Exact next step: package a fresh explicit runtime manifest for newvalidated
80x96/96x80 profile,including separateorientation forward/inverse FFT assets,
mixedIO,sp0 fixedwordmodels,FMA/separate SPYR/pool,mean/difference/residual,
comparison,mapABI13,product/base assets. Current legacy combinedFFT/math-dir
configuration cannot represent this graph;do not merely loosen oldguards.
Add manifest loader/validation and route onlyexact30ppd/age24/D65 luminance
shape/profile to evaluate_pathway_quality,keepingold32/64/128/64x128 routes.
Then run sixpairPFM CLI nativebitchecks +existing11pairregression andnegative
unsupportedview/profilechecks before declaringpublicintegrationdone. Preserve
allpre-existingchanges/staging/submodule;no publishingwithout authorization.
Broadergeometry/realHDRRGB/PPD/age/devices/GPUresidency/additionaloutputs open.

## GitHub publication authorization (2026-10-25)

User explicitly requested sending successful work to their GitHub. Publishing
validated runtime/API sources,reference/shader generators,tests andvalidation
documentation to origin feature/hdr-vdp3. Numerical acceptance remains sixpairs,
487checks/1966412words bitwise +13guard checks,with existing11pairregression
historically unchanged. No numerical implementation changed during publishing.
README now states exact experimental scope and localfixture/runtime limitations.
Pre-existing staged+unstaged corrdn shader,dirtyVkFFT,bin experiments,localglslang
configs,logs/binaries/builds/validation data andoldreferences are excluded and
preserved. An explicit path list and index snapshot are saved in the local
validation/github_publish_active_path.txt root. Commit uses --only with this
path list so unrelated original staging is retained. Push is a normal branch
push,no force. Final commit/remote verification is recorded in that local root.
Next implementation step remains explicit runtime-manifest packaging and CLI
routing,then sixpairCLI +existing11pairregression before opening geometryguards.
