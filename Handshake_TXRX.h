#ifndef Handshake_TXRX_H
#define Handshake_TXRX_H


#include <stdio.h>
#include <stdlib.h>
#include <QueueArray.h>

#define PREAMBLE      0x33
#define POSTAMBLE     0xCC
#define ESCAPE        0x3C

#define BITS_PER_BYTE 10
#define BITMASK   0x03FF  //THIS SHOULD BE THE NUMBER OF ONES IN BITS_PER_BYTE
#define ENCODING  1 //0 = direct, 1 = 4b5b

enum TX_ENCODE {ENC_DIRECT, ENC_4B5B};
enum SAMPLE_VALUE {ZERO, ONE, INCOMPLETE};

enum RX_STATE {WAIT_PRE, RX_DATA, RX_ESCAPE, WAIT_POST};


class Handshake_TXRX {

public:
  Handshake_TXRX();
  int setPeriod(uint16_t micro_sec);
  int setTXPins(uint8_t pin);
  int setRXPins(uint8_t pin);
  void TX(uint8_t bitt);
  SAMPLE_VALUE RX();
  uint16_t get_encoded_byte(uint8_t input_byte);
  uint8_t get_decoded_byte(uint16_t input_byte);
  void accumulate(uint8_t bitt);
  uint8_t accumulate_ready();
  uint8_t accumulate_get();
  uint8_t rx_available();
  void init_4b5b_table();
private:
  uint8_t pin_mask;
  uint16_t period;
  TX_ENCODE tx_encoding;
  SAMPLE_VALUE sample_guess;
  uint8_t sample_count;
  uint32_t accumulation;
  uint16_t accumulation_count;
  uint8_t escape_flag;

  uint8_t lookup_4b5b[16];
  uint8_t reverse_4b5b[32];


  uint32_t idle_count;

  RX_STATE receive_state;

};

#endif
