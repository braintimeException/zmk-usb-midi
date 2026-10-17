# Agent Developer & Maintainer Guide: `zmk-usb-midi`

This document contains critical architectural context, build invariants, design decisions, and debugging runbooks for AI agents and human developers maintaining and extending the `zmk-usb-midi` module.

---

## 1. Module Purpose & Core Architecture

`zmk-usb-midi` is an out-of-tree ZMK module providing **class-compliant USB-MIDI 1.0 composite functionality** (simultaneous Keyboard HID + USB-MIDI streaming over a single USB-C connection).

### Architectural Philosophy: Self-Contained Event Pipeline
Unlike earlier prototypes (such as PR #2338) that modified core ZMK files like `app/src/endpoints.c`, this module is **completely decoupled and self-contained**:
1. **Behavior (`&midi`)**: Key presses trigger `behavior_midi_key_press.c`, emitting `zmk_midi_key_state_changed` events via ZMK's event manager.
2. **Listener (`midi_listener.c`)**: Subscribes to `zmk_midi_key_state_changed`, tracks pressed/released keys, and manages polyphony in `midi.c`.
3. **USB Transport (`usb_midi.c`)**: Packets encoded by `usb_midi_packet.c` (standard 4-byte USB-MIDI cable messages) are directly written to the USB endpoint `ZMK_USB_MIDI_EP_IN` via Zephyr's `usb_write()`.
4. **No Core Patches**: No modifications to ZMK's core endpoint router or transport abstractions are required.

### Target USB Stack: Zephyr Legacy Device Stack
- **Stack in Use**: `CONFIG_USB_DEVICE_STACK=y` (Zephyr's legacy USB device stack).
- **Rationale**: Core ZMK (as of v0.3.0 and mainline) is entirely built on Zephyr's legacy USB stack. Newer Zephyr drivers using the `device_next` USB stack cannot coexist with ZMK's current HID driver without disabling keyboard typing (`CONFIG_ZMK_USB=n`). This module preserves full composite typing and MIDI performance.

---

## 2. Directory Structure & Key Files

```
zmk-usb-midi/
├── CMakeLists.txt              # Build rules for Zephyr CMake system
├── Kconfig                     # Kconfig menu and configuration options
├── LICENSE                     # MIT License
├── README.md                   # User-facing overview and setup instructions
├── AGENTS.md                   # This developer and maintainer guide
├── zephyr/
│   └── module.yml              # Module declaration with build hooks and dts_root
├── dts/
│   ├── bindings/
│   │   └── behaviors/
│   │       └── zmk,behavior-midi-key-press.yaml  # DT binding for &midi
│   └── behaviors/
│       └── midi_key_press.dtsi # Behavior node definition (/omit-if-no-ref/ midi)
├── include/
│   ├── dt-bindings/zmk/
│   │   └── midi.h              # Keymap macros: NOTE_*, chords, CC, pitch bend
│   └── zmk/
│       ├── usb_midi.h          # USB descriptors, interface IDs, endpoint numbers
│       ├── usb_midi_packet.h   # 4-byte USB-MIDI packet definition
│       ├── midi.h              # State tracking and report structs
│       ├── midi_keys.h         # Type definitions (cin, key, value)
│       └── events/
│           └── midi_key_state_changed.h # Event definition
└── src/
    ├── usb_midi.c              # USBD class descriptors, status callback, usb_write
    ├── usb_midi_packet.c       # Packet parsing and serialization
    ├── midi.c                  # Polyphonic note tracking and state storage
    ├── midi_listener.c         # Event subscription and dispatch to USB
    ├── behaviors/
    │   └── behavior_midi_key_press.c # Driver for &midi behavior
    └── events/
        └── midi_key_state_changed.c  # Event registration
```

---

## 3. Invariants & Critical Pitfalls (DO NOT BREAK)

### ⚠️ Pitfall 1: Devicetree File Shadowing (`behaviors.dtsi`)
- **Rule**: **NEVER** create a file named `dts/behaviors.dtsi` inside this repository.
- **Why**: Zephyr adds `<module_root>/dts` to the preprocessor include path when `settings: { dts_root: . }` is set in `zephyr/module.yml`. If this module contains `dts/behaviors.dtsi`, it **shadows and completely replaces** ZMK's core `app/dts/behaviors.dtsi`.
- **Symptom**: Every build (central, peripheral, `settings_reset`) immediately fails with unknown symbol errors (`&kp`, `&mo`, `&bootloader`, `&trans` undefined).
- **Correct Pattern**: Place module-specific DTS includes in subdirectories, e.g., `dts/behaviors/midi_key_press.dtsi`, and explicitly include that file in keymaps.

### ⚠️ Pitfall 2: Split Keyboard Roles & USB Dependencies
- **Rule**: `menuconfig ZMK_USB_MIDI` **MUST** declare `depends on ZMK_USB` and `default n`.
- **Why**: In split keyboards (e.g., KLOR, Corne), configuration files (like `klor.conf`) apply to both the Central (Left) and Peripheral (Right) builds.
  - The **Central** half (`CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y`) runs the USB device stack (`CONFIG_ZMK_USB=y`, `CONFIG_USB_DEVICE_STACK=y`).
  - The **Peripheral** half (`CONFIG_ZMK_SPLIT_ROLE_CENTRAL=n`) operates exclusively as a BLE sender and has USB disabled (`CONFIG_ZMK_USB` is not set).
  - Utility shields like `settings_reset` do not enable USB HID.
- **Symptom**: If `ZMK_USB_MIDI` does not depend on `ZMK_USB`, the peripheral and reset builds attempt to compile `src/usb_midi.c` without `CONFIG_USB_DEVICE_STACK`, causing compilation errors on USB descriptor macros (`USBD_CLASS_DESCR_DEFINE`, `USBD_DEFINE_CFG_DATA`).
- **Correct Pattern**:
  ```kconfig
  menuconfig ZMK_USB_MIDI
      bool "ZMK USB-MIDI Class Support"
      depends on ZMK_USB
      select USB_DEVICE_STACK
      default n
  ```

### ⚠️ Pitfall 3: Module Declaration Hooks
- **Rule**: `zephyr/module.yml` **MUST** explicitly specify `cmake: .` and `kconfig: Kconfig`.
- **Why**: Without these keys, Zephyr registers the DTS root but ignores CMake and Kconfig files, causing `CONFIG_ZMK_USB_MIDI` to be unrecognized and sources omitted.
- **Correct Pattern**:
  ```yaml
  name: zmk-usb-midi
  build:
    cmake: .
    kconfig: Kconfig
    settings:
      dts_root: .
  ```

### ⚠️ Pitfall 4: CMake Source Inclusion Scoping
- **Rule**: Use `zephyr_include_directories(include)` and guard source targets with `target_sources_ifdef(CONFIG_ZMK_USB_MIDI app PRIVATE ...)`.
- **Why**: Ensures include headers (`<dt-bindings/zmk/midi.h>`) are visible globally while compilation of C sources is strictly limited to targets where `CONFIG_ZMK_USB_MIDI` is explicitly enabled.

---

## 4. How to Integrate into User Config Repositories

### Step 1: `config/west.yml`
Add the module remote and project:
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

### Step 2: `config/<shield>.conf`
Enable USB MIDI:
```ini
CONFIG_ZMK_USB_MIDI=y
```

### Step 3: `config/<shield>.keymap`
Include headers and map behaviors:
```dts
#include <dt-bindings/zmk/midi.h>
#include <behaviors/midi_key_press.dtsi>

// In keymap bindings:
&midi NOTE_C_4      // Middle C
&midi NOTE_G_4      // G4
&midi ALL_NOTES_OFF // Panic / kill all notes
```

---

## 5. Local Testing & Verification Runbook

### Verifying USB Enumeration (Linux)
When the central half is plugged in via USB:
```bash
# Verify USB descriptors (look for Audio Class 0x01 / Subclass 0x03 MIDI Streaming):
lsusb -v -d <vendor_id>:<product_id> | grep -E "bInterfaceClass|bInterfaceSubClass"

# Verify PipeWire / ALSA MIDI device detection:
pw-cli list-objects | grep -i midi
aseqdump -l
```

### Live MIDI Event Monitoring
Listen to MIDI messages in real time using ALSA utility:
```bash
# Replace 'XX:0' with the port number listed by aseqdump -l
aseqdump -p XX:0
```
When keys with `&midi` are pressed and released, `NoteOn` and `NoteOff` events with velocity values should appear instantly in the terminal.

### DAW & Synthesizer Verification
- **DAWs**: Open Ardour, Reaper, Bitwig, or Carla.
- **Softsynths**: Load SFizz (SoundFont/SFZ player) or Yoshimi/ZynAddSubFX.
- **Polyphony**: Press simultaneous chord keys (e.g., C Major chord) and verify multiple note voices trigger simultaneously without stuck notes.

---

## 6. Upstream Contribution Pathway (Issue #3352)

- **Target Upstream Issue**: [zmkfirmware/zmk Issue #3352](https://github.com/zmkfirmware/zmk/issues/3352).
- **Upstream Strategy**:
  1. Maintain and harden this standalone module with active daily use on personal builds.
  2. Once stability is proven across multiple keyboards and environments, open an upstream PR to `zmkfirmware/zmk`.
  3. Upstream integration will merge `include/` and `src/` directly into `app/`, routing MIDI reports either through the decoupled listener or directly into `endpoints.c`.
