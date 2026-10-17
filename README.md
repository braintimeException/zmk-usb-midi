# ZMK USB-MIDI Module

A standalone out-of-tree Zephyr/ZMK module providing class-compliant **USB-MIDI 1.0** output for ZMK keyboards running on Zephyr's standard USB device stack.

## Features

- **Class-Compliant USB MIDI 1.0:** Operates natively on Linux (ALSA/PipeWire), macOS, Windows, iOS, and Android with zero drivers or daemons.
- **Simultaneous Composite Device:** Runs alongside ZMK's standard USB HID Keyboard driver. Normal typing layers send text; music layers send musical notes.
- **10 Octaves of Notes:** Full range from `NOTE_C_0` (0x00) to `NOTE_G_10` (0x7F / 127).
- **MIDI Control Changes:** Native support for `SUSTAIN` (CC 64), `PORTAMENTO` (CC 65), `SOSTENUTO` (CC 66), and `ALL_NOTES_OFF` (CC 123 / Panic).
- **Standalone Module:** Requires zero modifications to core ZMK. Imported cleanly via `west.yml`.

---

## Installation

Add this module to your ZMK config repository's `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: braintimeException
      url-base: https://github.com/braintimeException
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-usb-midi
      remote: braintimeException
      revision: main
  self:
    path: config
```

---

## Configuration

Enable the module in your keyboard's `.conf` file (e.g. `config/klor.conf`):

```ini
CONFIG_ZMK_USB_MIDI=y
```

---

## Keymap Usage

Include the MIDI header at the top of your `.keymap` file:

```dts
#include <dt-bindings/zmk/midi.h>
```

Bind keys using the `&midi` behavior:

```dts
/ {
    keymap {
        compatible = "zmk,keymap";

        music_layer {
            bindings = <
                // Left Hand Bass / Chords
                &midi NOTE_C_2    &midi NOTE_G_2    &midi NOTE_A_2    &midi NOTE_F_2
                
                // Right Hand Melody (Middle C = NOTE_C_4)
                &midi NOTE_C_4    &midi NOTE_D_4    &midi NOTE_E_4    &midi NOTE_F_4
                
                // Panic / All Notes Off
                &midi ALL_NOTES_OFF
            >;
        };
    };
};
```

---

## MIDI Keycode Reference

* **Middle C:** `&midi NOTE_C_4` (MIDI note 60 / 0x3C)
* **Concert A (440Hz):** `&midi NOTE_A_4` (MIDI note 69 / 0x45)
* **Deep Bass:** `&midi NOTE_C_2` (MIDI note 36 / 0x24)
* **Sharps / Flats:** `NOTE_Cs_4` / `NOTE_Db_4`, `NOTE_Ds_4` / `NOTE_Eb_4`, `NOTE_Fs_4`, etc.
* **Controls:** `&midi SUSTAIN`, `&midi SOSTENUTO`, `&midi ALL_NOTES_OFF`

---

## License

MIT License. See [LICENSE](LICENSE) for details.
Based on original implementation work by SolidHal / SolidEva in ZMK PR #2338.
