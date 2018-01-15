#pragma once

#include <stdint.h>


#define EXPECT_FALSE 0
#define EXPECT_TRUE 1

extern uint8_t keyscanner_debounce_cycles;

/*
each of these 8 bit variables are storing the state for 8 keys

so for key 0, the counter is represented by db0[0] and db1[0]
and the state in state[0].
*/
typedef struct {
    uint8_t counters[8];
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
            // If we have not yet filled the counter
	    // It'll usually not be filled
            if (__builtin_expect(( debouncer->counters[i] < keyscanner_debounce_cycles ), EXPECT_TRUE) ) { 
                //Increment the counter
                debouncer->counters[i]++;
                if (debouncer->counters[i] == keyscanner_debounce_cycles) {
		    // If the debounced state is currently 'off'...
		    // (It's more likely the case that by the time we hit this code path,
		    //  the debounced state would be on. We'd only be toggling it
		    //  the first time we got here after the counter filled)
                    if (__builtin_expect( (debouncer->state ^ _BV(i)), EXPECT_FALSE) ) {  
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
            if (__builtin_expect(( debouncer->counters[i] > 0), EXPECT_FALSE) ) {

                // Decrement the counter
                debouncer->counters[i]--;
                if (debouncer->counters[i] == 0 ) {

		    // If the debounced state is currently 'on'...
		    // (It's more likely the case that by the time we hit this code path,
		    //  the debounced state would be offf. We'd only be toggling it
		    //  the first time we got here after the counter emptied
                    if (__builtin_expect(  (debouncer->state & _BV(i)), EXPECT_FALSE) ) {

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
