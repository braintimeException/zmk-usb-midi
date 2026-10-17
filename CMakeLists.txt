# Copyright (c) 2026 Anton Komissarov
# SPDX-License-Identifier: MIT

zephyr_include_directories(include)

target_sources_ifdef(CONFIG_ZMK_USB_MIDI app PRIVATE
  src/events/midi_key_state_changed.c
  src/midi.c
  src/midi_listener.c
  src/behaviors/behavior_midi_key_press.c
  src/usb_midi.c
  src/usb_midi_packet.c
)
