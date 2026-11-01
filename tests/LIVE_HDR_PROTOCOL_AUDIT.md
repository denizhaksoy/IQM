# LIVE HDR / HDR-VDP-3 Section 6 protocol audit

Date: 2026-11-01. Status: protocol audit completed; evaluation pipeline not yet implemented.
No LIVE HDR metric scores or published-result equivalence are claimed.

## Sources and scope

- [HDR-VDP-3 paper, Section 6](https://www.cl.cam.ac.uk/~rkm38/pdfs/mantiuk2023hdrvdp3.pdf).
- [Official LIVE HDR dataset page](https://live.ece.utexas.edu/research/LIVEHDR/LIVEHDR_index.html).
- Clean official v3.0.7 distribution in `validation/general_quality_639290762115433659/official_clean/hdrvdp-3.0.7`.
- User archive staged at `validation/live_hdr_639290787607997134`; original ZIP remains unchanged in Downloads.
- Machine-readable audit: `validation/live_hdr_protocol_20261101_audit01/metadata_audit.json`.

Section 6 specifies the quality task, independent frame evaluation, every-30th-frame PNG decoding with FFmpeg, averaging frame scores, inverse PQ into absolute linear BT.2020 RGB, and reflected ambient illumination of 200 lux with reflectivity 0.005. The model retains its UPIQ calibration; it is not fitted on LIVE HDR. The paper points to the v3.0.7 video example. It does not publish a per-video prediction vector or an exact correlation target in Section 6.

## 1. Pairings and subjective data: verified metadata

The supplied archive inventory and CSV files contain:

| Check | Result |
|---|---|
| Unique video names / metadata rows | 310 / 310 |
| Unique references | 31 |
| Reference self-pairs | 31 |
| Distorted-to-reference pairs | 279 |
| Rows per reference group | 10: one reference and nine distorted clips |
| Duplicate video names | 0 |
| Missing video/reference archive names | 0 / 0 |
| Declared frame count differs from declared reference count | 0 |
| Bright MOS/CI95/DMOS | 310 rows, complete, finite, unique |
| Dark MOS/CI95/DMOS | 310 rows, complete, finite, unique |

Pair by the CSV `video` and `ref` identifiers, preserving case; do not infer pairs from sorting or bitrate names. These checks establish metadata consistency, not successful decoding or temporal alignment of every clip. Only the Daylight1 reference and 4k_3M pair have been probed and their first frames decoded so far.

CSV files contain CRCRLF and an unnamed index column. The inspection script normalizes carriage returns in memory, preserves originals, and records their SHA256 hashes. It rejects quoted CSV instead of silently misparsing an unsupported format.

Bright-condition labels are the relevant starting point for a 200-lux evaluation. MOS, DMOS, reference inclusion and the exact challenge cohort must be reported explicitly. Human scores are correlation targets, not expected Q_JOD values. Keep dark-condition analysis separate and change its display model only with documented evidence. The README's dark-column descriptions mistakenly say bright; actual CSV column names distinguish the conditions.

## 2. Transfer function, color and luminance

Observed in both sampled Daylight1 streams: `yuv420p10le`, `bt2020nc`, `smpte2084`, `bt2020`, 3840x2160, 480 frames, `60000/1001` fps. CSV fps is nominally 60. Do not assume every archive stream has identical metadata before probing it.

The clean example uses `im2single(imread(...))` and `rgb-bt.2020`. Its `pq2lin.m` implements ST2084 with maximum 10000 cd/m² and constants n=0.1593017578125, m=78.84375, c1=0.8359375, c2=18.8515625, c3=18.6875. This conversion applies to normalized PQ RGB, after YCbCr-to-RGB decoding. It is not an sRGB conversion or a grayscale luma substitution.

The example adds `200*0.005/pi = 0.31830988618379069` to each linear RGB channel, then clamps each at `2000 + L_refl`. The **2000 cd/m² display limit is specified by the example, not Equation 1 in the paper**. Lower-resolution distorted frames are bicubic-resized in the PQ domain to reference dimensions and clamped to [0,1] before conversion. Reference geometry is retained.

Unresolved decoder details: FFmpeg version, RGB PNG output bit depth, range expansion, chroma reconstruction, matrix selection and dithering are not pinned in the paper or the example commands. Our already decoded `rgb48be` PNGs are a documented local diagnostic choice; they are not evidence of the original authors' PNG precision. The example's plain FFmpeg command does not explicitly request that format. A reproduction must retain this distinction and record decoded code values and PNG depth.

## 3. Frame sampling: intended protocol versus literal source

The example has `for kk=1:30:N_frames`, but both file paths are hardcoded to `frame_0001.png`. Executed literally, it repeatedly evaluates the first frame. It cannot implement the stated sampling protocol as written.

The defensible intended-example interpretation is one-based indices 1,31,61,... <= N, or zero-based 0,30,60,... < N. For 480 frames this selects 16 frames, ending at zero-based 450. **The paper does not independently establish this starting phase.** A future runner must name this interpretation and record the correction to the source; it must not silently claim the literal snippet was reproduced.

Use decoded frame indices, not seconds or nominal 60-fps timestamps. Derive N separately for each verified pair; the example's `N_frames=571` is clip-specific. Validate decoded counts, reference/test timing, selection order and selected-frame hashes. Do not drop/repeat frames through an implicit output frame-rate conversion. Temporal alignment remains unverified beyond matching CSV counts.

## 4. Metric settings and aggregation

The clean example specifies:

| Setting | Value / provenance |
|---|---|
| Task | `quality`, paper and example |
| Color encoding | `rgb-bt.2020`, example and paper's linear BT.2020 input |
| Geometry | 64.5-inch diagonal, [3840 2160], distance 1.2960 m; example |
| Pixels per degree | `hdrvdp_pix_per_deg(...)` = 62.729130063099447 in local Octave |
| Explicit options | `{'quiet', true}` only |
| Age | 24, implementation default |
| Display spectral model | `oled`, implementation default for BT.2020 |
| Optical MTF | `hdrvdp`, implementation default |
| Decomposition | `spyr` / `sp0Filters`, quality defaults |
| GPU | requested by default, disabled if unavailable; actual execution environment must be logged |
| Video score | arithmetic mean of per-frame `res.Q_JOD`, example |

Do not substitute mean Q followed by the Q-to-JOD mapping. The example uses uniform sample weights; it has no temporal model or duration weighting. Record the accumulator's actual precision rather than assuming double from its initialization to zero.

The example's TV geometry comment names a Samsung Q90T, while the metric's default spectral model is `oled`. Preserve and disclose this source behavior; do not replace it with a presumed measured TV spectrum. `Single` input is explicit in the example; downstream intermediate and output classes require measurement. Local CPU Octave is a reproducible numerical baseline, but not proof of the original submission's CPU/GPU environment.

The current independent C++ general-quality path supports Double input only. Promoting Single data to Double and running that path would be a separate diagnostic, not exact reproduction of the example's numerical semantics. Existing fixed GPU profiles also do not establish full-resolution LIVE parity.

## 5. Missing information and implementation gate

### Additional organizer evidence checked 2026-11-01

The [original workshop competition page](https://sites.google.com/view/wacv2023-workshop-quality-va/competition)
identifies bright Sureal MOS as the public target. Its public FR leaderboard
requires 27 predictions, excluding reference videos Bonfire, CargoBoat and
football1 from a 30-video set. It describes 27 sources / 243 distorted sequences
and a separate reserved final evaluation set. This differs from the supplied
31-source / 310-file release; it does not identify the full private-test mapping.
The stated criteria are SROCC (ranking), PLCC and RMSE. No exact regression
mapping or HDR-VDP-3 per-video vector is supplied there.

The [linked submission repository](https://github.com/Easoncyx/vqm_challenge)
has since changed: current main describes L40S hardware and CUDA12.8. Those
current settings are not evidence of the 2023 execution/decode configuration.
Do not adopt its current example conversion as the HDR-VDP-3 paper protocol.

A [historical README pinned to commit 0c8ce3ef](https://github.com/Easoncyx/vqm_challenge/blob/0c8ce3ef1d69240d3f43b2cf1061a47dd83e9b5f/README.md)
(2022-11-29) confirms V100/p3.2xlarge infrastructure and HEVC MP4
`yuv420p10le` inputs. It permits model-specific decoding/upscaling, so its
illustrative Lanczos/Y4M command does not establish HDR-VDP-3's PNG precision
or resize settings. Files and commit metadata are retained under
`validation/live_hdr_challenge_audit_20261101_run01`. The submission's actual
CPU/GPU backend remains unverified.

Before claiming original-submission reproduction, resolve or explicitly mark as unavailable:

1. The actual WACV train/test cohort and reference-inclusion policy. All 310 public files cannot automatically be equated with the submitted evaluation subset.
2. Original frame-selection phase and the fixed-filename correction.
3. Original FFmpeg/PNG precision and color conversion settings.
4. The original backend, MATLAB/Octave versions and intermediate numerical precision.
5. Original per-video predictions, correlation method, any fitted mapping, and reference/label filtering. Section 6 provides no numerical target to reproduce exactly.

A documented local reconstruction may proceed using clearly named choices, but those choices must remain distinct from author-confirmed settings. No undocumented correlation fitting or retraining should be introduced. Report rank correlation and raw Pearson correlation only under explicit analysis definitions; any mapped PLCC/RMSE needs a separately documented fitting procedure and split.

Next implementation step: create a protocol manifest separating paper facts, example settings, defaults, observed metadata and unresolved choices. Then verify all stream headers and selected-frame alignment, validate Single preprocessing against the clean official implementation, and establish one full-resolution per-frame official baseline before extending the C++/Vulkan path. Compare identical hashed tensors and settings; report bitwise equality separately from tolerance passes. Correlation with subjects is a separate validation axis.

## Reproduction and provenance

Metadata audit command (fresh output directory required):

```powershell
node tests/audit_live_hdr_metadata.cjs validation/live_hdr_639290787607997134 validation/live_hdr_protocol_20261101_audit01
```

Clean distribution ZIP SHA256: `BAA5C25CCA2E38176345A2A5423793459FBF571282396640C5E4AC85BF8381B3`.
Clean `hdrvdp3.m`: `0F9F267F68B042C167EF03F41DAAF07388AE192AB30F81B243766863F9A5402A`.
Video example: `4D67E8A5D1E68D54882393036D42AFF0B3FC6C29BAC0975C4343D0E1AE17E3B8`.
Geometry helper: `03C5D52DC22E272803C40D66AF7B6FC272186AE649922412BD862278770ED7F5`.
Use the clean distribution, not the older locally instrumented reference tree. No commit, push or cleanup was performed for this audit.
