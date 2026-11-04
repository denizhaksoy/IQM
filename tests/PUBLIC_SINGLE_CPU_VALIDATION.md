# Public CPU Single quality: real-image acceptance

2026-11-02: `IQM-hdrvdp3-cpu.exe` was invoked with the public named CLI options,
not the frozen diagnostic executable's positional interface. The test used
frame0 of `1080p_9M_CargoBoat` and `4k_ref_CargoBoat` as independently decoded
RGB16 PQ PNGs. The reference canvas is3840x2160; distorted PNGs are resized in
PQ before the photometric conversion.

| Output | Clean official3.0.7 = public CPU CLI | Single hex |
|---|---|---|
| Q | 6.786186695098877 | 40d92871 |
| Q_JOD | 7.6794109344482422 | 40f5bdbc |

`tests/run_live_hdr_cpu.cjs` checks the official source PNG hashes, version,
Single precision and PPD before invocation. It verifies both result bits and
the declared CPU/FFTW backend, zero GPU matrix dispatches, age24/OLED,
ambient200lux/reflectivity0.005/peak2000 and FFTW3threads/estimate settings.
Executable, model files, DLL, input and official-score SHA256 records, logs and
a copy of the executable are preserved in
`validation/live_hdr_public_cpu_cargoboat9M_20261102_run01`.

This confirms the public CPU application interface on this real frame. It is
an existing cohort case and does not increase the unique dataset sample count.
It does not establish all viewing conditions, all original metric tasks,
fullGPU completion or original paper submission reproduction.
