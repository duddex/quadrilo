# TODO

## Verification

- [ ] Load the VST3 in a DAW and listen: delay times, feedback, damp, pan, mix and bypass.
- [ ] Check Tempo Sync against the host tempo, including tempo changes while playing.
- [ ] Check the per-tap Note dropdown that replaces the Delay knob when Sync is on (layout and behaviour).
- [ ] Check automation, saving and reloading a project, and plugin state restore.
- [ ] Test in more than one host (and with different sample rates and block sizes).

## Sound and behaviour

- [ ] Review the parameter ranges and tapers (feedback, damp, Delay knob) by ear.
- [ ] Review the parameter smoothing times.
- [ ] Calculate the reported tail length from the longest delay and the feedback setting.
- [ ] Decide whether very long tempo-synced notes at slow tempos should be allowed beyond the 2 s delay line.

## Features

- [ ] Factory presets, and preset save/load.
- [ ] Tooltips on the controls.
- [ ] Resizable window.
- [ ] Optional: animated tape reels.

## Build and release

- [ ] Retake `docs/screenshot.png` so it shows the current defaults (Feedback 50 %).
- [ ] Tag and publish `v0.2.0` (v0.1.0 does not include the tests or the current engine).
- [ ] Write release notes by hand instead of the auto-generated ones.
- [ ] Add macOS and Linux builds to the CI workflow.
- [ ] Add a `.gitattributes` file to settle line endings.

## Before promoting the plugin

- [ ] Search KVR, plugin directories and trademark databases for the name "Quadrilo".
- [ ] Add GitHub repository topics and a social preview image.
