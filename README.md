# Nick Toneprint

Nick Toneprint is a first-pass custom guitar/vocal/synth texture plugin. It combines soft drive, tone shaping, stereo width, and a short modulated slap delay into one simple effect.

Logic Pro loads Audio Unit plugins, so the macOS target is an AU `.component`. The same project can also build VST3 and a standalone app through JUCE.

The plugin uses a compact fixed-size knob grid so all current controls are visible at once.

## Controls

- Drive: input push into the soft saturation stage.
- Tone: darker to brighter post-drive shaping.
- Slap: short delay time.
- Feedback: repeat amount.
- Drift: delay modulation depth.
- Drift Rate: modulation speed.
- Width: stereo spread for the wet path.
- Mix: dry/wet blend.
- Verb Mix: amount of drift-fed reverb bloom.
- Decay: reverb tail length.
- Size: reverb tank spread.
- Damping: darker to brighter tail absorption.
- PreDelay: space before the reverb bloom arrives.
- Shimmer: octave-like shimmer feed into the reverb tank.
- Shimmer Tone: darker to brighter shimmer voice.
- Drift Send: how much of the drift/slap path feeds the reverb.
- Output: final level trim.

## Presets

- Tape Ghost: tight slap, mild drive, and low drift for always-on guitar or vocal thickening.
- Oscillator Slap: the first magic zone; slow slap, around 30% feedback, and long drift movement.
- Blue Drift: wide drift feeding a long, soft shimmer wash for chords, swells, and ambient beds.
- Vocal Mirage: lower drive, wider modulation, and controlled bloom for a vocal double/dream smear.
- DI Fever Dream: hotter drive and unstable drift for leads, noise parts, and synth-like guitar lines.
- Cloud Machine: maximum bloom with big verb, clear shimmer, and enough drift send to turn notes into atmosphere.

## Local DSP Test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Build For Logic Pro On macOS

Prerequisites:

- Xcode
- CMake
- Internet access for CMake to fetch JUCE

Build:

```bash
cmake -S . -B build-mac -G Xcode -DNICK_TONEPRINT_BUILD_PLUGIN=ON
cmake --build build-mac --config Release
```

The AU component should be copied after build by JUCE. If needed, install it manually:

```bash
mkdir -p ~/Library/Audio/Plug-Ins/Components
cp -R build-mac/plugin/NickToneprint_artefacts/Release/AU/Nick\\ Toneprint.component ~/Library/Audio/Plug-Ins/Components/
```

Then open Logic Pro, go to Plug-in Manager, and rescan if Logic does not pick it up automatically.

## Next Musical Iterations

- Replace the generic JUCE editor with a custom interface.
- Add named presets for guitar DI, lead vocal, and synth pad.
- Tune the saturation curve against real recordings.
- Add optional tempo-sync for slap time.
