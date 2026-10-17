# Copyright (c) 2026 Anton Komissarov
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_USB_MIDI)
  target_include_directories(app PRIVATE include)
  target_sources(app PRIVATE
    src/events/midi_key_state_changed.c
    src/midi.c
    src/midi_listener.c
    src/behaviors/behavior_midi_key_press.c
    src/usb_midi.c
    src/usb_midi_packet.c
  )
endif()
