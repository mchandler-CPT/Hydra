# The Hydra — Active Workspace Memory

## Current Project Status
- **Core DSP Complete:** Monophonic additive core implemented in `Source/DSP/HydraEngine`.
  - 7 partials running on an arithmetic harmonic progression ($f, 2f, 3f \dots 7f$).
  - Zero-allocation hot loop processing without `fmod`.
  - Flange-prevention initialization using the first 7 prime numbers for starting phases: $2, 3, 5, 7, 11, 13, 17$.
- **Next Task:** Implement the block-rate macroeconomic scaling filters (Sigmoidal Bloom population arrays & Root Sum Square energy normalization) and integrate with MIDI inputs.

## Technical Decisions Ledger
1. **Separation of Framework:** `HydraEngine` accepts pure primitive floats to completely avoid coupling with JUCE APVTS classes.
2. **Deterministic Wave Morphing:** Quadrant-based wave linear interpolation running continuously from Sine $\rightarrow$ Triangle $\rightarrow$ Saw $\rightarrow$ 3-bit Crush.

## Known Rules & Constraints
- Never allow a virtual method call or heap memory allocation on the audio thread loop.
- Execute the test runner block inside `/tests` whenever modifications are written to `HydraEngine`.