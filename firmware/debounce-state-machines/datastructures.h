#pragma once
#include <stdint.h>

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
    uint8_t unexpected_data_phase;
    uint8_t expected_data;
    uint8_t change_output_on_expected_transition;
    uint8_t timer;
} lifecycle_phase_t;


