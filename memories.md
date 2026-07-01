# The Hydra — Active Workspace Memory

## Current Project Status
- **Core DSP Complete (SRP refactor):** Additive stack split into isolated modules under `Source/DSP/`:
  - `HydraOscillator` — 4097-entry shared wavetables (Sine/Triangle/Saw/3-bit Crush), `LinearSmoothedValue<double>` phase increment (3ms ramp), branchless subtraction-loop phase wrap.
  - `HydraMacroMapper` — sigmoidal bloom (`beta`/`alpha` arrays) + RSS amplitude normalization + constant-power `cos/sin` pan pairs + 5-recipe harmonic crossfade via `harmony` + 6 inversion modes.
  - `HydraParallelSaturator` — 7 partials grouped into Low (cubic soft-clip), Mid (tanh), High (hard asymmetric clip) bands; operates before ADSR envelope gain.
  - `HydraPhaseDisperser` — 4-stage first-order all-pass cascade, fixed coefficients [0.42, -0.15, 0.68, -0.33], identical L/R (fully mono-compatible).
  - `HydraEngine` — orchestrates 7 oscillators, smoothed voice targets, ADSR + filter ADSR, per-partial stagger delays (0–14.5 ms), spectral damping, harmonic tilt, env warp per-partial attack stagger, glide via `smoothedFrequency`, and `renderBlock`.
  - `ZdfLadderFilter` — 4-pole ZDF ladder (Moog topology), per-stage tanh non-linearity, resonance feedback cutoff modulation via `lastOutput`, input compensation `1 + 0.5×resonance`.
  - 7 partials, `frequencyMultipliers[]` from `HydraMacroMapper`, spectral damping constant `0.0008`.
  - Zero-allocation hot loop; branchless phase wrap (no `fmod` in audio loop).
- **PluginProcessor Integration — 100% Complete:**
  - **APVTS parameter matrix:** 23 parameters (22 float + 1 bool) — full list in `HYDRA_INTERNAL_DSP_REFERENCE.md` §9.
  - **4× oversampling:** `juce::dsp::Oversampling`, `filterHalfBandPolyphaseIIR`, `isMaximumQuality=true`.
  - **Post-filter chain:** ZDF ladder → Filter Overload (tanh + √-compensation) → HP SVT filter (smoothed, 20ms ramp).
  - **MIDI routing:** `noteOn` / `noteOff` + 16-slot legato note stack dispatched from `processBlock`.
  - **State serialization:** `getStateInformation` / `setStateInformation` using `apvts` XML binary helpers; `metaLoadedPresetName` stored as extra `ValueTree` property.
  - **Mono-right scratch buffer:** `monoRightScratch` allocated in `prepareToPlay`; mono buses render L/R internally then downmix `0.5×(L+R)` without heap allocation on the audio thread.
- **Internal Engineering Documentation:**
  - `HYDRA_INTERNAL_DSP_REFERENCE.md` — complete signal flow, oscillator math, filter internals, phase disperser transfer functions, modulation chain, full APVTS registry. Source of truth before README authoring.
- **Build & Verification:** Project compiles cleanly (Release). Catch2 suite (`BoutiqueTests`) passes all test cases — oscillator integrity, RSS energy conservation, and engine voice lifecycle.
- **Front-Panel UI — 100% Complete:**
  - `BoutiqueLookAndFeel` custom rotary skin (`Source/UI/`).
  - `PluginEditor` with APVTS `SliderAttachment` wiring. LookAndFeel nullified in destructor per host-safety rule.
- **Factory Presets:** 29 `.hydra` files in root `Presets/`. Installers deploy to the same path `PresetManager` uses: `%AppData%\bdEnergy\The Hydra\Presets` (Windows) and `~/Library/Application Support/bdEnergy/The Hydra/Presets` (macOS via postinstall copy from system staging).
- **Next Immediate Milestone:** End-to-end DAW validation (Standalone/VST3) and visual polish pass.

## Technical Decisions Ledger
1. **Separation of Framework:** `HydraEngine` accepts pure primitive floats to completely avoid coupling with JUCE APVTS classes.
2. **Deterministic Wave Morphing:** Quadrant-based wave linear interpolation: Sine → Triangle → Saw → 3-bit Crush. Morph parameter range [0.0, 3.0].
3. **Processor ↔ Engine Bridge:** `PluginProcessor` owns all framework concerns (APVTS, MIDI, preset XML, bus layout, oversampling, HP filter); `HydraEngine` is framework-agnostic.
4. **RSS Energy Conservation:** `HydraMacroMapper` normalises raw sigmoid amplitudes by `sqrt(Σa²)` so total energy is conserved as partials bloom in.
5. **Mono Compatibility:** Phase disperser uses identical coefficients on L and R with separate state — no inter-channel phase difference introduced. Pan architecture uses constant-power law.
6. **ZDF Filter Self-Modulation:** `lastOutput × resonance × maxSwing` warps cutoff per-sample, causing filter "breathing" at high resonance. Documented as intentional "Hidden Monster Sound" territory.

## Known Rules & Constraints
- Never allow a virtual method call or heap memory allocation on the audio thread loop.
- Execute the test runner block inside `/tests` whenever modifications are written to `HydraEngine` or related DSP components.
- All filter instances reset on voice silence (`getVoiceAmplitude() == 0`).
- Oversampling factor is 4× — always reason about cutoff, group delay, and timing values at `fs × 4` when inside `renderBlock` scope.
 