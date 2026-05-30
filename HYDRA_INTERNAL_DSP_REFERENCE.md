# HYDRA INTERNAL DSP REFERENCE
## Engineering Source-of-Truth — v1.0

> **Audience:** Internal engineering. All numbers, equations, and code behaviours documented here are derived directly from audited source files. Do not edit without cross-referencing the originating `.cpp`/`.h` file cited in each section.

---

## Table of Contents

1. [Complete Signal Flow Architecture](#1-complete-signal-flow-architecture)
2. [The Oscillator & Wave-Shaping Matrix](#2-the-oscillator--wave-shaping-matrix)
3. [The HydraMacroMapper — Parameter-to-Partial Translation Layer](#3-the-hydramacromapper--parameter-to-partial-translation-layer)
4. [The Parallel Saturator](#4-the-parallel-saturator)
5. [The ZDF Ladder Filter](#5-the-zdf-ladder-filter)
6. [The Phase Disperser & Spatial Mechanics](#6-the-phase-disperser--spatial-mechanics)
7. [ADSR Envelope Architecture & Env Warp](#7-adsr-envelope-architecture--env-warp)
8. [Control Voltage & Modulation Mapping](#8-control-voltage--modulation-mapping)
9. [APVTS Parameter Registry](#9-apvts-parameter-registry)

---

## 1. Complete Signal Flow Architecture

### 1.1 High-Level Block Diagram

```
[MIDI Input]
     │
     ▼
[PluginProcessor::processBlock]
     │  block-rate parameter snapshot (atomic loads)
     │  MIDI dispatch → hydraEngine.noteOn / noteOff
     │
     ▼
[juce::dsp::Oversampling × 4]  ← filterHalfBandPolyphaseIIR, isMaximumQuality=true
     │  fs_os = fs_native × 4
     │
     ▼
[HydraEngine::renderBlock]  — operates entirely at fs_os
     │
     │  Per-sample hot loop:
     │    ├── [juce::ADSR]  volume envelope  →  envelopeGain
     │    ├── [juce::ADSR]  filter envelope  →  filterEnvAmt
     │    ├── dynamic cutoff computation (KB tracking + EGR modulation)
     │    ├── smoothedFrequency.getNextValue()  →  currentBaseFreq
     │    ├── for n = 0..6:
     │    │     oscillator[n].evaluateSample(morph)  →  oscillated
     │    │     voice[n].processDelaySample(oscillated) →  delayed
     │    │     spectral damping (fn > cutoff → attenuation)
     │    │     × amplitude × phaseIn × panL / panR
     │    │     → partialSamplesLeft[n] / partialSamplesRight[n]
     │    └── [HydraParallelSaturator::processSample]
     │          → (L, R) saturated pair  × envelopeGain × noteVelocity
     │
     │  Post-loop (block-level):
     └── [HydraPhaseDisperser::processBlock]  — 4-stage all-pass, identical on L and R
          │
          ▼
[ZdfLadderFilter × 2]  ← filterL / filterR, per-sample at fs_os
     │  cutoff modulated per-sample from filterCutoffBuffer[]
     │
     ▼
[HydraEngine::applyFilterOverload]  ← tanh saturation + √-compensated gain
     │
     ▼
[juce::dsp::StateVariableTPTFilter × 2]  ← mHpFilterL / mHpFilterR, highpass
     │  cutoff = smoothedHpCutoff (20ms smoothing)
     │
     ▼
[juce::dsp::Oversampling::processSamplesDown]  ← back to fs_native
     │
     ▼
[buffer.applyGain(gain)]  ← linear scalar 0.0 – 1.0
     │
     ▼
[pushVisualSample]  ← lock-free FIFO for waveform UI display (1024-slot ring buffer)
     │
     ▼
[Stereo Output Bus]
```

### 1.2 Oversampling Specification

| Parameter | Value |
|---|---|
| Class | `juce::dsp::Oversampling<float>` |
| Factor exponent | 2 (2² = **4×**) |
| Oversampling ratio | 4× |
| Filter type | `filterHalfBandPolyphaseIIR` |
| Quality flag | `isMaximumQuality = true` |
| Effective rate at 44.1 kHz | 176.4 kHz |
| Effective rate at 48 kHz | 192 kHz |

Oversampling is applied unconditionally on every call to `processBlock`. The `HydraEngine`, both `ZdfLadderFilter` instances, both `StateVariableTPTFilter` instances, and the `HydraPhaseDisperser` all operate at `fs_os`. The final `applyGain` and visual FIFO probe operate at native rate after decimation.

### 1.3 Mono Bus Handling

When the host presents a single-channel output bus (not normal operating mode, but handled), `monoRightScratch` (a `std::vector<float>` pre-allocated to `oversampledBlockSize` in `prepareToPlay`) serves as the right-channel render target. After `renderBlock`, the two buffers are summed with `0.5 × (L + R)` on a per-sample basis before the filter stage. No heap allocation occurs on the audio thread; the resize guard (`if (size < osNumSamples) resize(...)`) is the only potential allocation and is hit only in pathological host misconfigurations.

### 1.4 Filter Reset on Voice Silence

When `hydraEngine.getVoiceAmplitude()` returns `0.0f` (i.e., `lastEnvelopeGain == 0` — envelope fully decayed, note fully released), all four stateful filter objects (`filterL`, `filterR`, `mHpFilterL`, `mHpFilterR`) are hard-reset via their respective `reset()` methods. This eliminates DC offset accumulation and state machine artifacts between notes.

---

## 2. The Oscillator & Wave-Shaping Matrix

**Source files:** `HydraOscillator.h`, `HydraOscillator.cpp`

### 2.1 Shared Wavetable Construction

A single `SharedWavetables` instance is constructed once at static initialisation time (Meyer's singleton via `getSharedTables()`). Four tables of **4097 floats** each are built, where the final entry is a guard copy of index 0 enabling branchless linear interpolation at the wrap boundary.

| Table | Formula |
|---|---|
| `sineTable[i]` | `sin(2π · i / 4096)` |
| `triangleTable[i]` | `(2/π) · arcsin(sin(2π · i / 4096))` — pure bandlimited triangle via arcsine of sine |
| `sawtoothTable[i]` | `2 · (phase / 2π) - 1` — naive linear ramp, unity-peak |
| `crushedTable[i]` | `round(sawtoothTable[i] × 4) / 4` — 3-bit quantization (9 discrete levels: −1, −0.75, −0.5, −0.25, 0, 0.25, 0.5, 0.75, 1) |

Wavetable size `N = 4096`. All table access uses linear interpolation:

```
continuousIndex = (phase / 2π) × N
indexBase       = floor(continuousIndex)
fraction        = continuousIndex − indexBase
output          = table[indexBase] + fraction × (table[indexBase+1] − table[indexBase])
```

### 2.2 Phase Accumulator

Each `HydraOscillator` maintains a `double` precision phase accumulator `currentPhase ∈ [0, 2π)`.

**Advance step (called once per sample after evaluateSample):**
```
currentPhase += phaseIncrement.getNextValue()
while currentPhase >= 2π:  currentPhase -= 2π
while currentPhase <  0.0: currentPhase += 2π
```

The subtraction-loop wrap is branchless in the common case (one iteration at most per sample). No `fmod` is used to avoid heap/libm overhead on the audio thread.

**Phase increment target:**
```
targetIncrement = (2π × frequencyHz) / sampleRate
```

`phaseIncrement` is a `juce::LinearSmoothedValue<double>` with a **3 ms** smoothing ramp (`kPhaseIncrementSmoothingSeconds = 0.003`). This prevents clicks on rapid frequency updates. When `glidePitch = false`, the smoother is snapped to the new value immediately (`setCurrentAndTargetValue`); when `true`, it ramps (`setTargetValue`).

### 2.3 Four-Quadrant Wave Morphing

`evaluateSample(float morphState)` maps the `morph` parameter `[0.0, 3.0]` to four waveform quadrants via piecewise linear interpolation between adjacent table outputs at the current phase position:

| Morph Range | Waveform Blend | Equation |
|---|---|---|
| `[0.0, 1.0)` | Sine → Triangle | `sine + morph × (triangle − sine)` |
| `[1.0, 2.0)` | Triangle → Saw | `triangle + (morph−1) × (saw − triangle)` |
| `[2.0, 3.0]` | Saw → Bit-Crush | `saw + clamp(morph−2, 0, 1) × (crushed − saw)` |

At `morph = 0.0`: pure sine. At `morph = 1.0`: pure triangle. At `morph = 2.0`: pure naive saw. At `morph = 3.0`: fully bit-crushed saw (3-bit, 9-level).

All four table values are evaluated at the same interpolated phase index before the morph branching — only the blend output differs.

### 2.4 Partial Frequency Architecture

**Seven partials** are allocated at indices `n = 0..6`. Their frequencies are computed in `HydraEngine::renderBlock`:

```
fn = sanitizeTargetFrequency(currentBaseFreq × frequencyMultipliers[n])
```

`frequencyMultipliers` is populated by `HydraMacroMapper::computeTargets()` (see Section 3). `currentBaseFreq` is the current output of `smoothedFrequency`, a `LinearSmoothedValue<float>` whose reset time equals `glideTimeSeconds`.

`sanitizeTargetFrequency` clamps to `[8.0 Hz, 20000.0 Hz]` and guards against non-finite values, returning 440 Hz as the safe fallback.

**Morph-Tuned Pitch Scaling (`scaleMorph`):**
```
tuningDivisor = 12.0 + (scaleMorph × 12.0)
frequency = 440 × 2^((midiNote − 69) / tuningDivisor)
```
At `scaleMorph = 0.0`: standard 12-tone equal temperament. At `scaleMorph = 1.0`: 24-TET (quarter-tone tuning), compressing all intervals by half. This parameter is currently internal (not yet exposed in APVTS v1).

### 2.5 Spectral Damping

Each partial whose frequency `fn` exceeds the current ZDF filter cutoff `cutoffHz` is pre-attenuated inside the engine's hot loop before saturation:

```
damping = (fn ≤ cutoffHz) ? 1.0 : exp(−(fn − cutoffHz) × 0.0008)
```

The constant `spectralDampingS = 0.0008f` produces approximately −7 dB of attenuation when a partial sits 10 kHz above the cutoff, and approximately −20 dB at 28 kHz above. This is a first-order exponential falloff pre-filter that reduces aliasing energy from high harmonics near the filter cutoff, complementing the ZDF filter rather than replacing it.

### 2.6 Partial Stagger Delay Lines

Each `HydraPartialVoice` contains a **16384-sample circular delay buffer** (power-of-2, mask `0x3FFF`). Partials 1–6 receive a micro-delay to create spatial decorrelation between harmonics:

| Partial Index | Delay Time (s) | Delay at 176.4 kHz (samples) |
|---|---|---|
| 0 | 0.0000 | 0 |
| 1 | 0.0021 | ~370 |
| 2 | 0.0044 | ~776 |
| 3 | 0.0068 | ~1200 |
| 4 | 0.0093 | ~1641 |
| 5 | 0.0119 | ~2099 |
| 6 | 0.0145 | ~2558 |

These delays are computed in `configureDelay` and stored as integer sample counts. The delay buffer implements a simple fractional-free integer delay (circular buffer read-write). The micro-delays contribute to the sense of spatial width and harmonic richness even without the panning / Girth controls.

### 2.7 Env Warp Per-Partial Attack Stagger

Each partial has its own attack ramp (`phaseIn`), computed independently of the global ADSR:

```
if envWarp >= 0:
    partialAttackSeconds[n] = baseAttackSeconds + envWarp × 0.05 × n

if envWarp < 0:
    partialAttackSeconds[n] = baseAttackSeconds + |envWarp| × 0.05 × (6 − n)
```

`phaseIn[n] = clamp(samplesSinceNoteOn / partialAttackSamples[n], 0, 1)`

This is separate from the global ADSR envelope gain. At `envWarp > 0`, higher-index partials (higher harmonics) phase in progressively later — a rising harmonic bloom. At `envWarp < 0`, lower-index partials bloom later — the overtone cloud emerges before the fundamental.

---

## 3. The HydraMacroMapper — Parameter-to-Partial Translation Layer

**Source files:** `HydraMacroMapper.h`, `HydraMacroMapper.cpp`

`HydraMacroMapper::computeTargets(depth, girth, harmony, harmonicInversionIndex)` returns a `HarmonicTargetPacket` containing:
- `amplitudes[7]` — per-partial RSS-normalised amplitude scalars
- `panningPairs[7]` — (panL, panR) constant-power pairs
- `morphTargets[7]` — per-partial morph parameter targets `[0, 3]`
- `frequencyMultipliers[7]` — harmonic ratios relative to the fundamental
- `assignedHarmonicOrders[7]` — the harmonic order numbers used by the tilt algorithm

### 3.1 Depth (X) — Sigmoidal Harmonic Bloom

For each partial `n`:
```
amplitude_raw[n] = 1 / (1 + exp(−alpha[n] × (X − beta[n])))
```

| Partial | beta (bloom onset) | alpha (gate steepness) |
|---|---|---|
| 0 | 0.00 | 20 |
| 1 | 0.15 | 18 |
| 2 | 0.30 | 15 |
| 3 | 0.45 | 12 |
| 4 | 0.60 | 10 |
| 5 | 0.75 |  8 |
| 6 | 0.90 |  6 |

**Hard silence guard:** When `X < 0.02`, all partials `n > 0` are forced to zero amplitude. Only the fundamental is active below this threshold.

**RSS Normalisation (energy conservation):**
```
totalEnergy = sqrt(Σ amplitude_raw[n]²)
amplitude[n] = amplitude_raw[n] / totalEnergy
```
This maintains constant perceived loudness as harmonics bloom in. The more partials that are active, the lower each individual amplitude, preserving total RMS energy.

### 3.2 Harmony — Five-Recipe Crossfade System

Five harmonic recipes define the frequency multiplier for each of the 7 partial slots. Harmony `[0.0, 1.0]` linearly interpolates between adjacent recipes:

| Recipe Index | Multipliers `[1..7]` | Character |
|---|---|---|
| 0 | `[1, 2, 3, 4, 5, 6, 7]` | Pure harmonic series (Fourier) |
| 1 | `[1, 2, 3, 4, 6, 8, 12]` | Octave stack — dominant fundamental |
| 2 | `[1, 2, 2.4, 3, 4, 4.8, 6]` | Compressed intervals — Bohlen-Pierce proximity |
| 3 | `[1, 3, 5, 7, 9, 11, 13]` | Odd harmonics only — square-wave spectral content |
| 4 | `[1, 2, 4, 5.039, 6, 8, 16]` | Bell/inharmonic — stretched partials |

Computation:
```
scaledIndex = harmony × 4.0
baseIdx     = floor(scaledIndex)          (clamped to [0, 3])
frac        = scaledIndex − baseIdx

recipeIndex = harmonicOrdering[n] − 1     (reordering applied via Inversion mode)

baseHarmonic = recipe[baseIdx][recipeIndex]
             + frac × (recipe[baseIdx+1][recipeIndex] − recipe[baseIdx][recipeIndex])
```

### 3.3 Harmony Quantize (Stepped Mode)

When `harmonyQuantize = true`, the raw harmony value is snapped to the nearest of **13 stepped positions** before recipe interpolation, defined in `HydraHarmonySnap::steppedHarmonyPositions`:

```
[0.0000, 0.0833, 0.1667, 0.2500, 0.3333, 0.4167, 0.5000,
 0.5833, 0.6667, 0.7500, 0.8333, 0.9167, 1.0000]
```

Steps are spaced at `1/12` intervals — each step corresponds to a distinct crossfade segment between recipe transitions. The snap uses nearest-neighbour distance search (no rounding artefacts from DSP scaling).

### 3.4 Harmonic Inversion Modes

Six permutation modes re-order which harmonic recipe slot is assigned to which partial index. The partial's physical voice properties (panning, morph path) remain fixed; only the frequency ratio changes.

| Mode Index | Name | Order Applied to Partial Slots [0..6] |
|---|---|---|
| 0 | Linear | `[1, 2, 3, 4, 5, 6, 7]` — ascending natural order |
| 1 | Shuffle | `[1, 3, 2, 6, 4, 7, 5]` — alternating odd/even swap |
| 2 | Prime | `[1, 3, 5, 7, 2, 4, 6]` — odd harmonics first |
| 3 | Undertone | `[1, 7, 5, 3, 6, 4, 2]` — descending mirror |
| 4 | Octave | `[1, 2, 4, 6, 3, 5, 7]` — octave-biased spacing |
| 5 | Bell | `[1, 5, 7, 6, 3, 2, 4]` | — inharmonic cluster |

The Inversion parameter is a `juce::AudioParameterInt` (`[0, 5]`, integer steps). At block rate, the engine reads it and smoothly interpolates via `harmonicInversionSmoothed` (a `LinearSmoothedValue<float>` with 20 ms ramp), calling `applyMacroTargets()` each time the rounded index changes within the sample loop.

### 3.5 Girth (Y) — Spatial Spread Computation

Effective Girth is gated by Depth:
```
Y = rawGirth × clamp(X × 2.0, 0.0, 1.0)
```
Girth has zero effect when `X < 0.5`. Full Girth effect requires `X ≥ 0.5`.

**Fundamental (n = 0):** Always centre-panned at `(0.707, 0.707)`. Morph locked to `0.0`. Frequency multiplier locked to `1.0`.

**Odd-indexed partials (n = 1, 3, 5) — dynamic constant-power panning:**
```
harmonicIndex = n + 1
sign          = (harmonicIndex % 2 == 0) ? −1.0 : +1.0
thetaG        = Y × 0.08
angle         = ((harmonicIndex − 1) × π / 12) + thetaG
panOffset     = clamp(Y × sign × sin(angle), −1, 1)
normalizedPan = (panOffset + 1) / 2

panL = cos(normalizedPan × π/2)
panR = sin(normalizedPan × π/2)
```
The constant-power law (`cos/sin` pair) maintains equal loudness at all pan positions. The alternating `sign` sends odd-indexed groups to opposite sides of the stereo field.

**Even-indexed non-fundamental partials (n = 2, 4, 6) — fixed edge panning + micro-detuning:**

| Partial | Morph Target | Micro-Frequency Offset | Pan Destination |
|---|---|---|---|
| 2 | `Y × 3.0` | `+Y × 0.015` | Left |
| 4 | `Y × 3.0` | `−Y × 0.018` | Right |
| 6 | `Y × 3.0` | `+Y × 0.045` | Left |

Pan computation (blending from centre to hard edge):
```
panL = 0.707 + Y × (leftEdge − 0.707)
panR = 0.707 + Y × (rightEdge − 0.707)

where leftEdge = 1.0 for n=2,6 and rightEdge = 1.0 for n=4
```
At `Y = 0`: all even partials are centred `(0.707, 0.707)`. At `Y = 1.0`: n=2 and n=6 are fully left `(1.0, 0.707)`, n=4 is fully right `(0.707, 1.0)`.

The micro-detuning offsets ensure even partials have slightly different frequency ratios on each side of the stereo field, creating natural comb-filtering differences between L and R that contribute to perceived width.

**Morph targets by group:**
- Fundamental (n=0): morph = `0.0` (always pure sine)
- Odd partials (n=1,3,5): morph target = `Y × 2.0` → range `[0, 2]` → interpolates from sine to saw
- Even partials (n=2,4,6): morph target = `Y × 3.0` → range `[0, 3]` → interpolates from sine to bit-crush

### 3.6 Harmonic Tilt

Applied in `renderBlock` after amplitudes are resolved from `applyMacroTargets`. Energy is preserved via a normalisation constant.

**Tilt < 0 (roll-off higher harmonics):**
```
gain[n] = exp(tilt × (order[n] − 1) × 0.38)
```
At `tilt = −1.0`: partial 7 is attenuated by `exp(−0.38 × 6) ≈ 0.10` (−20 dB). Fundamental gain = `exp(0) = 1.0`.

**Tilt > 0 (boost higher harmonics):**
```
Fundamental (order ≈ 1):         tiltWeight = max(0.05,  1 − tilt × 0.90)
Low harmonics (order ≤ 3.01):    tiltWeight = 1 − tilt × 0.55 × (4 − order)/3
High harmonics (order > 3.01):   tiltWeight = 1 + tilt × (order−1)^0.60 × 0.22
Order 6 (exact):                 tiltWeight ×= (1 − tilt × 0.25)   ← extra attenuation
Order 7 (exact):                 tiltWeight ×= (1 − tilt × 0.65)   ← heaviest attenuation
```
At `tilt = +1.0`, the 7th harmonic is reduced by `~65%` of its base weight before the power normalisation step — an intentional spectral shaping choice to control harshness from the highest partial.

**Energy normalisation applied to the tilt result:**
```
tiltEnergyNorm = sqrt(energyBeforeTilt / energyAfterTilt)
finalAmplitude[n] = baseAmplitude[n] × tiltWeight[n] × tiltEnergyNorm
```

---

## 4. The Parallel Saturator

**Source files:** `HydraParallelSaturator.h`, `HydraParallelSaturator.cpp`

The saturator is called once per sample inside `renderBlock`, **before** the ADSR envelope gain is applied. It operates directly on the 7 partially-panned partial signals, grouping them into three spectral bands.

### 4.1 Band Grouping

| Band | Partial Indices | Nominal Harmonic Orders | Saturation Character |
|---|---|---|---|
| Low | 0, 1 | H1, H2 | Soft asymmetric cubic clip |
| Mid | 2, 3, 4 | H3, H4, H5 | Asymmetric tanh |
| High | 5, 6 | H6, H7 | Hard asymmetric limiter |

Partials are summed per band before saturation:
```
lowBandL  = partialL[0] + partialL[1]
midBandL  = partialL[2] + partialL[3] + partialL[4]
highBandL = partialL[5] + partialL[6]
```
(Same for right channel.)

### 4.2 Per-Band Transfer Functions

**Low band — asymmetric cubic soft clip:**
```
cubicSoftClip(x) = x − x³/3

if x ≥ 0:  processLowBand(x) = cubicSoftClip(x)
if x <  0:  processLowBand(x) = cubicSoftClip(x × 1.08)
```
The negative branch is pre-scaled by ×1.08 before the clip, introducing mild asymmetry. The cubic function `f(x) = x − x³/3` has gain = 1 at `x = 0`, and clips (peak) at `x = 1.0 → y = 0.667`. Slope goes to zero at `x = ±1`. Note: the function is technically unbounded for `|x| > 1` (it reverses direction) — the amplitude architecture is expected to keep levels below this threshold under normal operating conditions.

**Mid band — asymmetric tanh:**
```
if x ≥ 0:  processMidBand(x) = tanh(x × 1.4)
if x <  0:  processMidBand(x) = tanh(x × 1.22) × 0.92
```
Positive branch is driven harder (`×1.4`) producing more harmonic content. Negative branch is pulled back in both drive (`×1.22`) and output level (`×0.92`), introducing intentional second-harmonic asymmetry.

**High band — asymmetric hard limiter:**
```
if x ≥ 0:  processHighBand(x) = min(x, 0.70)
if x <  0:  processHighBand(x) = max(x, −0.58)
```
Hard clip at asymmetric thresholds (+0.70 / −0.58) with no overshoot protection. The `0.70 / 0.58` asymmetry is a `≈1.2` ratio, creating second-harmonic distortion on high-band content. This is the harshest stage and is effectively a bright edge-enhancement for the upper partials.

### 4.3 Output Assembly

```
outputL = saturatedLowL + saturatedMidL + saturatedHighL
outputR = saturatedLowR + saturatedMidR + saturatedHighR
```

This is then multiplied by `lastEnvelopeGain = envelopeGain × noteVelocity` to produce the final per-sample stereo signal from `renderBlock`.

---

## 5. The ZDF Ladder Filter

**Source files:** `ZdfLadderFilter.h`, `ZdfLadderFilter.cpp`

### 5.1 Architecture Overview

A four-pole ladder filter (Moog topology) implemented via the Zero-Delay Feedback (ZDF) formulation. ZDF eliminates the unit-delay in the feedback path that produces cutoff-dependent phase error in naïve digital ladder implementations. The filter uses the Trapezoidal Rule (bilinear transform) for each integrator stage.

Two separate instances — `filterL` and `filterR` — process the left and right channels independently in `processBlock`, each with their own state variables `{s1, s2, s3, s4, lastOutput}`. Both receive the same `modulatedCutoff` value from `filterCutoffBuffer[]` on each sample.

### 5.2 Cutoff Pre-Processing

```
maxSafeCutoff = min(21000 Hz, sampleRate × 0.475)
cutoffHz      = clamp(cutoffHz, 20 Hz, maxSafeCutoff)
```

The `0.475 × fs` ceiling prevents the bilinear pre-warping `g = tan(π·fc/fs)` from approaching infinity as `fc → fs/2`.

### 5.3 Resonance Feedback Cutoff Modulation

Before computing filter coefficients, the resonance feedback loop modifies the cutoff frequency using the filter's own last output sample:

```
maxSwing       = min(600 Hz, cutoffHz × 0.75)
modulatedCutoff = cutoffHz + (lastOutput × resonance × maxSwing)
clampedCutoff   = clamp(modulatedCutoff, 20 Hz, maxSafeCutoff)
```

`lastOutput` is `y4` from the previous sample. This creates a non-linear, signal-dependent cutoff modulation: as the filter self-oscillates or passes large amplitudes, its own output warps the cutoff upward (or downward, depending on the sign of `lastOutput`). This is responsible for the dynamic, alive quality of the filter at high resonance — the cutoff frequency "breathes" with the signal.

At `resonance = 4.0` and `cutoffHz = 1000 Hz`: `maxSwing = 750 Hz` (capped at 600 Hz), so the cutoff can modulate ±600 Hz per sample based on `lastOutput`.

### 5.4 Input Compensation

```
compensation     = 1.0 + 0.5 × resonance
compensatedInput = input × compensation
```

At `resonance = 4.0`: input is pre-amplified by ×3.0 before entering the ZDF solver. This compensates for the increasing insertion loss of the ladder at high resonance, maintaining perceived output level.

### 5.5 ZDF Coefficient Computation

```
g = tan(π × clampedCutoff / sampleRate)   ← bilinear pre-warp
h = g / (1 + g)                            ← integrator coefficient (Trapezoidal)
```

`g` is the normalised frequency coefficient. `h = g/(1+g)` is the one-pole TPT integrator gain. At cutoff = `fs/4`: `g = tan(π/4) = 1.0`, `h = 0.5`.

### 5.6 Resolved Input Computation

The ZDF solver computes the resolved input `u` analytically (solving the feedback loop simultaneously with the forward path):

```
baseFeedback = h³·s1 + h²·s2 + h·s3 + s4
denominator  = 1 + resonance × h⁴
u = (compensatedInput − resonance × tanh(baseFeedback)) / denominator
```

The `tanh(baseFeedback)` in the feedback path is the non-linear saturation of the feedback signal. This prevents the feedback from driving the filter into runaway at high resonance values, while simultaneously generating harmonic content in the resonance peak. At moderate resonance, `tanh(x) ≈ x` (linear). As resonance grows, the feedback clips and the resonance peak compresses.

### 5.7 Four TPT Integrator Stages

Each stage applies `tanh` non-linearity to the integrator input (the difference between the incoming signal and the stored state):

```
v1 = tanh(u  − s1);  y1 = s1 + g·v1;  s1 = y1 + g·v1
v2 = tanh(y1 − s2);  y2 = s2 + g·v2;  s2 = y2 + g·v2
v3 = tanh(y2 − s3);  y3 = s3 + g·v3;  s3 = y3 + g·v3
v4 = tanh(y3 − s4);  y4 = s4 + g·v4;  s4 = y4 + g·v4
```

The integrator state update `s = y + g·v` is the standard ZDF TPT state equation. The final output `y4` is the 4-pole (24 dB/octave) lowpass output.

```
lastOutput = isfinite(y4) ? y4 : 0.0
return lastOutput
```

### 5.8 Resonance Parameter Mapping

| APVTS Range | Effective Behaviour |
|---|---|
| `[0.0, 1.0]` | Sub-resonance to mild resonance peak. Filter rolls off cleanly. |
| `[1.0, 2.5]` | Growing resonance peak. `tanh` feedback begins compressing. |
| `[2.5, 3.5]` | Strong self-resonance territory. Filter begins to ring. Input compensation approaching ×2.75. |
| `[3.5, 4.0]` | Self-oscillation zone. `denominator = 1 + 4·h⁴` grows large at high cutoff, attenuating `u`; but `compensation = 3.0` and `maxSwing` feedback causes cutoff to breathe wildly. |

### 5.9 "Dead Zones" and Extreme Behaviour

**The Hidden Monster Sound:** At `resonance ≥ 3.5`, `filterOverload > 0.5`, `cutoff` in the `200–800 Hz` range, and an input signal with significant harmonic content:

1. `lastOutput × resonance × maxSwing` term warps the cutoff up and down per sample by hundreds of Hz.
2. The `tanh(baseFeedback)` clips the feedback, injecting odd harmonics into the resonance peak.
3. Each of the 4 `tanh(yn − sn)` stages further clips the integrator inputs, adding more harmonic content at the cutoff frequency.
4. `filterOverload` downstream amplifies and then tanh-clips the entire signal, compressing the tail of the resonance ring.

The result is a complex, wavefolding-style distortion that is not a standard ladder resonance effect. It sounds like the filter is chewing on itself. This territory is intentional and desirable for aggressive sound design.

**DC Accumulation Guard:** When `adsr.isActive()` returns false, `filterL.reset()` and `filterR.reset()` set all state variables to zero, preventing DC offset or NaN propagation between notes.

### 5.10 Filter Overload (Post-Filter Saturation)

Applied in `HydraEngine::applyFilterOverload` after the ZDF stage:

```
driveFactor      = 1.0 + (filterOverload × 4.0)
drivenSample     = sample × driveFactor
saturatedSample  = tanh(drivenSample)
compensationFactor = 1.0 / sqrt(driveFactor)
output           = saturatedSample × compensationFactor
```

At `filterOverload = 0.0`: no effect (driveFactor = 1.0, output = tanh(sample) / 1.0 ≈ sample for small signals). At `filterOverload = 1.0`: driveFactor = 5.0, drive = ×5, compensation = `1/√5 ≈ 0.447`. The `√`-reciprocal compensation is approximately a loudness-neutral level restoration (perceived loudness scales with RMS, which is related to the square of amplitude).

---

## 6. The Phase Disperser & Spatial Mechanics

**Source files:** `HydraPhaseDisperser.h`, `HydraPhaseDisperser.cpp`

### 6.1 Architecture

The phase disperser is a **4-stage first-order all-pass cascade** applied identically and independently to the left and right channels. It is called at the block level at the end of `HydraEngine::renderBlock`, after all oscillator summation and parallel saturation, but before the ZDF filter stage in `processBlock`.

### 6.2 All-Pass Transfer Function

Each stage implements the standard first-order all-pass filter:

```
y[n] = −c·x[n] + x[n−1] + c·y[n−1]
```

In Z-domain:
```
H(z) = (z⁻¹ − c) / (1 − c·z⁻¹)
```

This is an all-pass filter: `|H(e^jω)| = 1` for all ω. It passes all frequencies at unity amplitude but introduces frequency-dependent phase shift.

**Phase response per stage:**
```
φ(ω) = π − 2·arctan(c·sin(ω) / (1 − c·cos(ω)))
```

**Group delay per stage:**
```
τ(ω) = (1 − c²) / (1 + c² − 2c·cos(ω))  [in samples]
```

Where `ω = 2πf/fs`.

### 6.3 Fixed Coefficient Set

The four coefficients are compile-time constants:

| Stage | Coefficient `c` | Phase Character |
|---|---|---|
| 0 | +0.42 | Positive — phase lead at low frequencies |
| 1 | −0.15 | Negative — gentle phase reversal zone |
| 2 | +0.68 | Positive, strong — large low-frequency group delay |
| 3 | −0.33 | Negative — partial phase correction |

The cascade of these four stages creates a composite group delay curve with:
- Maximum group delay (longest time delay) concentrated in the **sub-200 Hz region** due to the large positive coefficients (0.42, 0.68).
- Shorter group delay at high frequencies.
- The negative coefficients (−0.15, −0.33) introduce group delay peaks at intermediate frequencies, creating a complex, non-monotonic phase response.

The net perceptual effect is that low-frequency content arrives slightly later in phase relative to high frequencies, which the ear interprets as added "weight" and "body" in the low end. This is distinct from adding bass gain — the spectral content is unchanged (all-pass), but the temporal relationship between frequency bands is modified.

### 6.4 Mono Compatibility

Since the **identical** coefficient array and **separate** state arrays (`allPassChainL[4]` and `allPassChainR[4]`) are applied independently to L and R, both channels receive the same phase transformation. A mono downmix (`L + R) / 2`) results in a signal that has received the same all-pass treatment as each individual channel. There is **no phase cancellation** between L and R from the disperser — mono compatibility is fully preserved.

### 6.5 User Control

The phase disperser in the current implementation uses fixed coefficients with no user-facing control. The dispersion depth, character, and frequency distribution are baked into the coefficient set. This is intentional: it is tuned as a passive "always-on" enhancer of the additive engine's spectral output, not a user-twiddlable effect.

---

## 7. ADSR Envelope Architecture & Env Warp

### 7.1 Volume ADSR

Instance: `juce::ADSR adsr` (inside `HydraEngine`).

Operating sample rate: `fs_os` (oversampled rate). This means envelope curvature is computed at the oversampled rate — sharper transients are possible than what the native sample rate would yield.

**Parameter ranges (APVTS, skewed NormalisableRange):**

| Parameter | Range | Skew | Default |
|---|---|---|---|
| Attack | 0.001 – 5.0 s | 0.5 | 0.1 s |
| Decay | 0.010 – 5.0 s | 0.5 | 0.3 s |
| Sustain | 0.0 – 1.0 | — | 0.8 |
| Release | 0.010 – 10.0 s | 0.5 | 0.5 s |

Skew factor `0.5` means the midpoint of the UI control maps to `sqrt(range)`, making the lower half of the knob travel across the perceptually important short-to-medium region and the upper half expand across longer tails.

The ADSR is called with `adsr.getNextSample()` per sample in the hot loop. Output is `envelopeGain ∈ [0.0, 1.0]`. Final per-sample amplitude:

```
lastEnvelopeGain = envelopeGain × noteVelocity
```

`noteVelocity` is the MIDI velocity of the triggering note, stored as a float `[0.0, 1.0]`.

### 7.2 Filter ADSR

Instance: `juce::ADSR filterAdsr`. Separate parameter set (`filterAttack`, `filterDecay`, `filterSustain`, `filterRelease`) — same ranges as volume ADSR. Output `filterEnvAmt ∈ [0.0, 1.0]` fed to the cutoff modulation formula (see Section 8.2).

### 7.3 Env Warp

`envWarp ∈ [−1.0, 1.0]`. Applied to both volume and filter ADSR attack times per sample in the hot loop:

```
warpedAttack = max(0.001, baseAttack × (1.0 + envWarp))
```

At `envWarp = +1.0`: attack time is doubled. At `envWarp = −1.0`: attack time is halved (toward minimum 1 ms). At `envWarp = 0.0`: no change.

This warping is applied sample-by-sample to the ADSR `setParameters` call, so the ADSR continuously tracks the warped time if Env Warp is modulated live.

Additionally, Env Warp controls the **per-partial attack stagger** (see Section 2.7), which is a separate mechanism operating on top of the ADSR.

---

## 8. Control Voltage & Modulation Mapping

### 8.1 Glide (Portamento)

```
glideTimeSeconds ∈ [0.0, 2.0]  (default 0.05)
appliedGlideTimeSeconds = max(0.003, glideTimeSeconds)
smoothedFrequency.reset(sampleRate, appliedGlideTimeSeconds)
```

`smoothedFrequency` is a `LinearSmoothedValue<float>`. On a new note while a note is already playing (`adsr.isActive() == true`), the engine sets a target value; the frequency ramps at the specified glide time. On the first note (no prior active note), the frequency is snapped directly with `setCurrentAndTargetValue`.

**Note stack (legato behaviour):** A 16-slot `noteStack[16]` array tracks pressed notes. On `noteOff`, if other notes remain held, the engine retunes to the top-of-stack note with glide enabled — conventional legato monophonic behaviour.

### 8.2 Filter Cutoff Modulation Chain

The per-sample cutoff written to `filterCutoffBuffer[]` is computed as:

```
baselineCutoff = smoothedCutoffHz.getNextValue()    ← 10ms smoothed APVTS value

octavesFromAnchor = (midiNoteNumber − 69) / tuningDivisor

trackedCutoffFloor = baselineCutoff × 2^(kbTrack × octavesFromAnchor)

dynamicCutoff = trackedCutoffFloor + (filterEnvAmt × egrAmount × 3500 Hz)

cutoffHz = clamp(dynamicCutoff, 20 Hz, 21000 Hz)
```

**KB Tracking (`kbTrack ∈ [0.0, 1.0]`):** At `kbTrack = 1.0`, the cutoff tracks note pitch exactly — one octave higher pitch = one octave higher cutoff. At `kbTrack = 0.5`, the cutoff tracks at half the rate. At `kbTrack = 0.0`: no tracking, absolute cutoff value.

**EGR Amount (`egrAmount ∈ [−1.0, 1.0]`):** Scales and optionally inverts the filter envelope's impact. At `egrAmount = +1.0`: note-on triggers the filter sweeping upward by up to +3500 Hz. At `egrAmount = −1.0`: inverse sweep downward by up to −3500 Hz. At `egrAmount = 0.0`: filter envelope has no effect on cutoff.

### 8.3 HP Filter

`mHpFilterL` and `mHpFilterR` are `juce::dsp::StateVariableTPTFilter<float>` instances configured as highpass filters.

```
hpCutoffRange: [20 Hz, 2000 Hz], skew 0.35, default 35 Hz
```

Cutoff is smoothed with `mSmoothedHpCutoff` (20 ms ramp, `LinearSmoothedValue<float>`). The smoothed value is applied sample-by-sample via `setCutoffFrequency` inside the post-ZDF loop. The TPT (Trapezoidal Partitioned) topology matches the ZDF approach — zero unit delay error.

---

## 9. APVTS Parameter Registry

All parameters are registered in `HydraAudioProcessor::createParameterLayout()` under the APVTS identifier `"PARAMETERS"`.

| Parameter ID | Display Name | Type | Range | Skew | Default | Unit |
|---|---|---|---|---|---|---|
| `depth` | Harmonic Bloom (X) | Float | 0.0 – 1.0 | — | 0.2 | norm |
| `girth` | Spatial Spread (Y) | Float | 0.0 – 1.0 | — | 0.0 | norm |
| `harmony` | Harmony | Float | 0.0 – 1.0 | — | 0.0 | norm |
| `gain` | Master Gain | Float | 0.0 – 1.0 | — | 0.5 | linear |
| `cutoff` | Filter Cutoff | Float | 20 – 21000 Hz | 0.2 | 20000 Hz | Hz |
| `res` | Resonance | Float | 0.0 – 4.0 | — | 1.0 | — |
| `attack` | Attack | Float | 0.001 – 5.0 s | 0.5 | 0.1 s | s |
| `decay` | Decay | Float | 0.010 – 5.0 s | 0.5 | 0.3 s | s |
| `sustain` | Sustain | Float | 0.0 – 1.0 | — | 0.8 | norm |
| `release` | Release | Float | 0.010 – 10.0 s | 0.5 | 0.5 s | s |
| `filterAttack` | Filter Attack | Float | 0.001 – 5.0 s | 0.5 | 0.1 s | s |
| `filterDecay` | Filter Decay | Float | 0.001 – 5.0 s | 0.5 | 0.3 s | s |
| `filterSustain` | Filter Sustain | Float | 0.0 – 1.0 | — | 0.7 | norm |
| `filterRelease` | Filter Release | Float | 0.001 – 5.0 s | 0.5 | 0.5 s | s |
| `egrAmount` | EGR Amount | Float | −1.0 – 1.0 (step 0.01) | — | 0.0 | bipolar |
| `envWarp` | Env Warp | Float | −1.0 – 1.0 | — | 0.0 | bipolar |
| `glideTime` | Glide Time | Float | 0.0 – 2.0 s (step 0.001) | — | 0.05 s | s |
| `tilt` | Harmonic Tilt | Float | −1.0 – 1.0 | — | 0.0 | bipolar |
| `inversion` | Inversion | **Int** | 0 – 5 | — | 0 | index |
| `hpCutoff` | HP Cutoff | Float | 20 – 2000 Hz | 0.35 | 35 Hz | Hz |
| `kbTrack` | KB Tracking | Float | 0.0 – 1.0 (step 0.01) | — | 0.0 | norm |
| `filterOverload` | Overload | Float | 0.0 – 1.0 (step 0.01) | — | 0.0 | norm |
| `harmonyQuantize` | Harmony Stepped | **Bool** | false/true | — | false | — |

**Atomic access:** All parameters are cached as `std::atomic<float>*` via `apvts.getRawParameterValue()` at construction time. Parameter reads in `processBlock` use `->load()` — a single atomic load per parameter per block, with no JUCE APVTS locking overhead on the audio thread.

**State Serialisation:** `getStateInformation` / `setStateInformation` uses `apvts.copyState()` → XML → binary blob. The preset name is stored as an additional property `metaLoadedPresetName` on the root `ValueTree` node alongside the parameter state.

---

*Document generated from source audit of: `PluginProcessor.cpp`, `HydraEngine.cpp/.h`, `HydraOscillator.cpp/.h`, `HydraMacroMapper.cpp/.h`, `HydraParallelSaturator.cpp/.h`, `ZdfLadderFilter.cpp/.h`, `HydraPhaseDisperser.cpp/.h`, `HydraHarmonySnap.h`. May 2026.*
