# Identity acceptance for the mixed pathway

The native generator's explicit `--identity` option preserves the default
localized transform. It compares each of asymmetric,impulse andHDR-ramp inputs
with itself in80x96/96x80. Stock and traced official3.0.7 scores must agree.
The installed reference is read-only;tracing uses an isolated copy. A regression
regenerated206 default localized text exports,all hashes identical to the
previous accepted trace. Old references remain unchanged.

Identity has an important singular case:the native residual FFT maximum selects
index1 (zero-based0),frequency is0,andNCSF sensitivity0 produces positive infinite
base noise. The exporter now permits this value only for explicit identity base
neural/selection fields,with finite index/frequency/luminance,frequency0 and
positive infinity required. NaN and unrelated nonfinite data remain rejected.
No production numerical algorithm or exceptional math policy was loosened.

`validate_pathway_identity.ps1` checks six self-PFM comparisons against native
input/P/log/score bits and zero band/total errors. An independent in-memory
`IQM-pathway-identity-test` measures actual diagnostic state,not just the final
score:GPU selectedindex0,frequency+0 andnoise+Inf binary64 words,andzero D,
psychometric,masked andquality errors. Native P/log/score words must match and
Q=Q_JOD=10. Signed zero in the zero difference maps is not separately asserted.
`validate_pathway_identity_trace.ps1` audits all six diagnostic traces/logs.
CLI andAPI runs reuse the same sixpairs;they are not twelve unique pairs.

Checkpoint: validation/pathway_identity_639285453875965132,Intel Arc Pro Graphics.
The runtime package was generated from an isolated source-only tree andmatches
the accepted package's20payload hashes exactly. Original inputs are positive
Float32 luminance,30ppd,age24,D65,sp0;log/padding/DC/order remain the native
contracts. This limit check does not permit arbitrary nonfinite user pixels or
extend geometry,viewing conditions,RGB/device coverage.

Final result:all sixCLI andall sixAPI trace runs PASS,no Vulkan errors. The
trace auditor verifies the exact+0/+Inf binary64 diagnostic bytes. The default
reference-generator regression compares206 numerical text exports identically.
Artifacts:cli/PASS.txt,API_PASS.txt,api_results.json,default_control_PASS.txt.
