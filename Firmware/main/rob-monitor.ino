#include "IRsend.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

STARTUP(setPinsToInput());
STARTUP(WiFi.selectAntenna(ANT_AUTO));

#define but_clean D1
#define bump_left D3
#define bump_right D4
#define volt_dock A4
#define volt_batt A3
#define led_red A2
#define led_green A0

int voltDock = 0;
int voltBatt = 0;
boolean but_clean_state = false;
boolean bump_left_state = false;
boolean bump_right_state = false;
int led_red_max = 1000;
int led_green_max = 1000;
int led_red_min = 1000;
int led_green_min = 1000;
int led_red_volt = 1000;
int led_green_volt = 1000;
String robOldState = "idle";
String robNewState = "idle";
int sameStateCounter = 1;
int idleCounter = 0;

uint8_t retry_count = 0;
unsigned long old_time = millis();
unsigned long check_time = millis();

String IRcommand = "";

void setup(){
	//initialize IR
	pinMode(SEND_PIN, OUTPUT);
	digitalWrite(SEND_PIN,HIGH);
	initDutyCycle(38); //set to 40% as default, cahnge this as you like to one of the permitted values

	WiFi.on();
	attachInterrupt(but_clean,butCleanInt,FALLING);
	attachInterrupt(bump_left,bumpLeftInt,FALLING);
	attachInterrupt(bump_right,bumpRightInt,FALLING);

	Particle.function("function", cloudFunction);
	Particle.function("ir", processIR);
}

void loop(){
	manageWiFi();
	checkState();
	checkIRstatus();

	if(but_clean_state){
	 //Particle.publish("rob/but/clean",String(but_clean_state));
	   but_clean_state = false;
	}
	if(bump_left_state){
	 //Particle.publish("rob/bump/left",String(bump_left_state));
	   bump_left_state = false;
	}
	if(bump_right_state){
	 //Particle.publish("rob/bump/right",String(bump_right_state));
	   bump_right_state = false;
	}
}


void setPinsToInput()
{
	for (int i = 0; i<=7;i++){
 	pinMode(i, INPUT);
	}
	for (int i = 10; i<=19;i++){
 	pinMode(i, INPUT);
	}
}

void manageWiFi(){
	if(millis() - old_time >= 2000){
	 if(retry_count < 10){
	  if(!WiFi.ready()){
	   WiFi.connect();
	   retry_count++;
	  }
	  else if (!Particle.connected()){
	   Particle.connect();
	   retry_count++;
	  }
	 }
	 else{
	     WiFi.off();
	     retry_count = 0;
	     WiFi.on();
	 }
	 old_time = millis();
	}
}

void butCleanInt() {
	but_clean_state = true;
}

void bumpLeftInt() {
	bump_left_state = true;
}

void bumpRightInt() {
	bump_right_state = true;
}


int cloudFunction(String command)
{
	if(command == "press")
	{
	 pinMode(but_clean,OUTPUT);
	 digitalWrite(but_clean,LOW);
	 delay(50);
	 pinMode(but_clean,INPUT);
	 return 1;
	}
	else if(command == "safemode") {
	 System.enterSafeMode();
	 return 2;
	}
	else return -1;
}

int processIR(String command) {
	if(command == "clean") IRcommand = "clean";
	else if(command == "home") IRcommand = "home";
	else if(command == "left") IRcommand = "left";
	else if(command == "right") IRcommand = "right";
	else if(command == "up") IRcommand = "up";
	else if(command == "down") IRcommand = "down";
	else if(command == "spiral") IRcommand = "spiral";
	else if(command == "edge") IRcommand = "edge";
	else return -1;
	return 1;
}



void checkVolt(void){
	voltDock = analogRead(volt_dock);
 voltDock = map(voltDock,0,2525,0,1260);
 voltBatt = analogRead(volt_batt);
 voltBatt = map(voltBatt,0,2525,0,1260);
}

void checkState(void){
	led_red_volt = analogRead(led_red);
	led_green_volt = analogRead(led_green);
	led_red_max = max(led_red_max,led_red_volt);
	led_green_max = max(led_green_max,led_green_volt);
	led_red_min = min(led_red_min,led_red_volt);
	led_green_min = min(led_green_min,led_green_volt);

	checkVolt();

	if(millis() - check_time > 3000) {
  if(led_red_max < 2000 && led_red_min != 1000) {
   if(led_green_max < 2000){
    robNewState = "idle";
    checkIdleStatus();
   }
  	else if (led_green_max > 2000 && led_green_min == 1000 && voltDock < 1200)
   	robNewState = "vacuuming";
    else if (led_green_max > 2000 && led_green_min == 1000 && voltDock > 1200)
    robNewState = "charged";
   else if (led_green_max > 2000 && led_green_min != 1000)
    robNewState = "active";
  	}
  	else if (led_red_max > 2000){
   	if(led_green_max < 2000 && led_green_min != 1000)
     robNewState = "error";
    else if (led_green_max > 2000 && led_green_min == 1000)
     robNewState = "homing";
    else if (led_green_max > 2000 && led_green_min != 1000)
     robNewState = "charging";
				}

	  led_red_max = 1000;
	  led_green_max = 1000;
	  led_red_min = 1000;
	  led_green_min = 1000;
	  led_red_volt = 1000;
	  led_green_volt = 1000;

   if(robNewState == robOldState) {
    if(sameStateCounter >= 5) {
     sameStateCounter = 1;
    }
    else
     sameStateCounter++;
   }

   if (robNewState != robOldState || sameStateCounter >= 5) {
    Particle.publish("rob/status/state",robNewState + "," + WiFi.RSSI());
    Particle.publish("rob/volt",String(voltBatt) + "," + String(voltDock));
   }

   robOldState = robNewState;
   check_time = millis();
  }
}


void checkIdleStatus() {
	if(idleCounter < 20)
	 idleCounter++;
	else {
	 Particle.publish("rob/status/state","sleep");
	 Particle.process();
	 delay(1000);
	 System.sleep(led_green,CHANGE);
	 idleCounter = 0;
	}
}


void checkIRstatus() {
	if(IRcommand != 0) {
		if(IRcommand == "clean") sendHexNEC(0x2aa22dd, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "home") sendHexNEC(0x2aa8877, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "left") sendHexNEC(0x2aa33cc, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "right") sendHexNEC(0x2aa44bb, NEC_BIT_COUNT, 0, 38);
 	else if (IRcommand == "up") sendHexNEC(0x2aa55aa, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "down") sendHexNEC(0x2aa6699, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "spiral") sendHexNEC(0x2aa7788, NEC_BIT_COUNT, 0, 38);
		else if (IRcommand == "edge") sendHexNEC(0x2aa7766, NEC_BIT_COUNT, 0, 38);

		Particle.publish("rob/state/ir",IRcommand);
		IRcommand = "";
	}
}
