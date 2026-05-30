# The Hydra

> *Seven heads. One voice. Infinite territory.*

**The Hydra** is a monophonic additive synthesizer built for sound designers who want to construct timbre from first principles. It does not start with a waveform and cut it down - it builds sound up, partial by partial, using a seven-voice harmonic engine run through analog-modeled saturation and a non-linear Zero-Delay Feedback ladder filter that has a genuine personality of its own.

The factory preset bank is intentionally lean. **The Hydra is a canvas, not a library.** It rewards exploration and patience. A single slow pass of the XY pad from one corner to the other contains more sonic territory than most synthesizers offer across their entire preset bank.

---

## The Core Philosophy

Most synthesizers treat timbre as something to be carved away from - a rich waveform progressively thinned by filters. The Hydra works in the opposite direction. It begins in near-silence and adds energy to the spectrum one harmonic at a time, under precise logarithmic control. When you move the Depth knob, you are not adding gain - you are releasing partials from silence, one by one, each with its own spatial position, its own waveshape character, and its own place in the stereo field.

The result is a synthesizer where the act of building sound is the instrument itself.

---

## Signal Architecture

Understanding how The Hydra processes sound will permanently change how you approach programming it.

### 1 - Seven Independent Partials

At its core, The Hydra runs seven oscillators simultaneously - called **partials**. Each partial is a standalone voice tuned to a harmonic multiple of the note you play. By default they sit at 1×, 2×, 3×, 4×, 5×, 6×, and 7× the fundamental frequency - a standard overtone series. But the Harmony control radically restructures these relationships, and the Inversion modes re-route which partial carries which harmonic. More on both of those shortly.

Each partial has its own **waveshape**, its own **stereo position**, and a tiny built-in **micro-delay** (ranging from zero up to about 14.5 milliseconds for the highest partial). These fractional time offsets between partials are what give The Hydra its characteristic sense of three-dimensional space - even before you touch the Girth control.

### 2 - Harmonic Saturation in Three Bands

Before the signal reaches any filter, the seven partials are grouped into three spectral bands and individually saturated:

- **Low partials** (harmonics 1 and 2) pass through a **soft cubic clipper** - asymmetric, warm, even-order. This is where the body of the sound gets its weight.
- **Mid partials** (harmonics 3, 4, and 5) pass through a **hyperbolic tangent saturator** - smooth compression of peaks, asymmetric positive/negative drive. Classic analog-character density.
- **High partials** (harmonics 6 and 7) pass through an **asymmetric hard limiter** - a deliberate edge and bite that keeps the upper spectrum present without turning brittle.

This three-band architecture means that driving the engine hard via the Girth and Depth controls produces harmonically coherent distortion that sounds like analog saturation rather than digital clipping.

### 3 - The Phase Disperser

After the saturator, before the filter, every signal passes through an **always-on 4-stage all-pass filter cascade** - the Phase Disperser. This is not a tonal effect. It adds zero gain and removes zero frequencies. What it does is redistribute the **timing relationships** between different frequencies: low frequencies arrive at your ears a few samples later than high frequencies, in a non-linear pattern tuned specifically for low-end weight and spatial enhancement.

The effect is subtle but physical - like the way a room or a large speaker cabinet smears low frequencies in a way that reads as depth and dimension rather than muddiness. Bypassing it would feel like the low end of the sound shrinking inward.

> **Mono Compatibility Note:** The Phase Disperser applies the exact same transformation to both the left and right channels. There is zero inter-channel phase difference introduced. The Hydra is fully mono-compatible at all settings.

### 4 - The ZDF Ladder Filter

The final and most expressive stage is a **four-pole, 24 dB/octave lowpass ladder filter** modeled after the Moog topology using a Zero-Delay Feedback implementation. ZDF solves a fundamental problem with digital ladder filters: the artificial unit-delay error that makes cheap digital filters sound plasticky and thin. The Hydra's ZDF filter has no such delay - its feedback loop is solved analytically every sample, giving it the immediate, dynamic, pressure-sensitive character of hardware.

The filter's personality is not passive. At high Resonance, the filter **feeds its own output back into its cutoff frequency**, creating a signal-dependent modulation that makes the filter "breathe" with the incoming sound. This is described in detail in the Resonance section below.

### 5 - Post-Filter Processing

After the ladder filter, a **High-Pass filter** removes unwanted sub-bass buildup. Then a **linear gain stage** sets the final output level. The last step before the output bus is a **lock-free waveform visualization probe** feeding the XY pad's oscilloscope display in real time.

The entire engine from oscillators to decimation runs at **4× oversampling** using a half-band polyphase IIR filter. This means the filter's non-linearities, the saturation stages, and all frequency-dependent processing happen at a sample rate that all but eliminates aliasing artifacts from the harmonic distortion.

