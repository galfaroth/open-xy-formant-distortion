# Formant by Veff

Stereo effect for Open-XY SDK 1.8: input-envelope formant coloration → wavefolder → signed frequency shifter → dynamic lowpass → output. Implemented and host/native tested; listening, hardware acceptance and measured device CPU remain pending.

**Experimental CPU-heavy effect.** The SDK host estimate is approximately 74,000 Blackfin instructions per 64-sample block, above its 30,000 target and 48,000 warning threshold. It may cause audio dropouts in dense projects. This is not a measured hardware result or a catalog-ready CPU measurement.

## Controls

Press any encoder to switch all four controls between pages.

| Page 1 | Range / behavior |
|---|---|
| Formant | Envelope-remapping offset, approximately −12…+12 semitones; center neutral |
| Fold | 1…8× drive into a triangle wavefolder; minimum bypasses folding |
| Shift | Approximately −200…+200 Hz additive translation; center neutral |
| Dynamic | Positive input envelope opens the lowpass |

| Page 2 | Behavior |
|---|---|
| Cutoff | Base lowpass coefficient; maximum bypasses filtering |
| Emphasis | Blends the complementary highpass back into the lowpass; no resonant feedback |
| Response | Detector release, approximately 3…23 ms; attack approximately 0.36 ms |
| Output | 0…2× gain; default 1× |

The original design proposed resonance. This implementation uses a stable complementary-filter emphasis control instead, and does not describe it as resonant filtering. The definition contains Neutral, Vocal Edge, Metal and Low Warp settings.

## What the formant stage does

Eight fixed bandpasses, spaced half an octave from 280 to 3168 Hz, measure the actual input's stereo-linked spectral envelope. The stage interpolates that envelope at shifted log-frequency positions and applies bounded target/original envelope ratios to the corresponding input bands. Neutral ratios leave the input unchanged. Gain ratios are limited to 0.25…4× and updated once per block; linked envelope detection preserves the stereo image.

This is coarse **input-envelope remapping**, not a fixed vowel bank, pitch shifting, or a high-resolution speech formant transposer. It redistributes existing spectral energy and cannot create a clean missing harmonic. The parallel bands overlap, so the displayed offset is a requested envelope offset, not a guarantee that each input peak moves exactly that many semitones. Tests show the 1120/560 Hz amplitude ratio changing from about 0.475 to 1.164 on a known broad spectral input at the upper offset, with the strongest measured band moving from 560 to 792 Hz (six semitones, despite the requested twelve). Broadband leads and textured basses are the intended material.

## Other stages and limits

- Triangle folding is symmetric and bounded. It is not oversampled; high drive intentionally aliases.
- A 31-tap windowed FIR Hilbert transform and quadrature oscillator produce signed frequency translation. Low-frequency image rejection is limited: the test at a 1 kHz carrier and approximately +80 Hz shift gives desired/image amplitudes approximately 0.0421/0.00788, or 14.6 dB rejection. This produces metallic coloration, not transparent pitch shifting.
- Fixed latency is 15 samples (0.34 ms at 44.1 kHz), including the neutral setting. Internal audio uses Q23 headroom, preserving approximately 24-bit input precision before filtering and rounding.
- The dynamic filter is a stable one-pole lowpass. Emphasis restores some high-frequency content; it is not a resonant peak. Controls and spectral gains glide at block rate, with a half-distance step each block.
- Final Q31 output saturates safely. There is no true-peak limiter or DC blocker.
- `dryLevel: 0` removes the SDK effect's dry contribution at full send. Partial sends still retain the track's parallel dry path, which this effect cannot distort, limit or align with its latency.

## Build and verification

```sh
python3 ../open-xy/sdk.py test --machine .
```

Validated against SDK `7fd8fdf1757e11ab6fdca915b09f1773d7611a31`. The process-level test checks neutral delay/precision, signed sidebands, spectral-envelope movement, folding harmonics, envelope-controlled filtering, stereo linking, extreme controls, dirty scratch reset and missing-buffer recovery. AddressSanitizer and UndefinedBehaviorSanitizer pass. Native compilation with official 2op FM also passes; this SDK's effect-only zero-instrument build has an unrelated unused-helper compiler error.

Per-send scratch is 512 bytes; bounded persistent state stays below the SDK's 472-byte buffered-effect allowance. Audio allocates nothing and performs no I/O. Coefficient generation is offline; the checked-in fixed-point tables have no runtime floating-point dependency.

`ci/machine.yml.example` is the generated CI template, stored inactive because the publishing OAuth credential lacks workflow scope. Local checks above were executed; no CI success is implied.

Hardware audition and revision-bound CPU profiling must happen before catalog submission. Firmware and stock content are never stored in this repository. The original design brief is preserved in Git history.
