#pragma once
#include <stdio.h>
#include <stdint.h>
#include "keyscanner.h"


typedef struct {
    uint8_t phase;
    uint8_t ticks;
} key_info_t;

typedef struct {
    key_info_t key_info[8];
    uint8_t state;  // debounced state
} debounce_t;


// We use a bitmask to determine whether to change the output state on a phase transition
// because it generates much more efficient code than a branching conditional

#define CHANGE_OUTPUT 0xFF

typedef struct {
    uint8_t next_phase;
    uint8_t expected_data;
    uint8_t unexpected_data_phase;
    uint8_t change_output_on_expected_transition;
    uint8_t timer;
} lifecycle_phase_t;

#include DEBOUNCE_STATE_MACHINE

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
        key->ticks--;
        if (key->ticks == 0) {
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
