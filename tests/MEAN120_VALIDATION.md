# Exact bounded120-sample mean and DC subtraction

Checkpoint: `validation/mean120_639283811026801827`.
New isolated `mean120.comp` computes the mean of120 Float32 inputs and subtracts
that single-rounded mean from each input. Production MapGPU guards and shaders
remain unchanged. This does not implement base-adaptation pow10 or full quality.

Domain: finite normal Float32 or signed zero, nonzero unbiased exponents
[-90,100] with spread at most16. The host checks this contract before dispatch.
At most47 magnitude bits suffice for the exact sum, accumulated in two uint32
words. The shader performs exact integer quotient/remainder division for120
and ties-to-even rounding directly to Float32, then precise Float32 subtraction.
There is no Float64/Int64 capability and no reciprocal multiplication.

Within this domain native double accumulation is exact, including all partial
sums. For division by120=15*8, a non-midpoint exact rational is separated from
a Float32 midpoint by more than the double-division rounding error with this
47-bit numerator bound; an exact midpoint is representable in double. Thus
direct Float32 rounding matches double division followed by Float32 conversion.
This reasoning and the measured probes do not extend the allowed input domain.
The domain also keeps nonzero mean/subtraction results normal. All-zero mean
is positive zero, while centered negative-zero inputs retain their sign.

`generate_mean120_reference.m` exports12 official SPYR residual fixtures from
80x96/96x80 images: asymmetric,constant10 plus impulse7,bounded HDR ramp1e-2..1e3,
each ref/test factor1.05 at30ppd. It verifies120 residual samples, observed
exponent eligibility, native single mean against exact double accumulation,
and exports mean followed by120 centered words. These are standalone official
SPYR component fixtures, not complete visual-pathway or full-quality pairs.

Result on Intel Arc Pro Graphics:8192 synthetic cases (991232 output words)
and12 native cases (1452 output words) match **bitwise**, including signed zeros.
Synthetic cases span the exponent domain, random mantissas/signs, allzero,
negative-zero,constant and cancellation controls. This is not exhaustive over
all possible120-tuples. Host reference requires RTE and gradual underflow;
native mean/subtraction words are independently checked against the host
double-accumulation contract before GPU comparison.

Input layout: each contiguous group has120 row-major real Float32 bit words.
Output: one mean word followed by120 centered words per group. No FFT,
padding, MTF/LA filter or crop is performed here. Normalization is division120
once, before single subtraction; this DC operation is separate from FFT inverse
normalization30720. Explicit DenormPreserve32/RTE32 SPIR-V modes are compiled
and validated. Capability checks alone do not activate them.

Build `mean120-test` from `tests/hdrvdp3_dimensions`. Reproduce:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests/hdrvdp3_dimensions/compile_mean120.ps1 -OutputDirectory <fresh shader directory>
octave-cli --quiet --no-gui tests/generate_mean120_reference.m <fresh native directory>
mean120-test <shader.spv> <native directory>/manifest.txt <fresh GPU directory>
```

Acceptance is `gpu/PASS.txt`, `gpu/results.tsv`, preserved input/expected/output
word artifacts and GPU log. Native Octave emitted package-shadow warnings and
its usual exit cleanup message but returned exit0 and completed all12 fixtures.
No numerical failures occurred. Next: integrate this component with residual
10x12/12x10 transforms and existing base-adaptation power handling after those
contracts are independently validated; retain the public shape guards.
