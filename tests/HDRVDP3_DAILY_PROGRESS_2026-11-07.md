# HDR-VDP-3 progress - 2026-11-07

- Compact official PNG-to-score workflow:16/16 exact score-bit and input-hash comparisons passed. Single extrema compared by exact Float32 encoding; no numerical tolerance.
- Supplied LIVE archive:310 ZIP-byte-verified videos,31 reference groups,279 distorted pairs; HDR stream headers verified. Planned local phase0/stride30 samples:4716.
- New compact end-to-end Daylight pilot: 13/16 frames exact; remaining3 pending at this checkpoint. Full archive controller waits for all16 frames and equality with preserved scores/PNGhashes/Singlemean before automatic evaluation.
- Double GPU comparison-mask resize:COMPLETE529/529 frames,1,456,866,000 Double values exact; final input/runtime hashes passed. This tests one stage only.
- Independent final full-archive audit tool prepared; syntax and unfinished-input rejection checked. Positive final audit and correlations await all279 videos.

Artifacts: validation/live_hdr_compact_daylight_20261107_run02, validation/live_hdr_full_scores_20261107_run01, validation/comparison_resize_live_continuation_20261107_run01.

No full archive equivalence or original paper reproduction claimed. Initial football150 official discrepancy and undocumented author evaluation choices remain unresolved. Runtime remains hybrid CPU/Vulkan. No commit/push in this session.

Independent full-archive audit watcher registered; it runs automatically only after all279 videos pass, and stops if evaluation reports a failure.

Update: Daylight pilot COMPLETE16/16, exact video mean8.4828519821166992. Full run01 completed2 videos/40frames, independently audited and preserved. Processes subsequently stopped without numerical failure; fresh run02 references both audited completions and restarts interrupted third video in a new directory. Hidden background controller24612 and finalaudit watcher15416 started; inspect live process and checkpoint rather than inferring activity from old logs.

Update: Full archive3/279 videos complete (61frames), all exact. CenterPanorama21frames independently audited; exact mean8.756434440612793. FourthvideoChasing1 active; first2frames exact. Controller and finalaudit watcher verified live; no failure recorded.

FullGPU work: optionalGPU L+M backend integrated and passed16/16 score-bit comparisons. GPU doubled symmetric/replicate padding passed20standalone cases/66,680,056values and, combined withL+M,16/16end-to-end cases including2real4Kpairs. FourlegacyCLImodes preserved exactportrait scores withnewoptionsabsent. Allfour targets compiled. ActualCPU FFTWgraphs exported for4Kpadding/base128/oddshapes aspreparation; GPUFFT itself remainsunimplemented in this newqualityprofile. full_gpu=false. Artifacts padding_integration_20261107_run01, gpu_frontend_compatibility_20261107_run02, fft_plan_provenance_20261107_run01.

GPU FFT progress: corrected128forward graph selection for3thread FFTW (historical22thread shader retained).16forward/inverse cases/524288components exact. Filtermultiply5cases/232972values exact inclsignedzero/subnormals. GPUforward/filter/inverse/crop andfixed128FFTBackendSingle provider each passed4convolution cases/28671values; unsupportedshape rejected.4KmixedradixGPUFFT notimplemented:13codeletoriginalsources extracted/hashverified fornextport. Fullarchive22videoscomplete,lastcheckgolf2active.

Later update: Fullrun02 stoppedbeforeLight2 quality dueabsolutetime origins/timebases,not numericalmismatch.23completevideos/405framespreserved. ExactintegerCFRclockvalidation addedwithnegativeguards; actual352frameLight2certificatepassed. Fresh12samplepilot running(firstframeexact),thennewrun03controller reuses23auditedcompletions andcontinuesautomatically. NewGPU n1_4leaf passed9604transforms/76832valuesbitwise; remaining4Kcompositionunfinished.
