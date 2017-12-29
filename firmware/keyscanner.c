#include <avr/interrupt.h>
#include <util/delay.h>
#include "debounce.h"
#include "wire-protocol.h"
#include "main.h"
#include "ringbuf.h"
#include "keyscanner.h"

debounce_t db[] = {
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        0x00
    },
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        0x00
    },
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        0x00
    },
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        0x00
    }
};

// do_scan gets set any time we should actually do a scan
volatile uint8_t do_scan = 1;

void keyscanner_init(void) {

    // Write to rows - we only use some of the pins in the row port
    DDR_ROWS |= ROW_PINMASK;
    PORT_ROWS |= ROW_PINMASK;

    // Read from cols -- We use all 8 bits of cols
    DDR_COLS  = 0x00;
    // Turn on the Pullups
    PORT_COLS = 0b11111111;

    // Assert comm_en so we can use the interhand transcievers
    // (Until comm_en on the i2c transcievers is pulled high,
    //  they're disabled)

    // PC7 is on the same port as the four row pins.
    // We refer to it here as PORTC because
    // we're not using it as part of the keyscanner
    HIGH(PORTC,7);
    SET_OUTPUT(DDRC,7);

    keyscanner_timer1_init();
}

void keyscanner_scan(void) {
    
    uint8_t debounced_changes = 0;
    uint8_t pin_data;

    // For each enabled row...
    for (uint8_t row = 0; row < ROW_COUNT; ++row) {
        // Reset all of our row pins, then
        // set the one we want to read as low
        LOW(PORT_ROWS, row);
        /* We need a no-op for synchronization. So says the datasheet
         * in Section 10.2.5 */
        asm("nop");
        pin_data = PIN_COLS;
        HIGH(PORT_ROWS,row);
        // Debounce key state
        debounced_changes += debounce((pin_data), db + row);
    }

    // Most of the time there will be no new key events
    if (__builtin_expect(debounced_changes == 0, 1)) {
        return;
    }

    // Snapshot the keystate to add to the ring buffer
    // Run this with interrupts off to make sure that
    // when we read from the ringbuffer, we always get
    // four bytes representing a single keyboard state.
    DISABLE_INTERRUPTS({
        ringbuf_append( db[0].state ^ 0b11111111 );
        ringbuf_append( db[1].state ^ 0b11111111 );
        ringbuf_append( db[2].state ^ 0b11111111 );
        ringbuf_append( db[3].state ^ 0b11111111 );
    });

}

void keyscanner_main(void) {
}

// initialize timer, interrupt and variable
void keyscanner_timer1_init(void) {

    // set up timer with prescaler = 256 and CTC mode
    TCCR1B |= _BV(WGM12)| _BV( CS12);

    // initialize counter
    TCNT1 = 0;

    // initialize compare value
    OCR1A = KEYSCAN_INTERVAL_DEFAULT;

    // enable compare interrupt
    TIMSK1 |= _BV(OCIE1A);

    // enable global interrupts
    sei();
}

// interrupt service routine (ISR) for timer 1 A compare match
ISR(TIMER1_COMPA_vect) {
    keyscanner_scan(); // Yes! Let's do a scan
}