---

## Controlling the Beast: Parameter Guide

---

### The Timbral Engine: X / Y / Harmony

These three controls define the fundamental character of any patch. Start here.

---

#### **X - Harmonic Bloom**

`Range: 0.0 → 1.0 | Default: 0.20`

X is the master gate of The Hydra's harmonic universe. At zero, only the fundamental (partial 1) is active - a single pure tone. As X increases, the remaining six partials **emerge one by one** through individual sigmoid curves, each calibrated to enter at a specific point in the X range.

The critical detail: as more partials emerge, The Hydra's RSS energy normalization automatically redistributes amplitude so that total loudness is conserved. You are not adding volume when you turn X up - you are redistributing energy from silence into structure. This is why moving X feels alive rather than loud.

> Partials 2 through 7 begin emerging at X positions of approximately 0.15, 0.30, 0.45, 0.60, 0.75, and 0.90. Partial 7 is silent below X = 0.90. The "sweet zone" for most patches is **0.35 – 0.70**, where three to five partials are audible in dynamic balance.

---

#### **Y - Spatial Spread**

`Range: 0.0 → 1.0 | Default: 0.00`

> **Y has no effect when X is below approximately 0.50.** Girth is gated by Depth - you must first bloom the partials before you can spread them.

Y controls two things simultaneously for each of the six upper partials: **stereo position** and **waveshape**.

As Y increases:
- **Odd-indexed partials** (2nd, 4th, 6th harmonic) swing outward to opposite sides of the stereo field using constant-power panning. The panning angle shifts with harmonic order, so each odd partial lands at a different point on the L/R spectrum rather than all going to the same side. A subtle angular wobble is applied that scales with Y, preventing a static, mechanical spread.
- **Even-indexed partials** (3rd, 5th, 7th harmonic) move toward fixed left or right edges while simultaneously receiving a **micro-detune offset** - a small, Y-scaled frequency nudge that differs for each one. This creates natural comb-filtering differences between the L and R signals that read as true stereo width rather than artificial panning.
- All partials **morph their waveshape** as Y increases. Odd partials interpolate from sine toward a sawtooth. Even partials interpolate further, from sine all the way toward the bit-crushed waveform. At Y = 1.0, the outermost even partials are fully crushed - adding gritty harmonic texture at the edges of the field.

At low Y: a focused, almost monophonic center with gentle width. At high Y: a broad, evolving stereo image with character and motion. **The XY pad is the fastest way to explore this simultaneously with X.**

---

#### **Harmony**

`Range: 0.0 → 1.0 | Default: 0.00`

Harmony does not change the volume or position of partials. It changes **what frequency they are tuned to** - specifically, it crossfades between five entirely different harmonic recipes, each defining a different set of frequency ratios for the seven partial slots.

| Harmony Value | Recipe | Character |
|---|---|---|
| **0.00** | Pure Series | 1×, 2×, 3×, 4×, 5×, 6×, 7× - classic Fourier overtones |
| **0.25** | Octave Stack | 1×, 2×, 3×, 4×, 6×, 8×, 12× - fundamental-dominant, wide top |
| **0.50** | Compressed | 1×, 2×, 2.4×, 3×, 4×, 4.8×, 6× - intervals squeezed inward, dense cluster |
| **0.75** | Odd Harmonics | 1×, 3×, 5×, 7×, 9×, 11×, 13× - square-wave spectrum, nasal and penetrating |
| **1.00** | Bell Spectrum | 1×, 2×, 4×, 5.039×, 6×, 8×, 16× - inharmonic, metallic, bell-like decay |

Between each pair, ratios are linearly interpolated - so at Harmony = 0.125 you are halfway between Pure and Octave. The transition itself is often the most interesting sound design territory.

---

#### **Harmony Stepped** *(toggle)*

When enabled, the Harmony knob snaps to **13 evenly spaced detent positions** rather than moving continuously. Each detent is a distinct crossfade point between the five recipes. Use this when you want to lock to reproducible tonal states - useful for sequencing or live performance where repeatable positions matter.

---

### The Harmonic Matrix: Tilt & Inversion

These controls restructure the spectral balance and frequency assignment of the entire partial stack.

---

#### **Tilt - Harmonic Balance**

`Range: -1.0 → +1.0 | Default: 0.00`

Tilt is a spectral balance control that reshapes the amplitude weighting across partials while **preserving total energy**. No matter where Tilt sits, the overall loudness of the instrument does not change - energy is transferred, not added or removed.

