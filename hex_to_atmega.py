#!/usr/bin/env python2.7

from collections import namedtuple
import sys

page_size = 64
frame_size = 16
memory_size = 8192
blank = 0xff
delay_ms = 1

template = '''
#include <util/crc16.h>
#include "Arduino.h"
#include "Wire.h"

void setup() {
  Wire.begin();
}

#define page_size %d
#define frame_size %d
#define blank 0x%x
#define pages %d
#define firmware_length %d
#define DELAY %d
const uint16_t offsets[pages] = {%s};
const byte firmware[firmware_length] PROGMEM = {%s};

byte written = 0;
byte addr = 0x58;

byte read_crc16(byte *version, uint16_t *crc16, uint16_t offset, uint16_t length) {
  byte result = 2;

  Wire.beginTransmission(addr);
  Wire.write(0x06); // get version and CRC16
  Wire.write(offset & 0xff); // addr (lo)
  Wire.write(offset >> 8); // addr (hi)
  Wire.write(length & 0xff); // len (lo)
  Wire.write(length >> 8); // len (hi)
  result = Wire.endTransmission();
  if (result != 0) {
      return result;
  }
  Wire.requestFrom(addr, (uint8_t) 3);
  Serial.print("Available bytes: ");
  Serial.print(Wire.available());
  Serial.print("\\n");
  if (Wire.available() == 0) {
  }
  byte v = Wire.read();
  *version = v;
  if (Wire.available() == 0) {
    return 0xFF;
  }
  byte crc16_lo = Wire.read();
  if (Wire.available() == 0) {
    return 0xFF;
  }
  byte crc16_hi = Wire.read();
  while (Wire.available()) {
    byte c = Wire.read();
  }
  *crc16 = (crc16_hi << 8) | crc16_lo;
  return result;
}

void loop() {
  if (written != 0) {
    // we're done
    return;
  }

  Serial.print("Communicating\\n");

  byte result = 2;
  while (result != 0) {
    Serial.print("Reading CRC16\\n");

    byte version;
    uint16_t crc16;
    result = read_crc16(&version, &crc16, 0, firmware_length);

    Serial.print("result ");
    Serial.print(result);
    Serial.print("\\n");

    if (result != 0) {
      _delay_ms(100);
      continue;
    }
    Serial.print("Version: ");
    Serial.print(version);
    Serial.print("\\n\\nExisting CRC16 of 0000-1FFF: ");
    Serial.print(crc16, HEX);
    Serial.print("\\n");
  }


  Serial.print("Erasing\\n");
  Wire.beginTransmission(addr);
  Wire.write(0x04); // erase user space
  result = Wire.endTransmission();
  Serial.print("result = ");
  Serial.print(result);
  Serial.print("\\n");
  if (result != 0) {
    _delay_ms(1000);
    return;
  }

  byte o = 0;

  for (uint16_t i = 0; i < firmware_length; i += page_size) {
    Serial.print("Setting addr\\n");
    Wire.beginTransmission(addr);
    Wire.write(0x1); // write page addr
    Wire.write(offsets[o] & 0xff); // write page addr
    Wire.write(offsets[o] >> 8);
    result = Wire.endTransmission();
    Serial.print("result = ");
    Serial.print(result);
    Serial.print("\\n");
    _delay_ms(DELAY);
    // got something other than ACK. Start over.
    if (result != 0) {
      return;
    }

    // transmit each frame separately
    for (uint8_t frame = 0; frame < page_size / frame_size; frame++) {
      Wire.beginTransmission(addr);
      Wire.write(0x2); // continue page
      uint16_t crc16 = 0xffff;
      for (uint8_t j = frame * frame_size; j < (frame + 1) * frame_size; j++) {
        if (i + j < firmware_length) {
          uint8_t b = pgm_read_byte(&firmware[i + j]);
          Wire.write(b);
          crc16 = _crc16_update(crc16, b);
        } else {
          Wire.write(blank);
          crc16 = _crc16_update(crc16, blank);
        }
      }
      // write the CRC16, little end first
      Wire.write(crc16 & 0xff);
      Wire.write(crc16 >> 8);
      Wire.write(0x00); // dummy end byte
      result = Wire.endTransmission();
      Serial.print("got ");
      Serial.print(result);
      Serial.print(" for page ");
      Serial.print(offsets[o]);
      Serial.print(" frame ");
      Serial.print(frame);
      Serial.print("\\n");
      // got something other than NACK. Start over.
      if (result != 3) {
        return;
      }
      delay(DELAY);
    }
    o++;
  }

  // verify firmware
  while (result != 0) {
    Serial.print("Reading CRC16\\n");

    byte version;
    uint16_t crc16;
    // skip the first 4 bytes, are they were probably overwritten by the reset vector preservation
    result = read_crc16(&version, &crc16, offsets[0] + 4, firmware_length - 4);

    Serial.print("result ");
    Serial.print(result);
    Serial.print("\\n");

    if (result != 0) {
      _delay_ms(100);
      continue;
    }
    Serial.print("Version: ");
    Serial.print(version);
    Serial.print("\\n\\nCRC CRC16 of ");
    Serial.print(offsets[0] + 4, HEX);
    Serial.print("-");
    Serial.print(offsets[0] + firmware_length, HEX);
    Serial.print(": ");
    Serial.print(crc16, HEX);
    Serial.print("\\n");

    // calculate our own CRC16
    uint16_t check_crc16 = 0xffff;
    for (uint16_t i = 4; i < firmware_length; i++) {
      check_crc16 = _crc16_update(check_crc16, pgm_read_byte(&firmware[i]));
    }
    if (crc16 != check_crc16) {
      Serial.print("CRC does not match ours: ");
      Serial.print(check_crc16, HEX);
      Serial.print("\\n");
      return;
    }
    Serial.print("CRC check: OK\\n");
  }

  written = 1; // firmware successfully rewritten

  Serial.print("resetting\\n");
  Wire.beginTransmission(addr);
  Wire.write(0x03); // execute app
  result = Wire.endTransmission();
  Serial.print("result ");
  Serial.print(result);
  Serial.print("\\n");

  Serial.print("done\\n");
}
'''

