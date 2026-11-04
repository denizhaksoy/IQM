# Separate official benchmark found during LIVE HDR evaluation

Checked 2026-11-02:

- [Cambridge ColorVideoVDP benchmark](https://www.cl.cam.ac.uk/research/rainbow/projects/colorvideovdp/reports/benchmark/)
- [Its HDR-VDP-3 v3.0.7 LIVEHDR report](https://www.cl.cam.ac.uk/research/rainbow/projects/colorvideovdp/reports/benchmark/LIVEHDR_HDR-VDP-3/index.html)
- [Original HDR-VDP-3 paper, Section 6](https://www.cl.cam.ac.uk/~rkm38/pdfs/mantiuk2023hdrvdp3.pdf)

The benchmark states that its summary table covers testing data and that its
detailed reports cover both training and testing data. It also describes JOD
regression fitted using training data. These conditions are not automatically
equivalent to the original 2023 WACV submission, nor to our explicit 27-video
local candidate cohort and raw, unfitted bright-MOS correlation.

The detailed HDR-VDP-3 LIVEHDR HTML exposes two plot images, not a per-video
prediction CSV or a sampling/decoder manifest. This discovery therefore does
not resolve the missing original author cohort, PNG precision, sampling phase,
or original execution backend. No benchmark table numbers are used as numerical
acceptance thresholds. Our numerical reference remains clean official v3.0.7
executed on the same selected PNG inputs as C++.

Downloaded HTML is preserved in
`validation/live_hdr_author_benchmark_20261102_run01`. It is supplementary
provenance, not a replacement for the Section 6 protocol or original submission.
