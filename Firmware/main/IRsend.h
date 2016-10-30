/*
* Author: AnalysIR via http://www.AnalysIR.com/
*
* Date: 1st October 2015 V1.0
*
* License: Creative Commons, Attribution-NonCommercial-ShareAlike 4.0 International
*			http://creativecommons.org/licenses/by-nc-sa/4.0/
*
* For Commercial use or alternative License: Please contact the Author, via the website above
*
* Attribution: Please credit the Author on all media and provide a link to http://www.AnalysIR.com/
*
* Feedback: We would love to hear about how you use this software & suggestions for improvement.
*
*/

#ifndef _IRSEND_H_
#define _IRSEND_H_


#if defined(PARTICLE)
	#include "application.h"
#endif

//Variables declaration
#define SEND_PIN TX
#define NEC_BIT_COUNT 32

unsigned char carrierFreq = 0; //default
unsigned char DUTY = 0xF0;
unsigned int cycleCount = 0;

unsigned long sigTime = 0; //used in mark & space functions to keep track of time
unsigned long sigStart = 0; //used to calculate correct length of existing signal, to handle some repeats

//Function prototype
void initIR(void);
void sendHexNEC(unsigned long sigCode, byte numBits, unsigned char repeats, unsigned char kHz);
void initUPWM(unsigned char carrier);
void mark(unsigned int mLen);
void space(unsigned int sLen);
 void initDutyCycle(unsigned char dutyCycle);

void initIR(void) {
	     pinMode(SEND_PIN, OUTPUT);
	     digitalWrite(SEND_PIN,HIGH);
	     //make sure there is enough time to re-flash, may not be necessary
	     initDutyCycle(38); //set to 40% as default, cahnge this as you like to one of the permitted values
}


void sendHexNEC(unsigned long sigCode, byte numBits, unsigned char repeats, unsigned char kHz) {
	  /*  A basic 32 bit NEC signal is made up of:
	   *  1 x 9000 uSec Header Mark, followed by
	   *  1 x 4500 uSec Header Space, followed by
	   *  32 x bits uSec ( 1- bit 560 uSec Mark followed by 1690 uSec space; 0 - bit 560 uSec Mark follwed by 560 uSec Space)
	   *  1 x 560 uSec Trailer Mark
	   *  There can also be a generic repeat signal, which is usually not neccessary & can be replaced by sending multiple signals
	   */
	#define NEC_HEADER_MARK 9000
	#define NEC_HEADER_SPACE 4500
	#define NEC_ONE_MARK 560
	#define NEC_ZERO_MARK 560
	#define NEC_ONE_SPACE 1690
	#define NEC_ZERO_SPACE 560
	#define NEC_TRAILER_MARK 560

	  unsigned long bitMask = (unsigned long) 1 << (numBits - 1); //allows for signal from 1 bit up to 32 bits
	  //
	  if (carrierFreq != kHz)  initUPWM(kHz); //we only need to re-initialise if it has changed from last signal sent

	  sigTime = micros(); //keeps rolling track of signal time to avoid impact of loop & code execution delays
	  sigStart = sigTime; //remember for calculating first repeat gap (space), must end 108ms after signal starts
	  // First send header Mark & Space
	  mark(NEC_HEADER_MARK);
	  space(NEC_HEADER_SPACE);

	  while (bitMask) {
		if (bitMask & sigCode) { //its a One bit
		  mark(NEC_ONE_MARK);
		  space(NEC_ONE_SPACE);
		}
		else { // its a Zero bit
		  mark(NEC_ZERO_MARK);
		  space(NEC_ZERO_SPACE);
		}
		bitMask = (unsigned long) bitMask >> 1; // shift the mask bit along until it reaches zero & we exit the while loop
	  }
	  // Last send NEC Trailer MArk
	  mark(NEC_TRAILER_MARK);

	  //now send the requested number of NEC repeat signals. Repeats can be useful for certain functions like Vol+, Vol- etc
	  /*  A repeat signal consists of
	   *   A space which ends 108ms after the start of the last signal in this sequence
	  *  1 x 9000 uSec Repeat Header Mark, followed by
	  *  1 x 2250 uSec Repeat Header Space, followed by
	  *  32 x bits uSec ( 1- bit 560 uSec Mark followed by 1690 uSec space; 0 - bit 560 uSec Mark followed by 560 uSec Space)
	  *  1 x 560 uSec repeat Trailer Mark
	  */
	  //First calculate length of space for first repeat
	  //by getting length of signal to date and subtracting from 108ms

	  if (repeats == 0) return; //finished - no repeats
	  else if (repeats > 0) { //first repeat must start 108ms after first signal
		space(108000 - (sigTime - sigStart)); //first repeat Header should start 108ms after first signal
		mark(NEC_HEADER_MARK);
		space(NEC_HEADER_SPACE / 2); //half the length for repeats
		mark(NEC_TRAILER_MARK);
	  }

	  while (--repeats > 0) { //now send any remaining repeats
		space(108000 - NEC_HEADER_MARK - NEC_HEADER_SPACE / 2 - NEC_TRAILER_MARK); //subsequent repeat Header must start 108ms after previous repeat signal
		mark(NEC_HEADER_MARK);
		space(NEC_HEADER_SPACE / 2); //half the length for repeats
		mark(NEC_TRAILER_MARK);
	  }
}

