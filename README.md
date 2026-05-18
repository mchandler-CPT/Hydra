# The Hydra

A boutique 7-oscillator monophonic additive synthesizer built in C++ using the JUCE framework and CMake.

## Architectural Architecture

Unlike traditional subtractive synthesizers that rely on filtering rich geometric waveforms, **The Hydra** constructs its timbre from the ground up by manipulating the harmonic series directly.

### Core Pillars
- **Harmonic Scaffold:** Features 7 independent oscillator partials initialized using a prime-number starting phase matrix ($\phi_n = \frac{2\pi}{p_n}$) to eliminate static digital phase cancellation and flanging.
- **Ecological Sigmoidal Bloom:** Partials transition into existence utilizing non-linear logistic population growth vectors, providing an organic, highly pressurized harmonic evolution.
- **Metabolic Loudness Management:** Incorporates an absolute Root Sum Square (RSS) normalization engine to conserve total system energy, allowing upper overtones to organically draw power from the fundamental without increasing overall peak digital level.
- **Modular DSP Design:** Complete isolation between real-time, zero-allocation synthesis modules and the JUCE host lifecycle framework.

## Project Structure
- `Source/DSP/`: Core audio algorithms and the additive synthesis engine matrix.
- `Source/UI/`: Custom vector-drawn interfaces and LookAndFeel profiles.
- `Juce/`: Managed framework submodule wrapper.