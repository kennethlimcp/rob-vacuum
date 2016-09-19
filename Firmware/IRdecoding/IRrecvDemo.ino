/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 *
 * From: https://github.com/babean/IRRemote
 */


#if defined (PARTICLE)
#include "application.h"
#endif

#include <IRremote.h>

SYSTEM_MODE(MANUAL);

#define RECV_PIN D0

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.print("Type: ");
    if (results.decode_type == NEC) {
      Serial.println("NEC");
    } else if (results.decode_type == SONY) {
      Serial.println("SONY");
    } else if (results.decode_type == RC5) {
      Serial.println("RC5");
    } else if (results.decode_type == RC6) {
      Serial.println("RC6");
    } else if (results.decode_type == UNKNOWN) {
      Serial.println("UNKNOWN");
    }

    Serial.println("IRcode: " + String(results.value, HEX));
    Serial.println("no of bytes: " + String(results.bits));
    irrecv.resume(); // Receive the next value
  }
}
