# Absolute luminance input checks

This standalone project builds the grayscale float PFM reader without Vulkan
or the experimental metric wrapper. It also builds a validation-only Radiance
HDR crop preparer using the repository's stb decoder.

```powershell
octave-cli --quiet --no-gui tests/generate_luminance_input_reference.m '<fresh fixture directory>'
cmake -S tests/hdrvdp3_input -B '<fresh build>' -DIQM_LUMINANCE_FIXTURE='<absolute fixture directory>'
cmake --build '<fresh build>' --config Release
ctest --test-dir '<fresh build>' -C Release --output-on-failure
```

The input API is `IQM::HDRVDP3::load_luminance_pfm(path)`. It reads grayscale
`Pf` with a three-line header, either byte order, and bottom-to-top raster
as described by [Netpbm's PFM documentation](https://netpbm.sourceforge.net/doc/pfm.html).
It returns top-to-bottom row-major binary32 luminance. Header scale magnitude
is applied once in binary32; scale1 preserves bits. The caller supplies
absolute cd/m² calibration. Positive finite values are required by the current
metric path. Zero/nonfinite values, RGB `PF`, invalid dimensions/scale,
truncated payloads and trailing bytes are rejected. There is no resizing,
display normalization, gamma correction or tone mapping. Other PFM row-order
variants require explicit conversion before loading.

`prepare_hdr_luminance <HDR source> <fresh output directory>` extracts centered
32x32 and64x64 crops, writes grayscale PFM and expected text values, and creates
localized-gain/global-brightening pairs. It explicitly applies numeric linear
Rec.709 coefficients with binary32 non-FMA arithmetic and assumes one cd/m²
per derived Y unit. Source primaries and photometric calibration are not
independently established. This prepares shared luminance inputs for the
native `luminance` reference path; full `rgb-native` parity remains pending.
The original full-size HDR image is not evaluated by this fixed-size harness.

See [file-input metric validation](../LUMINANCE_INPUT_VALIDATION.md) for scopes,
measured results and the retained runtime artifacts. Test HDR data are external;
the source image and generated crops are not included in the commit.
