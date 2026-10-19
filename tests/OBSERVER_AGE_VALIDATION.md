# Observer age: isolated component validation

Updated 2026-10-19. Full metric parity is still scoped to age 24/D65.
The following checks establish two components needed for broader age support;
they do not validate age-dependent spectral/JND processing or full scores.

Evidence: `validation/pupil_age_639280292892783820/COMPLETED.json`, selected
by `validation/pupil_age_completed_path.txt`. It includes source/binary
hashes, reference fixtures, GPU validation logs and strict double CSF logs.

## Pupil

`generate_pupil_age_reference.m` calls the official v3.0.7 `geomean` and
`pupil_d_unified` functions. Five adaptation maps (constant 0.001, 10 and
10,000 cd/m², a spatial ramp and deterministic random luminance) are tested
at 32x32/64x64 and 30/60 ppd, with area=width*height/ppd².

The ages are 0, 15, 19.999, 20, 24, 28, 45, 60, 60.001, 75, 83, 83.001 and
100. Values outside 20–83 exercise the official pupil function's clipping;
they are not a claim of valid full-metric observer ages. The 60-year boundary
is included for later lens tests; pupil has no special branch there.

The Vulkan checker compares geometric mean, reference pupil diameter,
observer pupil diameter and squared diameter ratio: **260 cases, 1,040
native-single values, zero differences**. Transport is row-major; reduction
is column-major. DC processing and image padding do not apply to this
isolated adaptation-map input. Numeric float equality is the pass condition.
The new `--pupil-age` checker mode is local integration; its separate build
does not overwrite executables used by concurrent full metric runs.

## CSF/AESL

The existing independent scalar checker tests the same 13 ages, AESL on/off,
12 frequencies (including the low-frequency cutoff) and 16 luminances.
Both double-luminance and single-luminance query paths are compared in
**double precision**: 4,992 values each, **9,984 total, zero differences**.
This does not establish GPU AESL computation; the scalar CSF preparation
remains CPU work.

`generate_csf_reference.m` accepts an optional third argument containing
comma-separated ages. Historical defaults remain 24,45,70. Age fields now
use 17 significant digits so noninteger ages survive text transport. The
accepted expanded fixture is `csf_precise_age`; the first `csf` fixture is
preserved and is not counted twice.

```powershell
octave-cli --quiet --no-gui tests/generate_csf_reference.m '<fresh directory>' C:/HDR-VDP-3_Ref '0,15,19.999,20,24,28,45,60,60.001,75,83,83.001,100'
```

## Remaining age integration

The official quality defaults enable pupil contraction, AESL and aging
optical density (`do_slum`, `do_aesl`, `do_aod`); `do_aslw` is disabled.
`hdrvdp_visual_pathway.m` changes lens optical density at age 60, interpolates
it with pchip, applies transmission to the emission spectrum and integrates
the normalized LMSR matrix. The current C++ single JND path and Vulkan map
shader retain age-24 D65 coefficients. Default CSF parameters also select age
24 unless explicitly supplied.

Next implement and independently compare the age-dependent spectral matrix,
including 60/60.001 and the age-24 regression. Then pass observer age and the
matrix consistently through pupil, JND, CSF/AESL and all native validators;
run fresh full metric pairs. Merely exposing an age environment variable
would give inconsistent processing and is not an accepted implementation.
