/*
 * Copyright (c) 2024 The ZMK Contributors
 * Copyright (c) 2026 Anton Komissarov
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/midi_key_state_changed.h>
#include <zmk/midi.h>
#include <zmk/usb_midi.h>

static void listener_midi_key_pressed(const struct zmk_midi_key_state_changed *ev) {
    LOG_DBG("midi key pressed: 0x%04X", ev->key);
    int ret = zmk_midi_key_press(ev->key);
    if (ret < 0) {
        LOG_DBG("zmk_midi_key_press returned error %d, ignoring", ret);
        return;
    }
    struct zmk_midi_report *midi_report = zmk_get_midi_report();
    zmk_usb_send_midi_report(&midi_report->body);
}

static void listener_midi_key_released(const struct zmk_midi_key_state_changed *ev) {
    LOG_DBG("midi key released: 0x%04X", ev->key);
    int ret = zmk_midi_key_release(ev->key);
    if (ret < 0) {
        LOG_DBG("zmk_midi_key_release returned error %d, ignoring", ret);
        return;
    }
    struct zmk_midi_report *midi_report = zmk_get_midi_report();
    zmk_usb_send_midi_report(&midi_report->body);
}

int midi_listener(const zmk_event_t *eh) {
    const struct zmk_midi_key_state_changed *midi_key_ev = as_zmk_midi_key_state_changed(eh);
    if (midi_key_ev) {
        if (midi_key_ev->state) {
            listener_midi_key_pressed(midi_key_ev);
        } else {
            listener_midi_key_released(midi_key_ev);
        }
    }
    return 0;
}

ZMK_LISTENER(midi_listener, midi_listener);
ZMK_SUBSCRIPTION(midi_listener, zmk_midi_key_state_changed);
