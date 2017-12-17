#include <QueueArray.h>
#include <TimerOne.h>


#include "Handshake_TXRX.h"

#define SEND_PERIOD   10000
#define RECEIVE_PERIOD  (SEND_PERIOD / 5)   //For now, we won't automatically detect speed, and expect RX sample = 5 * TX

#define I_WILL_BE_THE_TX  0   //Set to 1 for the transmitter, 0 for receiver

QueueArray <uint8_t> rx_queue;

Handshake_TXRX txrx;
uint8_t flag = 1;
uint8_t char_read = 0;
SAMPLE_VALUE receive_bit;
uint8_t receive_sample;
uint8_t receive_byte;

uint16_t send_byte = 0;
int send_byte_idx = 0;
int send_byte_threshold = 7;

enum TX_STATE {TX_INIT, TX_WAIT, RX_WAIT, TX_READY, RX_READY, TX_SENDING_PRE, TX_SENDING, TX_SENDING_POST, RX_RECEIVING, TX_DONE, RX_DONE};
TX_STATE state = TX_INIT;

void setup() {
  // put your setup code here, to run once:


  Serial.begin(9600);
  Serial.setTimeout(1);

  Serial.println("Serial Initialized.");
    txrx.setRXPins(0);
    txrx.setRXPins(1);
    txrx.init_4b5b_table();
    Serial.println("Welcome to RX.");
    Timer1.initialize(RECEIVE_PERIOD);
    Timer1.attachInterrupt(receive_next_bit);
    Timer1.stop();
    state = RX_WAIT;
}

void loop() {
  if (!I_WILL_BE_THE_TX && state == RX_WAIT) {
    state = RX_RECEIVING;
    Timer1.start();
  }
}

void receive_next_bit() {
  receive_bit = txrx.RX();
   if (receive_bit == ZERO) txrx.accumulate(0);
   if (receive_bit == ONE) txrx.accumulate(1);

  if (txrx.accumulate_ready()) {
    receive_byte = txrx.accumulate_get();
    rx_queue.push(receive_byte);
  }
  if (txrx.rx_available() && !rx_queue.isEmpty()) {
    dump_rx_queue();
  }
}

void dump_rx_queue() {
  while (!rx_queue.isEmpty()) {
    Serial.print((char)rx_queue.pop());
//    Serial.print(" ");
  }
  Serial.println();
}
