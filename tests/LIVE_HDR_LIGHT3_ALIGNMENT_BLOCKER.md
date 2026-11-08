# LIVE HDR Light3 alignment blocker ? 2026-11-08

The Light2 pilot passed all 12 sampled frames and its ordered Single video mean exactly (Q_JOD 6.320251941680908). This brings the completed continuation checkpoint to 24 videos / 417 sampled frames; it does not establish original paper cohort reproduction.

The next reference, 4k_ref_Light3, declares 367 frames but ffprobe outputs 362; the distorted 1080p_1M_Light3 outputs 367. The reference average rate is 660600/11021 versus nominal 60000/1001. Reference decoded indices 359?361 jump from PTS 537537 to 540540, 543543 and 549549. The timing log contains no decoder error. A corrupt archive cannot be inferred from these observations: staging previously verified ZIP entry bytes.

Evidence: validation/live_hdr_full_scores_20261107_run03/1080p_1M_Light3 and validation/live_hdr_light3_alignment_20261108_run01/findings.json.

Do not silently truncate, duplicate, interpolate, drop or index-pair the final frames. Original reference media or a documented timestamp/content mapping is needed. No quality mismatch has been measured for this blocked pair.

The separate eligible continuation uses tests/resume_live_hdr_eligible_clockgrid.cjs and validation/live_hdr_full_scores_20261108_run01. It records this exclusion in blocked_videos.json and cannot mark the full archive passed. All earlier failed outputs remain preserved. A duplicate run02 controller was explicitly stopped; run01 is the intended active job. Independent whole-archive acceptance remains unavailable until the blocker is resolved.

## Decoder and container diagnosis (2026-11-08)

The reference edit-list media_time is 10997 at a 90000 Hz media timebase; test has a 66 ms empty edit plus media_time 2002 at 60000 Hz. Exact MP4 box parser, source hashes and outputs: validation/live_hdr_light3_mp4_boxes_20261108_run01.

With ffprobe -ignore_editlist 1 the reference produces 367 frames (start_pts 4505); -flags2 +showall still produces 362. The ignore-editlist sequence still has terminal PTS gaps at decoded indices 364, 365, 366 (3003, 3003, 6006 ticks). Thus the option restores the count but does not prove a complete CFR timeline or matched content. No evaluation decoder policy was changed.

Sparse nominal-clock inspection maps reference missing logical slots to 359, 361, 363, 364, 365; logical slot 360 is decoded reference index 359. Test logical slot 360 is decoded index 360. This is an explicit ambiguity between decoded-index and nominal-time sampling, not an accepted replacement mapping. Artifact: validation/live_hdr_light3_sparse_clock_20261108_run01.

Independent encoded-domain 64x36 gray16 thumbnail analysis is running under tests/diagnose_live_hdr_light3_content.cjs. It does not generate metric inputs, update the full-run acceptance or authorize alignment.

### Completed content diagnostic

All 362 default reference thumbnails are byte-identical to ignore-editlist thumbnails at index +5 (64x36 encoded-domain gray16). Ignoring edits restores five leading frames, not five safely aligned missing trailing frames. Therefore merely matching frame counts would introduce a five-frame content offset. Exact thumbnail mapping: validation/live_hdr_light3_content_20261108_run01/reference_mode_hash_mapping.json. This equality applies only to diagnostic thumbnails between decoder modes of the same reference.

For 337/362 default-reference thumbnails, the best normalized spatial correlation within a +/-15 test-frame window is at identical decoded index; with ignored edits, 337/367 have a -5 test index offset. Slow motion and compression produce very small competing-match margins, especially at the end, so these correlations are not an alignment acceptance test. The final reference thumbnail's best match has a margin of only about 7.1e-8. All raw thumbnails, commands, source hashes and per-frame candidates are retained. No original paper decoder setting or final-frame mapping is inferred.

Next: inspect original author decoding/frame-list policy or obtain corrected aligned reference media; meanwhile continue explicitly eligible videos without claiming full-archive or paper reproduction.
