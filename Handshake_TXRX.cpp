#include <TimerOne.h>
#include "Handshake_TXRX.h"

//Default constructor
Handshake_TXRX::Handshake_TXRX() {
  pin_mask = 0;
  //Set all pins to input, make sure they are off
  PORTB = 0x00;
  DDRB = 0x00;

  //Set all input pins to input
  PORTC = 0x00;

  sample_count = 0;
  sample_guess = INCOMPLETE;
  accumulation = 0;
  accumulation_count = 0;
  escape_flag = 0;

  idle_count = 0;
}

//Use this function to define which pins will be used to transmit
int Handshake_TXRX::setTXPins(uint8_t pin) {
  //Restrict to four output pins, digital 8-11 (14-17 on the ATmega168 package).
  if (pin > 11 || pin < 8) {
    return -1;
  }

  //Expect 'pin' value to be between 8-11, need to bitmap to a single bytes
  uint8_t map_pin = 1;
  map_pin = map_pin << (pin - 8);
  pin_mask |= map_pin;

  //Update Arduino hardware regs
  DDRB = pin_mask;
}

//Use this function to define which pins will be used to receive
int Handshake_TXRX::setRXPins(uint8_t pin) {
  //Restrict to 4 input pins: A0-A3 or 23-26 on the ATM168 package
  if (pin > 3) {
    return -1;
  }

  //Expect 'pin' value to be between 8-11, need to bitmap to a single bytes
  uint8_t map_pin = 1;
  map_pin = map_pin << (pin);
  pin_mask |= map_pin;
  map_pin ^= 0xff; //invert the byte

  //Update Arduino hardware regs
  DDRC &= map_pin;
}

int Handshake_TXRX::setPeriod(uint16_t micro_sec) {
  //Note: the arduino does not allow period of lower than 4 us.
  if (micro_sec < 4 || micro_sec > 65500) return -1;
  period = micro_sec;
}

uint16_t Handshake_TXRX::get_encoded_byte(uint8_t input_byte) {
  //Direct only, for now. Just need something here to intercept
  if (!ENCODING) return input_byte;
  else {
    if (ENCODING == 1) {
      //input should be 8 bits, return 10 bits
      uint16_t to_return = 0;
      uint8_t lower_nibble = (input_byte & 0x0F);
      uint8_t upper_nibble = (input_byte & 0xF0) >> 4;
      to_return |= (lookup_4b5b[upper_nibble] << 5);
      to_return |= (lookup_4b5b[lower_nibble]);
      return to_return;
    }
  }
}

uint8_t Handshake_TXRX::get_decoded_byte(uint16_t input_byte) {
  if (!ENCODING) return (input_byte & 0xFF);
  else {
    if (ENCODING == 1) {
      //input should be 10 bits, return 8 bits
      //There must be some efficient way to do this...
      int i;
      uint8_t to_return = 0;
      uint8_t lower_nibble = (input_byte & 0x001F);
      uint8_t upper_nibble = (input_byte & 0x03E0) >> 5;
      to_return |= (reverse_4b5b[upper_nibble] << 4);
      to_return |= (reverse_4b5b[lower_nibble]);
      return to_return;
    }
  }
}

