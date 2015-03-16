
#include <avr\io.h>
#include <avr\interrupt.h>

#include <stdlib.h>
#include "variables.h"
#include "utility.h"
#include "speed_control.h"
#include "nRF24L01.h"
#include "behaviors.h"
#include "sensors.h"
#include "irCommunication.h"

    int state = 0;
    unsigned long int fredStartTime;
    unsigned long int fredEndTime;

	unsigned long int startTime = 0, endTime = 0, turnOffLedsTime = 0;
	unsigned char prevSelector=0;
	unsigned int i=0;
	unsigned int currRand=0, currRand2=0;
	float targetAngle=0;

void setup() {
  // put your setup code here, to run once:

	initPeripherals();

	calibrateSensors();

	initBehaviors();

	startTime = getTime100MicroSec();

	speedStepCounter = getTime100MicroSec();



}



void loop() {
  // put your main code here, to run repeatedly:



}
