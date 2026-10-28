# rustedaperture GMMK Numpad keymap (Vial)

- Vial with per-key, per-layer colours (`users/key_colours`), edited from the Key colours tab in
  [RustedAperture/vial-gui](https://github.com/RustedAperture/vial-gui/tree/key-colours).
- Slider sends MIDI CC on channel 2 to a list of targets (`users/slider_targets`), edited from the Slider
  tab in the same app. Default: one target, CC 62.
- Layer 1 `7` is `SLIDER_NEXT`: switches to the next target and flashes the side LEDs in its colour.
- Layers 2 and 3 use Vial tap dances TD(0)-TD(6); set them up in Vial or load a saved layout.
- 4 KB of settings storage (wear-levelling backing 8 KB on the external SPI flash), so Vial gets
  32 tap dances, combos and key overrides.

Build: `make gmmk/numpad:rustedaperture`
