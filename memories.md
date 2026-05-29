# The Hydra — Active Workspace Memory

## Current Project Status
- **Core DSP Complete (SRP refactor):** Additive stack split into isolated modules under `Source/DSP/`:
  - `HydraOscillator` — per-partial phase accumulation and 4-quadrant waveshaper (Sine → Triangle → Saw → Bit-Crush).
  - `HydraMacroMapper` — sigmoidal bloom (`beta`/`alpha` arrays) + RSS amplitude normalization + constant-power pan pairs from `depth`/`girth`.
  - `HydraEngine` — orchestrates 7 oscillators, smoothed voice targets, and `renderBlock`.
  - 7 partials on arithmetic harmonics ($f \dots 7f$) with prime-number phase offsets: $2, 3, 5, 7, 11, 13, 17$.
  - Zero-allocation hot loop; branchless phase wrap (no `fmod` in audio loop).
- **PluginProcessor Integration — 100% Complete:**
  - **APVTS parameter matrix:** `depth`, `girth`, `morph`, `gain` with atomic `getRawParameterValue` caches and block-rate `HydraEngine` setters.
  - **MIDI routing:** `noteOn` / `noteOff` dispatched from `processBlock` via `juce::MidiBuffer` iteration.
  - **State serialization:** `getStateInformation` / `setStateInformation` using `apvts` XML binary helpers for DAW preset recall.
  - **Mono-right scratch buffer:** `monoRightScratch` allocated in `prepareToPlay`; mono buses render L/R internally then downmix without heap allocation on the audio thread.
- **Build & Verification:** Project compiles cleanly (Release). Catch2 suite (`BoutiqueTests`) passes all test cases — oscillator integrity, RSS energy conservation, and engine voice lifecycle.
- **Front-Panel UI — 100% Complete:**
  - `BoutiqueLookAndFeel` custom rotary skin (`Source/UI/`).
  - `PluginEditor` 500×250 four-column console: DEPTH | GIRTH | MORPH | GAIN with APVTS `SliderAttachment` wiring.
  - LookAndFeel nullified in destructor per host-safety rule.
- **Next Immediate Milestone:** End-to-end DAW validation (Standalone/VST3) and visual polish pass.

## Technical Decisions Ledger
1. **Separation of Framework:** `HydraEngine` accepts pure primitive floats to completely avoid coupling with JUCE APVTS classes.
2. **Deterministic Wave Morphing:** Quadrant-based wave linear interpolation running continuously from Sine $\rightarrow$ Triangle $\rightarrow$ Saw $\rightarrow$ 3-bit Crush.
3. **Processor ↔ Engine Bridge:** `PluginProcessor` owns all framework concerns (APVTS, MIDI, preset XML, bus layout); `HydraEngine` remains a framework-agnostic DSP orchestrator.

## Known Rules & Constraints
- Never allow a virtual method call or heap memory allocation on the audio thread loop.
- Execute the test runner block inside `/tests` whenever modifications are written to `HydraEngine` or related DSP components.
 