# is31io7326_clone
Functional clone of the IS31IO7326 using an ATtiny48 and ATtiny88.

Key reporting and rollover detection are tested and working, all options in the configuration register are saved but do not yet influence behaviour.

## Getting started:

1. Install an avr toolchain (if you're on OS X, try [CrossPack for AVR®](https://www.obdev.at/products/crosspack/))
2. `cd firmware`
3. `make`
4. `make flash` and `make fuse` as necessary to program your MCU

## To make the factory firmware

You'll need a copy of the `TWI_Slave` firmware from [attiny_i2c_bootloader](https://github.com/keyboardio/attiny_i2c_bootloader) built as `twi_slave.hex` and available in the root of this repository.

You can then type `make` in the root of the repository, which will stitch the bootloader together with the AVR keyscanner firmware to make a complete factory image.

## To upload new firmware to existing ATtiny chips

If you have new firmware you want to flash to the ATtiny chip (which has the bootloader installed),
you run:

```
python hex_to_atmega.py firmware/main.hex
```

This will output to `stdout` an ATmega / Arduino program (running as the I2C master) that will flash the firmware to the ATtiny.


### Common issues:

---

`avrdude: no programmer has been specified on the command line or the config file`

`avrdude` needs to know what kind of programmer you're using. You can uncomment/edit one of the `PROGRAMMER` lines in `firmware/Makefile`, or add a `default_programmer` directive to your `~/.avrduderc`.

For more information, check out [`firmware/Makefile`](firmware/Makefile).

---

`avrdude: AVR Part "attiny48" not found.`

`avrdude` can't find a part config for the attiny48. You can resolve this by appending [`doc/attiny48.avrduderc`](doc/attiny48.avrduderc) to `~/.avrduderc`:

```
cat doc/attiny48.avrduderc >> ~/.avrduderc
```

---

## Porting to different hardware

It's possible to configure this project for different AVRs. Check out [`config/attiny48.h`](firmware/config/attiny48.h) to get familiar with the required definitions and application considerations.