void initUPWM(unsigned char carrier) { // Assumes standard 8-bit Arduino, running at 16Mhz
	  //supported values are 30, 33, 36, 38, 40, 56 kHz, any other value defaults to 38kHz
	  //duty cycle is limited to 50, 40, 30, 20, 10 % - other values will be set to 40%

	  switch (carrier) { // set the baud rate to 10 time the carrier frequency
		case 30  : // 30kHz
		  Serial1.begin(300000);
		  break;

		case 33  : // 33kHz
		  Serial1.begin(330000);
		  break;

		case 36  : // 36kHz
		  Serial1.begin(360000);
		  break;

		case 40  : // 40kHz
		  Serial1.begin(400000);
		  break;

		case 56  : // 56kHz
		  Serial1.begin(560000);
		  break;

		case 38  : //default is 38kHz
		default :
		  Serial1.begin(380000);
		  break;
	  }
	  carrierFreq=carrier;
 }

 void initDutyCycle(unsigned char dutyCycle){
	  //now do Duty cycle - we simply set the character to be sent, which creates the duty cycle for us.
	  switch (dutyCycle) {
		case 50  : //50%
		  DUTY = 0xF0;
		  break;

		case 40  : // 40%
		   DUTY = 0xF8;
		  break;

		case 30  : // 30%
		  DUTY = 0xFC;
		  break;

		case 20  : // 20%
		  DUTY = 0xFE;
		  break;

		case 10  : // 10%
		  DUTY = 0xFF;
		  break;

		default : // 50% for any invalid values
		  DUTY = 0xF0;
		  break;
	  }
}

void mark(unsigned int mLen) { //uses sigTime as end parameter
	  sigTime += mLen; //mark ends at new sigTime
	  unsigned long startTime = micros();
	  unsigned long dur = sigTime - startTime; //allows for rolling time adjustment due to code execution delays

	  if (dur == 0) return;

		unsigned int cycleCount = dur / ((1000+carrierFreq/2) / carrierFreq); // get number of cycles & do rounding with integer maths

	  while (cycleCount) {
			Serial1.write(DUTY); //write a character to emulate carrier, character value determines duty cycle.
			--cycleCount;
	  }

	  while ((micros() - startTime) < dur) {} //just wait here until time is up

  }

void space(unsigned int sLen) { //uses sigTime as end parameter
	  sigTime += sLen; //space ends at new sigTime
	  unsigned long startTime = micros();
	  unsigned long dur = sigTime - startTime; //allows for rollingx time adjustment due to code execution delays
	  if (dur == 0) return;
	  while ((micros() - startTime) < dur) ; //just wait here until time is up
}

#endif
