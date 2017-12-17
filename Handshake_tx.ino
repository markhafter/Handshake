#include <QueueArray.h>
#include <TimerOne.h>


#include "Handshake_TXRX.h"

#define SEND_PERIOD   10000
#define RECEIVE_PERIOD  (SEND_PERIOD / 5)   //For now, we won't automatically detect speed, and expect RX sample = 5 * TX


#define I_WILL_BE_THE_TX  1   //Set to 1 for the transmitter, 0 for receiver
#define DEBUG    1

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

enum TX_STATE {TX_INIT, TX_WAIT, RX_WAIT, TX_READY, RX_READY, TX_SENDING_PRE, TX_SENDING, TX_SENDING_ESCAPE, TX_SENDING_POST, RX_RECEIVING, TX_DONE, RX_DONE};
TX_STATE state = TX_INIT;

void setup() {
  // put your setup code here, to run once:


  Serial.begin(9600);
  Serial.setTimeout(1);

  Serial.println("Serial Initialized.");
    txrx.setTXPins(8);
    txrx.setTXPins(9);
    txrx.init_4b5b_table();
    Serial.println("Welcome to TX.");
    Serial.println("Enter text to send:");
    Timer1.initialize(SEND_PERIOD);
    Timer1.attachInterrupt(send_next_bit);   
    Timer1.stop(); 
    state = TX_WAIT; //MSH set
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() && state == TX_WAIT) {
    state = TX_READY;
    Serial.println("begin send");
    Timer1.start();
    
  }
}

void send_next_bit() {
  switch (state) {
    case TX_READY:
      send_byte = txrx.get_encoded_byte(PREAMBLE);
      state = TX_SENDING_PRE;
      send_byte_idx = 0;
      if (DEBUG) Serial.println("Getting Preamble Ready");
    break;
    case TX_SENDING_PRE:
      txrx.TX((send_byte >> (BITS_PER_BYTE - send_byte_idx - 1)) & 0x01);
      send_byte_idx++;
      if (send_byte_idx >= BITS_PER_BYTE) {
        //We assume serial is available because that's how we got here in the first place
        //and no one else is reading it
        if (Serial.peek() == ESCAPE || Serial.peek() == POSTAMBLE) {
            send_byte = txrx.get_encoded_byte(ESCAPE);
            state = TX_SENDING_ESCAPE;
        }
        else send_byte = txrx.get_encoded_byte(Serial.read());
        state = TX_SENDING;
        send_byte_idx = 0;
        if (DEBUG) Serial.println("Pre Complete");
      }
    break;
    case TX_SENDING:
      txrx.TX((send_byte >> (BITS_PER_BYTE - send_byte_idx - 1)) & 0x01);
      send_byte_idx++;
      if (send_byte_idx >= BITS_PER_BYTE) {
        //If we are done with the current byte, try and get another
        if (Serial.available()) {
          //We can get more data
          //But first make sure we don't need to escape first
          if (Serial.peek() == ESCAPE || Serial.peek() == POSTAMBLE) {
            send_byte = txrx.get_encoded_byte(ESCAPE);
            state = TX_SENDING_ESCAPE;
          }
          else {
            send_byte = txrx.get_encoded_byte(Serial.read());
          }
        }
        else {
          //We can't get more data
          send_byte = txrx.get_encoded_byte(POSTAMBLE);
          state = TX_SENDING_POST;
        }
        send_byte_idx = 0;
      }
    break;
    case TX_SENDING_ESCAPE:
      //If we are here, it means we need to send the escape character before
      //we consume whatever is next in the serial buffer
      //Make sure send_byte and idx is set up already!
      txrx.TX((send_byte >> (BITS_PER_BYTE - send_byte_idx - 1)) & 0x01);
      send_byte_idx++;
      if (send_byte_idx >= BITS_PER_BYTE) {
        //This time, guarantee we send whatever is next. Don't check it.
        send_byte = txrx.get_encoded_byte(Serial.read());
        send_byte_idx = 0;
        state = TX_SENDING;
      }
    break;
    case TX_SENDING_POST:
      txrx.TX((send_byte >> (BITS_PER_BYTE - send_byte_idx - 1)) & 0x01);
      send_byte_idx++;
      if (send_byte_idx >= BITS_PER_BYTE) {
        //done sending postamble
        txrx.TX(0); //close channel
        state = TX_WAIT;
        Timer1.stop();
        if (DEBUG) Serial.println("Post Complete");
      }  
    break;
    case TX_WAIT:
      //we should never get here, but in case we do...
      txrx.TX(0);
      Timer1.stop();
    break;
    
  }
}
  
//  txrx.TX((send_byte >> (BITS_PER_BYTE - send_byte_idx - 1)) & 0x01);
//  send_byte_idx++; 
//  
//  if (send_byte_idx >= BITS_PER_BYTE) {
//    if (!Serial.available() && state != TX_WAIT) {
//      if (state == TX_SENDING_POST) {
//        Serial.println("Post complete");
//        txrx.TX(0);
//        state = TX_WAIT;
//        Timer1.stop();
//        return;
//      }
////      Serial.println("Send post");
//      state = TX_SENDING_POST;
//      send_byte = POSTAMBLE;
//      send_byte_idx = 0;
//    }
//    else {
//     if (state == TX_SENDING_PRE) {
////      Serial.println("Send pre");
//      send_byte = PREAMBLE;
//      state = TX_SENDING;
//     }
//     else {
////      Serial.println("Send data");
//      send_byte = txrx.get_encoded_byte(Serial.read());
//     }
//     send_byte_idx = 0;
//    }
//  }
//  if (state == TX_WAIT) {
//    txrx.TX(0);
//    Timer1.stop();
//  }
  
//  Serial.println("TEST PROGRAM");
  /*
  txrx.TX(flag);
  if (flag) flag = 0;
  else flag = 1;
  */

