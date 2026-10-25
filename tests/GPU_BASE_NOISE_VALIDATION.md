# GPU-derived base adaptation, frequency and noise

Checkpoint `validation/gpu_base_noise_639284555583605395`,2026-10-24,
Intel Arc Pro Graphics. Experimental synchronous/file composition only;
production profiles, shape guards and algorithms remain unchanged.

The masking checker's final profile obtains base log maps through GPU resize
and clamp, then runs the accepted base120 shader for mean and10^mean. The
bounded120-log domain is checked before dispatch. The dominant frequency index
comes from accepted GPU SPYR/residual stage10, not native selection. GPU base
CSF parameters are evaluated from the GPU luminance. Existing CPU double NCSF
evaluation then produces noise; that calculated value supplies D/quality.
Native adaptation/frequency/noise files are comparison goldens only. Fixed
model parameters and CPU double NCSF remain part of the existing contract.

An important arithmetic distinction is preserved: for10x12/12x10, native
frequency-grid coordinate construction is not bit-identical to the simpler
folded k/n expression in the production frequency helper. The experimental
checker preserves native arithmetic order. The old production helper is
unchanged; public general-shape quality remains disabled. A separate native
grid export validates **all240 double frequencies bitwise**, including native
rounding such as0.31250000000000011 and0.37499999999999989. Native source hash
is recorded; installed reference code was not edited or redistributed.

`gpu/PASS.txt`/`results.tsv`:126 checks,393384 values,zero numerical and
observed bit differences. The new additions are four base mean/power outputs
and six selected-index/frequency/noise scalar comparisons across two pairs.
All previous122 comparisons are retained, including D,D_p,band errors and
Q/Q_JOD. Double noise matches the native double exactly. No numerical trial
failed in this checkpoint. The refactored two-buffer kernel helper separately
sizes input/output buffers and is exercised by both D_p and base120 paths.

```powershell
octave-cli --quiet --no-gui tests/generate_base_frequency_grid_reference.m <fresh grid>
pathway-masking-test --frequency-grid <grid>/grid.txt <fresh grid check>
pathway-masking-test <bounded map.spv> <native masking goldens> <fresh GPU> <D_p.spv> <GPU comparison> <GPU bands> <GPU pool> <native log trace> <base120.spv> <GPU SPYR all-chain dumps>
```

Upstream native P/log-adaptation maps are still runtime inputs. Components
exchange files and host buffers, rather than one resident image-to-score GPU
pipeline. Matching native scores here does not establish full metric completion.

## Upstream native references prepared

`validation/general_visual_trace_639284557380638502` uses the optional
`generate_general_pathway_reference.m --visual` profile. It instruments fresh
copies of both metric and visual-pathway functions in the validation directory,
retaining licenses and hashes. Originals remain untouched. Native original and
instrumented Q/Q_JOD are exactly equal for both actual pairs.

`export_visual_pathway_stages.m` exports the four reference/test paths:retinal
luminance,double LMSR matrix,before/after-pupil single LMSR maps,local adaptation,
pupil,cone/rod channels,P and native JND lookup tables. Original classes are
recorded; MAT retains full arrays/cells. P_reference,P_test andlog_La are checked
bitwise against the previously accepted full masking traces. `stages/PASS.txt`
confirms identity. These newly exported upstream maps are native references,
not yet new GPU stage acceptance.30ppd,age24; no extra DC removal or FFT padding
is added by tracing. Text exports are row-major decimal17digits.

## Production frequency coordinate order (2026-10-25)

The native coordinate modulo and two-step scaling are now used in production
quality.cpp, including the direct DFT helper's selected coordinate. The old
folded k/n expression differed in binary64 on10x12/12x10. New standalone
verify_frequency_grid.cpp links production quality.cpp and checks all240
native double words bitwise at3.75ppd. Evidence:
validation/production_frequency_639285092485366527/grid/PASS.txt.
The experimental masking checker now calls that production function instead
of maintaining its own implementation. This change does not enable public
80x96/96x80 input support or imply FFT selection parity for arbitrary inputs.

The production-function HDR quality rerun also passes126 checks/393384 values
and independent final Q/Q_JOD bit identity for both orientations. Artifacts:
production_frequency_639285092485366527/quality and /validated. The existing
GPU-derived buffers are reused unchanged; this isolates the production CPU
frequency/CSF/quality integration from already validated upstream filters.

Final production CLI regression also passes11 PFM pairs/1166 numerical exports
and exact quality files: production_frequency_639285092485366527/cli_regression.
