# Parallel work while the frozen LIVE HDR evaluation runs

Update 2026-11-05: the standalone three-buffer GPU mask is now implemented and
passed 9,294,525 bitwise comparisons, including a real 4K capture. An optional
quality-stage provider and its separate validation target have been added.
See `COMPARISON_SINGLE_GPU_VALIDATION.md` for measured scope and commands.
The sections below preserve the original 2026-11-04 preparation plan; they are
not a statement that the standalone kernel remains unimplemented.

2026-11-04. The active numerical jobs use frozen executables, model files and
the clean reference. Independent development must keep those binaries, shaders,
reference files and active runner/scorer sources unchanged. Use separate build
and shader-output directories; never run compile_shaders.sh for this work.

## Next bounded GPU stage: the comparison mask

The current CPU quality stage in `general_quality_single.cpp` evaluates:

```cpp
comparison[i] = any_channel(abs(test[c*n+i] - reference[c*n+i])
                          / reference[c*n+i] > 0.001f) ? 1.0 : 0.0;
```

Inputs are positive finite Single native RGB planes already produced by the
front-end. The binary mask is later bicubic-resized as Double. A standalone
GPU mask can be compared bitwise with the existing CPU loop before connecting
it to quality evaluation. Native mask bits, resized masks and final Q/Q_JOD
must remain separate acceptance checks.

### Required implementation constraints

- Keep reference and test RGB in **separate storage buffers**. Combining both
  4K images into the current projection helper's single input buffer requires
  199,065,600bytes, exceeding a128MiB storage-buffer range. Each individual RGB
  input requires99,532,800bytes; a Single mask requires33,177,600bytes. Query
  actual device limits and validate every descriptor range before dispatch.
- Preserve subtraction, absolute value, division and strict comparison rounding
  at the Float32 threshold. Do not assume an algebraic multiplication rewrite or
  a native shader division gives identical classifications near0.001f.
- Test values immediately below/at/above the threshold, both signs of the RGB
  difference, each channel alone, identical images and native-domain extremes.
  Then compare all mask pixels on saved real4K native tensors, including the
  football150 investigation case. A mismatch is a failure, not a tolerance pass.
- If a Float64 technique is used, explicitly require/enable shaderFloat64.
  VulkanInstance enables this feature only when requested. Report that device
  requirement rather than silently falling back to CPU.
- Introduce a separate explicit provider/experimental path after the standalone
  kernel passes. Preserve the current CPU path and four-dispatch hybrid mode;
  new stage coverage/dispatch count must be reported accurately.

The current projection helper packs RGB and coefficients into one input buffer;
it cannot be reused unchanged for two full4K RGB images. A three-buffer runtime
is the concrete next implementation step. This document is implementation
preparation; no GPU comparison-mask kernel or numerical acceptance is claimed.

## Work that depends on the running results

Final cohort correlations, the numerical acceptance report and the decision on
the unresolved historical reference record require the active runs' results.
The conditional continuation already performs the remaining local evaluation,
independent aggregation/statistics, evidence audit and representative public
backend checks. Full GPU coverage and other original metric configurations
remain additional development work even if this cohort passes.