Line = namedtuple('Line', ['size', 'offset', 'kind', 'data', 'checksum'])

def parse_line(line):
    """Parses an Intel HEX line into a Line tuple"""
    if len(line) < 11:
        exit("invalid Intel HEX line: %s" % line)
    if line[0] != ':':
        exit("invalid Intel HEX line: %s" % line)
    line = line[1:]
    try:
        int(line, 16)
    except ValueError:
        exit("invalid Intel HEX line: %s" % line)
    line = line.decode('hex')
    return Line(line[0], line[1:3], line[3], line[4:-1], line[-1])

with open(sys.argv[1]) as fin:
    hex_in = fin.read()

hex_lines = [parse_line(l) for l in hex_in.strip().split()]
hex_lines = [l for l in hex_lines if l.kind == '\x00']

mem = [blank] * memory_size
offsets = []
data = []
for line in hex_lines:
    offset = (ord(line.offset[0]) << 8) + ord(line.offset[1])
    for i, x in enumerate(line.data):
        mem[offset + i] = ord(x)

# # if the first offset is not 0, then we need to write an additional 4 bytes for some reason
# for i in xrange(0, len(mem)):
#     if mem[i] != 0:
#         if i >= 4:
#             mem[i-4] = blank
#             mem[i-3] = blank
#             mem[i-2] = blank
#             mem[i-1] = blank
#         break
#
# scan memory
for i in xrange(0, len(mem), page_size):
    # can skip this page
    if all(x == blank for x in mem[i:i+page_size]):
        continue
    offsets.append(i)
    data.extend(mem[i:i+page_size])

offsets_text = ', '.join(str(x) for x in offsets)
data_text = ', '.join(hex(x) for x in data)

print template % (page_size, frame_size, blank, len(offsets), len(data), delay_ms, offsets_text, data_text)
