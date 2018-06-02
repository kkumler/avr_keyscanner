
// All state machine configs use the same guard -- we only want one at a time.
#ifndef _DEBOUNCER_STATE_MACHINE_CONFIG_H
#define _DEBOUNCER_STATE_MACHINE_CONFIG_H

enum lifecycle_phases {
    OFF, TURNING_ON, DEBOUNCING_ON, ON, DEBOUNCING_OFF, LOCKED_OFF,


};

lifecycle_phase_t lifecycle[] = {
    {
        // OFF -- during this phase, any 'off' value means that we should keep this key pressed
        // A single 'on' value means that we should start checking to see if it's really a key press
        //
        // IF we get an 'on' value, change the phase to 'TURNING_ON' to make sure it's not just
        // chatter
        //
        // Our timers are set to 0, but that doesn't matter because in the event that we overflow the timer
        // we just go back to the 'OFF' phase

        .next_phase = OFF,
        .expected_data = 0,
        .unexpected_data_phase = TURNING_ON,
        .change_output_on_expected_transition = 0,
        .timer = 0,
    },
    {
        // TURNING_ON-- during this phase, we believe that we've detected
        // a switch being turned on. We're now checking to see if it's
        // reading consistently as 'on' or if it was just a spurious "on" signal
        // as might happen if we saw key chatter
        //
        // If it was a spurious disconnection, mark the switch as noisy and go back to phase OFF
        //
        // If we get through the timer with no "off" signals, proceed to phase DEBOUNCING_ON

        .next_phase = DEBOUNCING_ON,
        .expected_data = 1,
        .unexpected_data_phase = OFF,
        .change_output_on_expected_transition = 0,
        .timer = 2,
    },
    {
        // DEBOUNCING_ON -- during this phase, the key is on, no matter what value we read from the input
        // pin.
        //
        // If we see any 'off' signals, that indicates a short read or chatter.
        // In the event of unexpected data, stay in the DEBOUNCING_ON phase, but don't reset the timer.

        .next_phase = ON,
        .expected_data = 1,
        .unexpected_data_phase = DEBOUNCING_ON,
        .change_output_on_expected_transition = CHANGE_OUTPUT,
        .timer = 55,
    },
    {
        // ON -- during this phase, any 'on' value means that we should keep this key pressed
        // A single 'off' value means that we should start checking to see if it's really a key release
        //
        // IF we get an 'off' value, change the phase to 'DEBOUNCING_OFF' to make sure it's not just
        // chatter
        //
        // Our timers are set to 0, but that doesn't matter because in the event that we overflow the timer
        // we just go back to the 'ON' phase
        .next_phase = ON,
        .expected_data = 1,
        .unexpected_data_phase = DEBOUNCING_OFF,
        .change_output_on_expected_transition = 0,
        .timer = 0,
    },
    {
        // DEBOUNCING_OFF -- during this phase, we believe that we've detected
        // a switch being turned off. We're now checking to see if it's
        // reading consistently as 'off' or if it was just a spurious "off" signal
        // as might happen if we saw key chatter
        //
        // If it was a spurious connection, mark the switch as noisy and go back to phase ON
        //
        // If we get through the timer with no "on" signals, proceed to phase LOCKED_OFF
        .next_phase = LOCKED_OFF,
        .expected_data = 0,
        .unexpected_data_phase = ON,
        .change_output_on_expected_transition = 0,
        .timer = 17,  // release latency
    },
    {
        // LOCKED_OFF -- during this phase, the key is off, no matter what value we read from the input
        // pin.
        //
        // If we see any 'on' signals, that indicates a short read or chatter.
        // In the event of unexpected data, stay in the LOCKED_OFF phase, but don't reset the timer.
        .next_phase = OFF,
        .expected_data = 0,
        .unexpected_data_phase =  LOCKED_OFF,
        .change_output_on_expected_transition = CHANGE_OUTPUT,
        .timer = 1,
    },
};

#endif