- **Tilt negative (toward −1.0):** Lower harmonics become progressively louder; upper harmonics fade. The sound becomes rounder, darker, more fundamental-focused. At −1.0, the 7th harmonic is barely audible.
- **Tilt positive (toward +1.0):** Upper harmonics come forward while the fundamental softens. Harmonics 4 and above gain energy. Harmonics 6 and 7 are specifically attenuated toward positive extremes - a deliberate brightness-limiting decision that prevents the highest partials from turning harsh.

Think of Tilt as a single-knob spectral balance between the bass and treble register of your harmonic stack.

---

#### **Inversion - Harmonic Re-Routing**

`Range: 0 → 5 | Default: 0`

Inversion is one of The Hydra's most extreme controls. It re-assigns which harmonic recipe slot each partial voice occupies. The partials themselves - their pan positions, waveshapes, delay offsets - do not change. But the **frequency relationships** between them are completely scrambled.

| Mode | Name | What Changes |
|---|---|---|
| **0** | Linear | Default ascending order - natural overtone series |
| **1** | Shuffle | Odd and even harmonics swapped in alternating pairs |
| **2** | Prime | Odd harmonics loaded first, even harmonics second |
| **3** | Undertone | Reversed - highest ratio partial occupies the lowest voice slot |
| **4** | Octave | Octave harmonics (1, 2, 4, 6) clustered in the lower slots |
| **5** | Bell | Inharmonic cluster - maximally detached from natural series |

When combined with a mid-range Harmony setting and a non-zero Girth, different Inversion modes produce completely different chords and tonal clusters from the same patch. Use it as a macro-scale timbre switch in a live set.

---

### The Volatile Filter State: Cutoff / Resonance / Overload / HP

---

#### **Cutoff**

`Range: 20 Hz → 21,000 Hz (skewed) | Default: 20,000 Hz`

The master frequency threshold of the four-pole ladder filter. All harmonic content above this frequency is progressively attenuated at 24 dB per octave. At maximum, the filter is fully open.

Cutoff is modulated in real time by three independent sources that stack additively: the Cutoff knob's smoothed value, the **filter envelope** (scaled by EGR Amount), and **keyboard tracking** (scaled by KB Tracking). All three sources are summed per sample before the filter receives the control signal.

---

#### **Resonance**

`Range: 0.0 → 4.0 | Default: 1.0`

Resonance increases the feedback gain around the ladder filter's four pole stages, creating an emphasis peak at the cutoff frequency. But The Hydra's ZDF ladder does something more interesting than a standard digital resonance peak - the filter **feeds its own output signal back into its own cutoff frequency**, creating a dynamic, per-sample modulation of the filter frequency that makes the resonance peak move in response to the audio passing through it.

At moderate resonance the effect is subtle warmth. As resonance increases, the peak begins to sway and breathe with the signal. The filter feels alive.

> ⚠️ **Self-Oscillation Zone: Resonance above approximately 3.5**
>
> At high resonance, all four internal integrator stages engage their individual non-linear saturation, the feedback cutoff modulation becomes extreme (up to ±600 Hz per sample), and the filter begins to generate its own pitches independent of the oscillator signal. This is intentional. It is difficult to control and deliberately so. Pair it with the Overload stage for maximum disruption.

---

#### **Overload**

`Range: 0.0 → 1.0 (step 0.01) | Default: 0.00`

Overload is a post-filter saturation stage. It drives the filtered signal into a hyperbolic tangent clipper with gain compensation, adding harmonic density to whatever comes out of the ladder. At low values it adds subtle warmth. At high values it compresses the filter's resonance peaks and makes them crunch.

At maximum Overload with maximum Resonance in the self-oscillation zone, the filter's own pitches get saturated, folded, and compressed back into the signal - a recursive distortion architecture that produces sounds unlike anything available from a conventional filter section.

---

#### **HP Cutoff**

`Range: 20 Hz → 2,000 Hz (skewed) | Default: 35 Hz`

A High-Pass filter placed after the ZDF ladder. Use it to remove sub-bass buildup from resonant patches or to clean up the low end when using high Girth settings. The cutoff is parameter-smoothed with a 20 ms ramp to prevent clicks during live automation.

---

### The Temporal Controls: ADSR & Env Warp

The Hydra features two independent ADSR envelopes - one for volume, one for the filter - plus a Warp control that bends both simultaneously.

---

#### **Volume Envelope (Attack / Decay / Sustain / Release)**

`Attack: 1ms – 5s | Decay: 10ms – 5s | Sustain: 0.0 – 1.0 | Release: 10ms – 10s`

All time knobs use a square-root scaling curve: the first half of the knob's travel covers a few milliseconds to about half a second, and the second half expands into multi-second territory. This puts the most musically useful range directly under your fingertip.

---

#### **Filter Envelope (Filter Attack / Filter Decay / Filter Sustain / Filter Release)**

Identical time ranges to the volume envelope. Output is scaled by the **EGR Amount** control before it modulates the cutoff.

