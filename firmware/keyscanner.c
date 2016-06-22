#include <util/delay.h>
#include "debounce.h"
#include "wire-protocol.h"
#include "main.h"
#include "ringbuf.h"

debounce_t db[] = {
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF},
    {0x00, 0x00, 0xFF}
};

void keyscanner_init(void) {

    // Write to cols - we only use some of the pins in the row port
    DDR_COLS = 0xff;
    PORT_COLS = 0xff;

    // Read from rows -- We use all 8 bits of cols
    DDR_ROWS  = 0x00;
    PORT_ROWS |= ROW_PINMASK;

    // Assert comm_en so we can use the interhand transcievers
    // (Until comm_en on the i2c transcievers is pulled high,
    //  they're disabled)
    DDRC ^= _BV(7);
    PORTC |= _BV(7);

}

static inline uint8_t popCount(uint8_t val) {
    uint8_t count;
    for (count=0; val; count++) {
        val &= val-1;
    }
    return count;
}

void keyscanner_main(void) {


    // uint32_t key_state = 0x00;
    uint8_t changes = 0;

    // For each enabled row...
    // TODO: this should really draw from the ROW_PINMASK
    for (uint8_t col = 0; col < COL_COUNT; ++col) {
        // Reset all of our row pins, then unset the one we want to read as low
        PORT_COLS = (PORT_COLS | COL_PINMASK ) & ~_BV(col);
        uint8_t row_bits = PIN_ROWS;
        row_bits &= (_BV(0) | _BV(1)| _BV(2) | _BV(3));
        // Debounce key state
        changes += debounce((row_bits ^ 0x0f) , db + col);
    }


    _delay_ms(1);
    // Most of the time there will be no new key events
    if (__builtin_expect(changes == 0, 1)) {
        return;
    }


    DISABLE_INTERRUPTS({
        ringbuf_append( db[0].state | (db[7].state << 4));
        ringbuf_append( db[6].state | (db[5].state << 4));
        ringbuf_append( db[4].state | (db[3].state << 4));
        ringbuf_append( db[2].state | (db[1].state << 4));
    });
}
