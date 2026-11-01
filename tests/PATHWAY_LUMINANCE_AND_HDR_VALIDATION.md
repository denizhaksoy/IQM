# Luminance controls and HDR-derived grayscale validation

These tests extend the fixed 80x96/96x80 in-memory pathway at 30 ppd,
age 24, D65 and sp0. They do not establish arbitrary-size or RGB parity.
Official HDR-VDP-3 v3.0.7 stock and instrumented reference scores agree before
the independent Vulkan API comparisons. Inputs are positive Float32; native
FFTW uses three threads, estimate planning and fresh single wisdom, with
official padding and DC processing. Runtime assets are the source-generated,
hash-pinned package; no image-dependent reference data is a runtime input.

## Luminance and decreasing contrast

Checkpoint: `validation/pathway_luminance_639285490131386436`.

| Input | Distortion | Gain |
|---|---|---|
| Constant 0.0001 cd/m² | Uniform | 1.01 |
| Constant 10000 cd/m² | Uniform | 1.01 |
| Asymmetric | Localized | 0.9999 |
| Synthetic HDR ramp | Localized | 0.999 |
| Asymmetric | Uniform | 0.99 |
| Constant 10 cd/m² | Uniform | 0.99 |

Each runs in both orientations, with zero additive offset: twelve pairs,
828 stage checks and 2,852,796 values pass bitwise, without Vulkan validation
errors. Score rounding to 10 is retained as native behavior. Intermediate
maps and accumulated errors are checked independently of rounded scores.

## Photograph-derived grayscale stimulus

Checkpoint: `validation/real_hdr_639285493148770692`.
Source: [Debevec's HDR research page](https://www.pauldebevec.com/Research/HDR/),
[Memorial Church RGBE image](https://www.pauldebevec.com/Research/HDR/memorial.hdr).
Downloaded input SHA256:
`18376B9219927B363127817900B1C170A0F752F52A63F5949655B4150E35061D`.
The photograph remains local under ignored validation data.

`tests/prepare_real_hdr.cpp` decodes the vendored stb RGBE reader's linear
Float32 RGB, takes the arithmetic RGB mean in double and scales the whole
source peak to an assigned 1000 cd/m². It applies a 0.0001 floor and one
Float32 cast, then extracts two center crops without resize, gamma or tone
mapping. This defines a grayscale display stimulus; it does not recover
absolute captured scene luminance or assume known RGB primaries/CIE Y.
The 512x768 source produces crop ranges 0.0103879–1.29148 and
0.00877256–1.29148 cd/m², with no floor-clamped pixels.

Localized gain 1.01, offset zero: both crops pass all 138 checks and 475,466
values bitwise, including P/log, bands, pooling, masking, CSF, perceptual
differences, psychometric maps, band/total errors, actual base frequency/noise,
Q and Q_JOD. No Vulkan validation errors. Q/Q_JOD are respectively
9.9999561309814453/9.999999046325684 and
9.999942779541016/9.999998092651367.

Localized gain 1.4, offset zero also passes both crops: another 138 checks
and 475,466 values bitwise, without Vulkan validation errors. Q/Q_JOD are
9.842323303222656/9.951226234436035 and
9.793542861938477/9.931108474731445. Thus four photograph-derived pairs
pass 276 checks / 950,932 values. These are API/trace tests, not four CLI runs.

Build the preparation utility with `tests/hdrvdp3_hdr_inputs/CMakeLists.txt`.
Run `prepare-real-hdr.exe <source.hdr> <fresh-output>`, then
`generate_general_pathway_reference.m <fresh-native> --pattern external
--input-directory <prepared-output> --transform localized --gain 1.01
--offset 0`. Run `export_native_band_errors.m <native>` before checking each
shape with `IQM-pathway-contrast-test <native-shape> <runtime> <fresh-results>`.
See `PATHWAY_CONTRAST_VALIDATION.md` for the strict checker contract.
`tests/hdrvdp3_evaluation/validate_pathway_hdr.ps1` automates both shapes;
its `-ResultsOnly` option audits retained outputs. Invoke PowerShell with
`-NoProfile -ExecutionPolicy Bypass -File` as in the other validation scripts.

External-input support preserves all 206 default numerical text exports
byte-for-byte (207 files when the pre-existing PASS marker is included).
No production numerical algorithm or tolerance changed for these tests.

## Public CLI reproduction

The photograph-derived CLI validator uses Node to write binary Float32 PFM
files from the native original inputs, reads the files back byte-for-byte
with the PFM row inversion, then invokes the public executable with Vulkan
validation enabled from the fresh output directory. It compares input buffers
to the originals, and P/log/scores to both official native text and previously
accepted API binary outputs. The quality text must contain exactly eight
finite values: five band errors, total error, Q and Q_JOD; their Float32 bits
must match both native and API data. Every invocation and log is retained.

```powershell
node tests/hdrvdp3_evaluation/validate_pathway_hdr_cli.cjs <IQM-hdrvdp3.exe> <runtime-directory> <real-HDR-root> <fresh-cli-results>
```

The source root must contain `native`, `native_stronger`, `api` and
`api_stronger` from the preceding checks. The validator fails on a nonzero
CLI exit, any bit mismatch, incomplete quality output or Vulkan error, and
writes its final PASS marker only after all four pairs finish.

Completed CLI checkpoint: `validation/real_hdr_cli_639285498027629037`.
All four pairs pass: 28 exports match original/native/API bits, including
the band-error and score values in the quality text. All CLI exits are zero;
Vulkan validation reports no errors. PFM readback, invocation logs, validator
snapshot, input/runtime/executable hashes and final artifact hashes are retained.
