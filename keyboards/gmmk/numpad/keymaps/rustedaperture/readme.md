# rustedaperture GMMK Numpad keymap (Vial)

- Vial with per-key, per-layer colours (`users/key_colours`), edited from the Key colours tab in
  [RustedAperture/vial-gui](https://github.com/RustedAperture/vial-gui/tree/key-colours).
- Slider sends MIDI CC 62 on channel 2.
- Layers 2 and 3 use Vial tap dances TD(0)-TD(6); set them up in Vial or load a saved layout.
- 4 KB of settings storage (wear-levelling backing 8 KB on the external SPI flash), so Vial gets
  32 tap dances, combos and key overrides.

Build: `make gmmk/numpad:rustedaperture`