---

#### **EGR Amount**

`Range: -1.0 → +1.0 (step 0.01) | Default: 0.00`

Scales the filter envelope's impact on cutoff frequency, with a range of ±3,500 Hz. At positive values: note-on triggers a sweeping upward opening of the filter. At negative values: the sweep is inverted - the filter closes on attack and opens on release. At zero: the filter envelope has no effect on cutoff.

---

#### **Env Warp**

`Range: -1.0 → +1.0 | Default: 0.00`

Env Warp simultaneously scales the attack time of both the volume and filter envelopes. But its most interesting behavior happens at the partial level.

**At positive Env Warp:** Each of the seven partials begins phasing in at a progressively later time after note-on - the fundamental arrives first, then harmonics 2, 3, 4 and so on, each blooming in turn with up to 5% of the global attack time added per partial index. The result is a rising harmonic bloom: the note starts as a sine and unfolds into its full harmonic content over the attack phase.

**At negative Env Warp:** The order is inverted - the highest harmonic arrives first and the fundamental anchors last. Patches phase in from the top of the spectrum downward, creating an ethereal shimmer-before-body effect.

This per-partial attack stagger is completely separate from the global ADSR. It operates on a simple ramp per partial independent of the envelope curve, so the two effects layer additively.

---

#### **Glide Time**

`Range: 0.0 – 2.0s (step 1ms) | Default: 0.05s`

Portamento time for the fundamental frequency. When a second note is triggered while one is already playing, the pitch slides from the previous note to the new one over this duration. The Hydra uses legato monophonic note stacking - releasing a held note while holding a lower one will re-trigger the lower pitch with glide applied.

---

#### **KB Tracking**

`Range: 0.0 – 1.0 (step 0.01) | Default: 0.00`

At 0.0: the filter cutoff is fixed at the Cutoff knob position regardless of the note played. At 1.0: the filter opens proportionally as pitch rises - one octave higher note equals one octave higher cutoff frequency. Intermediate values give partial tracking, useful for taming the filter's behavior across the keyboard range.

---

### Harmonic Tilt Controls

#### **Tilt**

`Range: -1.0 → +1.0 | Default: 0.00`

*(See Harmonic Matrix section above.)*

---

## Quick-Start Navigation & Cheat Sheet

---

### The XY Pad

The large panel at the center of the interface is a **live XY controller** that maps directly to the two most important parameters in the engine simultaneously.

| Axis | Parameter | Direction |
|---|---|---|
| **Horizontal (X)** | Harmonic Bloom | Left = silent / right = full harmonic stack |
| **Vertical (Y)** | Spatial Spread | Bottom = centered / top = maximum spread |

The XY pad also displays a **live oscilloscope** of the output signal. The waveform trace and its glow color respond dynamically to the current X/Y position, giving you immediate visual feedback on harmonic density and stereo activity.

**Interaction:**
- **Click and drag** anywhere on the pad to move the XY cursor.
- The cursor position updates both Depth and Girth parameters simultaneously as a complete gesture - fully automatable and DAW-recordable.
- The position crosshairs (dashed center lines) remain visible to give you spatial reference while dragging.

---

### Knob Interactions

| Action | Result |
|---|---|
| **Click + Drag (vertical)** | Adjust parameter value |
| **Double-click** | Reset knob to its factory default value |
| **Right-click** | DAW automation menu (host-dependent) |

---

### Programming Approach - Recommended Starting Points

**For pads and sustained textures:**
Start with X at 0.40, Y at 0.00, Harmony at 0.00. Slow Attack to 1–2s. Pull Cutoff to around 4–6 kHz. Slowly raise Y while listening to the stereo field open up. Then start nudging Harmony and observe how the chord quality of the partial cluster changes.

**For bass and monophonic leads:**
X at 0.55–0.65, Y at 0.15–0.25, Tilt at −0.30 to pull energy toward the fundamental. Use the filter with moderate resonance (1.5–2.5) and a small EGR Amount sweep. Glide at 80–150ms.

**For noise and destruction:**
X to maximum. Harmony to 0.75–1.00 (odd harmonics or bell spectrum). Resonance above 3.5. Overload above 0.60. Env Warp to either extreme. The filter will begin producing its own pitch material independent of the keyboard.

---

## Build & Technical Requirements

- **Format:** VST3 (Windows / macOS)
- **Framework:** JUCE 8 via CMake
- **Voices:** 1 (monophonic with legato note stack)
- **Latency:** Determined by host buffer size + 4× oversampler decimation filter latency
- **Tail Length:** 10 seconds (reported to host for DAW bouncing)
- **Preset Format:** `.hydra` (XML binary, APVTS state serialization)

---

*The Hydra - designed and built using the JUCE framework.*
