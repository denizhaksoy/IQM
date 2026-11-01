# Main IQM application RGB integration

2026-10-27 checkpoint: `validation/main_rgb_active_path.txt`.

The normal root-project `IQM-HDRVDP3` library and `IQM` application targets
build successfully in `build-vs2026`, Release, Visual Studio18 2026/MSVC19.51.
This closes the earlier limitation that RGB had only been compiled in isolated
HDRVDP3 application/test projects. Other configured metric libraries link into
the main application; their numerical behavior was not tested in this run.

```powershell
cmake --build build-vs2026 --config Release --target IQM-HDRVDP3 IQM -j4
```

Initial builds reported nonfatal warnings (including missing optional glfw3
configuration, unused locals, shadowing and existing deprecated getenv usage).
Final successful target confirmation is saved in `build_confirmation.log`.
No source fixes, resets or shader regeneration were needed for this checkpoint.
Existing dirty VkFFT and staged/worktree corrdn shader content are preserved.

## Numerical checks

- The normal `build-vs2026/bin/Release/IQM.exe` processes the HDR-derived color
  PF reference/test pair with the previously source-generated RGB runtime,
  oled spectrum,96×96,30ppd,age24,rgb-native/default quality.
- **221,184 Float32 export words and8 quality values match official3.0.7
  reference bits**. Ten binary exports plus quality/profile text (12 files)
  are byte-identical to the accepted isolated HDRVDP3 application results.
- Normal main-application v4 competing_axes luminance evaluation retains
  Q=.98083019256591797 and Q_JOD1.2951202392578125. All seven original input,
  P,adaptation and score/text exports are byte-identical to the older frozen
  CLI cohort.
- Vulkan validation enabled for both runs, with no VUID/validation/error
  markers reported. These overlap previous cohorts, not new image content.

The real color source assignment and limitations remain those documented in
`RGB_CONTENT_VALIDATION.md`. This run broadens application integration proof,
not supported dimensions, viewing conditions, encodings or GPU residency.

## Next encoding boundary

Read-only review of official code identified a meaningful precision distinction:
`display_model_srgb` creates default Double output and applies its display
model, then the ITU709-to-native transform and a1e-6 out-of-gamut clamp.
It cannot be accepted by merely forwarding encoded values into the existing
native Float32 path. Linear `rgb-bt.709` is a separate next candidate.

`hdrvdp_iturgb2native` integrates CIE1931 XYZ against display primaries, scales
after trapz, normalizes by the native Y row sum and solves a3×3 system.
`hdrvdp_colorspace_transform` uses M×RGB' arithmetic, so its accumulation order
must be characterized rather than assumed from the accepted LMSR projection.
Current CLI continues to reject unsupported encodings. Next capture matrix,
conversion and clamp stages for linear709 before implementing a new entry.