void Handshake_TXRX::init_4b5b_table() {
  memset(lookup_4b5b, 0, 16);
  lookup_4b5b[0] = 0x1E;
  lookup_4b5b[1] = 0x09;
  lookup_4b5b[2] = 0x14;
  lookup_4b5b[3] = 0x15;
  lookup_4b5b[4] = 0x0A;
  lookup_4b5b[5] = 0x0B;
  lookup_4b5b[6] = 0x0E;
  lookup_4b5b[7] = 0x0F;
  lookup_4b5b[8] = 0x12;
  lookup_4b5b[9] = 0x13;
  lookup_4b5b[10] = 0x16;
  lookup_4b5b[11] = 0x17;
  lookup_4b5b[12] = 0x1A;
  lookup_4b5b[13] = 0x1B;
  lookup_4b5b[14] = 0x1C;
  lookup_4b5b[15] = 0x1D;

  memset(reverse_4b5b, 0, 32);
  reverse_4b5b[0x1E] = 0;
  reverse_4b5b[0x09] = 1;
  reverse_4b5b[0x14] = 2;
  reverse_4b5b[0x15] = 3;
  reverse_4b5b[0x0A] = 4;
  reverse_4b5b[0x0B] = 5;
  reverse_4b5b[0x0E] = 6;
  reverse_4b5b[0x0F] = 7;
  reverse_4b5b[0x12] = 8;
  reverse_4b5b[0x13] = 9;
  reverse_4b5b[0x16] = 10;
  reverse_4b5b[0x17] = 11;
  reverse_4b5b[0x1A] = 12;
  reverse_4b5b[0x1B] = 13;
  reverse_4b5b[0x1C] = 14;
  reverse_4b5b[0x1D] = 15;
}

void Handshake_TXRX::TX(uint8_t bitt) {
  //Ensure bit is copied to all other bits in the byte
  uint8_t TX_mask = bitt | (bitt << 1) | (bitt << 2) | (bitt << 3) | (bitt << 4) | (bitt << 5) | (bitt << 6) | (bitt << 7);

  PORTB = TX_mask & pin_mask;
}

SAMPLE_VALUE Handshake_TXRX::RX() {
  //Use the Analog pins in digital mode for now...
  uint8_t RX_values = PINC;

  //Use a simple voting algorithm:
  //Do this later, right now, only one of the designated RX pins needs to be high to return a high.
  RX_values &= pin_mask;

  //Now we have the value, see how it matches up with our previous samples.
  SAMPLE_VALUE rx_value;
  if (RX_values) {
    rx_value = ONE;
    idle_count = 0;
  }
  else {
    rx_value = ZERO;
    idle_count++;
  }

  if (rx_value == sample_guess) {
    sample_count++;
  }
  else {
    sample_guess = rx_value;
    sample_count = 1;
  }

  if (sample_count >= 5) {
    sample_count = 0;
    return sample_guess;
  }
  else return INCOMPLETE;
}

void Handshake_TXRX::accumulate(uint8_t bitt) {
    bitt &= 0x01;
    accumulation = accumulation << 1;
    accumulation |= bitt;
    accumulation_count++;
}

uint8_t Handshake_TXRX::accumulate_ready() {
  //We will need some modulation-scheme variation here...
  //but for now:
  //THIS IS DONE :)
  switch(receive_state) {
    case WAIT_PRE:
      if ((accumulation & BITMASK) == get_encoded_byte(PREAMBLE)) {
        receive_state = RX_DATA;
        accumulation = 0;
        accumulation_count = 0;
        Serial.println("RX seen preamble");
        return 0;
      }
    break;
    case RX_DATA:
      if (accumulation_count >= BITS_PER_BYTE) {
        if ((accumulation & BITMASK) == get_encoded_byte(POSTAMBLE)) {
          receive_state = WAIT_PRE;
          accumulation = 0;
          accumulation_count = 0;
          escape_flag = 0;
          Serial.println("RX seen postamble");
          return 0;
        }
        else if (((accumulation & BITMASK) == get_encoded_byte(ESCAPE)) && !escape_flag) {
          accumulation = 0;
          accumulation_count = 0;
          Serial.println("Escape character");
          escape_flag = 1;
          //We need to waste the escape character and move on...
          return 0;
        }
        else {
          escape_flag = 0;
          return 1;
        }
      }
    break;
//    case RX_ESCAPE:
//
//    break;
    case WAIT_POST:
      receive_state == WAIT_PRE;
    break;
    default:
      return 0;
    break;
  }
}

uint8_t Handshake_TXRX::accumulate_get() {
  //More modulation magic needed...
  uint8_t to_return = get_decoded_byte(accumulation & BITMASK);
  accumulation = 0;
  accumulation_count = 0;
  return to_return;
}

uint8_t Handshake_TXRX::rx_available() {
  if (receive_state == WAIT_PRE) return 1;
  else return 0;
}
