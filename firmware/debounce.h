#pragma once

#include <stdint.h>


#define EXPECT_FALSE 0
#define EXPECT_TRUE 1


// #define DEBOUNCE_MINIMUM_MS 5
// #define KEYSCAN_INTERVAL_TO_MS_MULTIPLIER 0.032
// #define KEYSCAN_TIME  (KEYSCAN_INTERVAL  * KEYSCAN_INTERVAL_TO_MS_MULTIPLIER)
// #define DEBOUNCE_MINIMUM_CYCLES ((DEBOUNCE_MINIMUM_MS/KEYSCAN_TIME)+1)

#define DEBOUNCE_COUNTER_LIMIT 6
#define DEBOUNCE_THRESHOLD 3




static int8_t debounce_integrator_ceiling = DEBOUNCE_COUNTER_LIMIT;
static int8_t debounce_integrator_floor = (0-DEBOUNCE_COUNTER_LIMIT);
static int8_t debounce_toggle_on_threshold = DEBOUNCE_THRESHOLD;
static int8_t debounce_toggle_off_threshold =  (0-DEBOUNCE_THRESHOLD);


/*
each of these 8 bit variables are storing the state for 8 keys

so for key 0, the counter is represented by db0[0] and db1[0]
and the state in state[0].
*/
typedef struct {
    int8_t counters[8];
    uint8_t state;  // debounced state
} debounce_t;

/**
 * debounce --
 *    The debouncer is based on a stacked counter implementation, with each bit
 *    getting its own 2-bit counter. When a bit changes, a call to debounce
 *    will increment that bit's counter. When it overflows, the change is
 *    comitted to the final debounced state and the changed bit returned.
 *
 *    Because each key's counter and state are stored in this stacked way,
 *    8 keys are processed in parallel at each operation.
 *
 * args:
 *    sample - the current state
 *    debouncer - the state variables of the debouncer
 *
 * returns: bits that have changed in the final debounced state
 *
 * handy XOR truth table:   A   B   O
 *                          0 ^ 0 = 0
 *                          0 ^ 1 = 1
 *                          1 ^ 0 = 1
 *                          1 ^ 1 = 0
 *
 * This is used below as a difference detector:
 *   if A ^ B is true, A and B are different.
 *
 * And a way to flip selected bits in a variable or register:
 *   Set B to 1, then A ^ B = !A
 */
static uint8_t debounce(uint8_t sample, debounce_t *debouncer) {
    uint8_t changes = 0;


    // Scan each pin from the bank
    for(int8_t i=0; i<=7; i++) {
        // If the pin is on
        if (__builtin_expect(( sample & _BV(i)), EXPECT_FALSE) ) {  // It's probably not on


            // If the counter for this key is below the threshold
            // In this case, the threshold is half the number of cycles we need to debounce the switch + however
            // far we want to overscan before starting the journey back to "off"
            //
            // The overscan allows us to more easily 'lock out' key chatter events by ignoring
            // state changes for longer after detecting an event on a specific key
            //
            // It'll usually not be filled
            if (__builtin_expect(( debouncer->counters[i] < debounce_integrator_ceiling ), EXPECT_TRUE) ) {
                //Increment the counter
                debouncer->counters[i]++;

                // If the counter is at exactly the positivethreshold, then it's
                // time to consider toggling the key state.
                // Note that we'll hit the threshold twice during the period when the
                // switch should be on. Once on the way up and once on the way back down.

                if (debouncer->counters[i] == debounce_toggle_on_threshold) {
                    // If the debounced state is currently 'off'...
                    // (It's more likely the case that by the time we hit this code path,
                    //  the debounced state would be on. We'd only be toggling it
                    //  the first time we got here after the counter filled)
                    if (__builtin_expect( (debouncer->state & _BV(i)), EXPECT_FALSE) ) {
                        // record the change to return to the caller
                        changes |= _BV(i);
                        // Toggle the debounced state.
                        debouncer->state ^= _BV(i);
                    }
                }
            }
            // If the pin is off
        } else {
            // If the counter isn't bottomed out
            // (It'll usually be bottomed out)
            if (__builtin_expect(( debouncer->counters[i] > debounce_integrator_floor), EXPECT_FALSE) ) {

                // Decrement the counter
                debouncer->counters[i]--;

                // If the counter is at exactly the negative threshold, then it's
                // time to consider toggling the key state.
                // Note that we'll hit the threshold twice during the period when the
                // switch should be off. Once on the way down and once on the way back up.
                if (debouncer->counters[i] == debounce_toggle_off_threshold) {

                    // If the debounced state is currently 'on'...
                    // (It's more likely the case that by the time we hit this code path,
                    //  the debounced state would be offf. We'd only be toggling it
                    //  the first time we got here after the counter emptied
                    if (__builtin_expect(  (debouncer->state ^ _BV(i)), EXPECT_FALSE) ) {

                        // record the change to return to the caller
                        changes |= _BV(i);
                        // Toggle the debounced state.
                        debouncer->state ^= _BV(i);
                    }
                }

            }
        }

    }

    return changes;
}
