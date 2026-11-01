# Formant Distortion for Open-XY

**Status: planning only.** This repository currently contains a design brief, not an implemented or installable effect. No DSP, firmware, presets or measured hardware results are included.

## Goal

Combine formant shifting, wavefolding, frequency shifting and dynamic filtering for unusual leads and vocal-like basses. This is an audio effect, not a new synthesizer.

## Proposed MVP signal flow

Stereo input → formant stage → wavefolder → signed frequency shifter → envelope-controlled filter → output.

Keep a fixed stage order for the first version. The formant stage must state what it actually does: a movable vowel/resonator bank is a useful first coloration mode, but it is not arbitrary source-envelope formant shifting. Evaluate a spectral-envelope shifting implementation before claiming that capability. Frequency shifting means additive Hz translation using a quadrature method, not musical pitch transposition. Smooth all stage controls and bound resonant feedback.

## Proposed controls

| Primary | Purpose |
|---|---|
| Formant | Move the formant structure |
| Fold | Wavefold amount |
| Shift | Signed frequency-shift amount |
| Dynamics | Envelope-to-filter amount |

Proposed alternate controls: base cutoff, resonance, envelope response and output. Start with neutral defaults and a fixed processing order. Exact formant algorithm and attainable quality remain to be established.

Control assignments are a starting design, not a published compatibility contract. Provide at least a neutral/default sound and a few useful settings when implementation begins.

## Routing and resources

Prefer `dryLevel: 0` for full-send processing, with the partial-send dry-path limitation documented. A spectral formant implementation may require the public scratch buffer. Budget quadrature filters, envelope tracking and any spectral workspace before promising all four stages simultaneously at maximum quality.

Target a standalone Open-XY `effect` machine. The SDK supplies the adapters; this repository owns the DSP, controls, artwork and sound tests. SDK changes, if needed, belong in separate upstream PRs.

The current SDK processes stereo Q31 blocks of 64 samples at 44.1 kHz. Unbuffered state is at most 476 bytes; buffered state at most 472 bytes, with an additional reduction when using a shift row. Audio callbacks cannot allocate, block, perform I/O, log or draw. Do not assume a host estimate proves device timing.

## Implementation handoff

1. Start from a current SDK checkout beside this repository and read its authoring/UX instructions.
2. Generate the official effect starter into a fresh sibling directory, then bring its scaffold into this repository while preserving this brief:
   ```sh
   python3 ../open-xy/sdk.py new --repository --kind effect --id formantdist --name "Formant Distortion" --out ../open-xy-formant-distortion-starter
   ```
3. Implement the smallest signal path, controls and default settings; keep firmware addresses and platform code out of the effect.
4. Run the machine's tests with `python3 ../open-xy/sdk.py test --machine .` once the scaffold is present. Follow the generated README if its layout changes.
5. Measure memory and worst-case device CPU, then test an explicitly authorized development build before catalog submission.

No catalog entry or upstream PR is part of this planning repository. No firmware or stock content may be committed here.

## Validation plan

Stage isolation and neutral paths; formant movement on known vowel-like signals; wavefold symmetry and bounds; frequency-shift sideband suppression; envelope response; DC rejection; resonance stability; parameter smoothing and worst-case CPU for the full chain.

Host numerical tests, native build verification, listening tests and revision-bound hardware CPU measurements are separate requirements. No browser audition support is assumed for effects.

## SDK reference

Planning checked against Open-XY staging revision [`64bd090`](https://github.com/etcetc000/open-xy/tree/64bd0900a08a134e274a8b8decbc8b734ca534dd) on 2026-11-01:

- [Effect API and scratch buffers](https://github.com/etcetc000/open-xy/blob/64bd0900a08a134e274a8b8decbc8b734ca534dd/docs/api.md#effects)
- [Authoring journey](https://github.com/etcetc000/open-xy/blob/64bd0900a08a134e274a8b8decbc8b734ca534dd/docs/first-machine.md)
- [UX conventions](https://github.com/etcetc000/open-xy/blob/64bd0900a08a134e274a8b8decbc8b734ca534dd/docs/ux.md)
- [Hardware profiling](https://github.com/etcetc000/open-xy/blob/64bd0900a08a134e274a8b8decbc8b734ca534dd/docs/profiling.md)

## Next step

Assign a dedicated implementation agent after the brief is reviewed. Do not start other effects or modify the SDK as part of this repository's initial implementation.
