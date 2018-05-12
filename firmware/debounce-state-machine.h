#pragma once
#include <stdio.h>
#include <stdint.h>
#include "keyscanner.h"
#include "debounce-state-machines/chatter-defense.h"

static uint8_t debounce(uint8_t sample, debounce_t *debouncer) {
    uint8_t changes = 0;
    // Scan each pin from the bank
    for(int8_t i=0; i< COUNT_INPUT; i++) {
        key_info_t *key = debouncer->key_info+i;
        lifecycle_phase_t current_phase =  lifecycle[key->phase];

        if (!!(sample & _BV(i)) != current_phase.expected_data) {
            // if we get the 'other' value during a locked window, that's gotta be chatter
            if (key->phase != current_phase.unexpected_data_phase) {
                key->phase = current_phase.unexpected_data_phase;
                key->ticks= lifecycle[key->phase].timer;
                continue;
            }
        }

        // do not act on any input during the locked off window
        if (key->ticks > 0) {
            key->ticks--;
        } else {
            if (  (key->phase != current_phase.next_phase)) {
                key->phase = current_phase.next_phase;
                key->ticks = lifecycle[key->phase].timer;
                changes |= _BV(i) & lifecycle[key->phase].change_output_on_expected_transition;
            }
        }
    }

    debouncer->state ^= changes;
    return changes;
}
