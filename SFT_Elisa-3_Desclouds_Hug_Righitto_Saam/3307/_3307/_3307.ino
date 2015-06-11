
#include <mirf.h>
#include <constants.h>
#include <speed_control.h>
#include <nRF24L01.h>
#include <irCommunication.h>
#include <twimaster.h>
#include <eepromIO.h>
#include <adc.h>
#include <ports_io.h>
#include <leds.h>
#include <behaviors.h>
#include <variables.h>
#include <usart.h>
#include <sensors.h>
#include <spi.h>
#include <ir_remote_control.h>
#include <motors.h>
#include <utility.h>

#define LINE_IN_THR_BK 542
#define LINE_OUT_THR_BK 532

#define CONSTANT_SPEED_FOLLOW 10


const unsigned int LOW_SPEED = 9;
const unsigned int NORMAL_SPEED = 15;
const unsigned int HIGH_SPEED = 18;

boolean inLine;
boolean tempActionStarted;
unsigned long tempActionDuration;   // stores the desired duration of the temporal action
unsigned long tempActionStartTime;  // stores the time when the temporal action starts
unsigned long experienceStartTime;  // stores the time when the current run starts

unsigned int robotState;
const unsigned int LOW_BATTERY = 88;
const unsigned int BASE_MODE = 3;

void setup() {
  // put your setup code here, to run once:
  initPeripherals();
  calibrateSensors();
  initBehaviors();
  //irCommInit();

  setLeftSpeed(0);
  setRightSpeed(0);
  turnOffGreenLeds();

  inLine = false;

  tempActionStarted = false;


  tempActionStartTime = speedStepCounter =  getTime100MicroSec();
  tempActionDuration = 0;
  
  
  robotState=LOW_BATTERY;
}


void updateMotorSpeeds()  {
  if ((getTime100MicroSec() - speedStepCounter) >= SPEED_STEP_DELAY) {
    speedStepCounter = getTime100MicroSec();

    if (softAccEnabled) {
      if (pwm_right_desired == 0) {
        pwm_intermediate_right_desired = 0;
      }
      else if ((pwm_right_desired * pwm_intermediate_right_desired) < 0) {
        pwm_intermediate_right_desired = 0;
      }
      else if (pwm_right_desired > pwm_intermediate_right_desired) {
        pwm_intermediate_right_desired += speedStep;
        if (pwm_intermediate_right_desired > pwm_right_desired) {
          pwm_intermediate_right_desired = pwm_right_desired;
        }
      }
      else if (pwm_right_desired < pwm_intermediate_right_desired) {
        pwm_intermediate_right_desired -= speedStep;
        if (pwm_intermediate_right_desired < pwm_right_desired) {
          pwm_intermediate_right_desired = pwm_right_desired;
        }
      }
      if (pwm_left_desired == 0) {
        pwm_intermediate_left_desired = 0;
      }
      else if ((pwm_left_desired * pwm_intermediate_left_desired) < 0) {
        pwm_intermediate_left_desired = 0;
      }
      else if (pwm_left_desired > pwm_intermediate_left_desired) {
        pwm_intermediate_left_desired += speedStep;
        if (pwm_intermediate_left_desired > pwm_left_desired) {
          pwm_intermediate_left_desired = pwm_left_desired;
        }
      }
      else if (pwm_left_desired < pwm_intermediate_left_desired) {
        pwm_intermediate_left_desired -= speedStep;
        if (pwm_intermediate_left_desired < pwm_left_desired) {
          pwm_intermediate_left_desired = pwm_left_desired;
        }
      }
    }
    else {
      pwm_intermediate_right_desired = pwm_right_desired;
      pwm_intermediate_left_desired = pwm_left_desired;
    }
  }

  handleMotorsWithSpeedController();
}


int front_diff = 0;
void braitenbergLineFollower() {
  // proximityResult [0 - 1024], 1024 -> white, 0 -> black
  // proximityResult[8]  -> side left
  // proximityResult[9]  -> front left
  // proximityResult[10] -> front right
  // proximityResult[11] -> side right

  front_diff = ((int)proximityResult[9] - (int)proximityResult[10]) >> 5;
  //signed char forward_speed = CONSTANT_SPEED_FOLLOW - abs(front_diff);
  //if (forward_speed < 0) {
  //  forward_speed = 0;
  //}

  setGreenLed(4, 0);
  //-------------------------------------------------------------------------------------------
  if (inLine) {
    if ( (proximityResult[9] < LINE_OUT_THR_BK) && (proximityResult[10] < LINE_OUT_THR_BK) ) {
      inLine = false;
      setGreenLed(0, 1);
      enableObstacleAvoidance();
    }
  }
  else {
    if ( (proximityResult[9] > LINE_IN_THR_BK) || (proximityResult[10] > LINE_IN_THR_BK) ) {
      inLine = true;
      setGreenLed(0, 0);
      disableObstacleAvoidance();
    }
  }
  //-------------------------------------------------------------------------------------------
  if ( !inLine && (proximityResult[8] > LINE_IN_THR_BK) && (proximityResult[11] > LINE_IN_THR_BK) ) {  //special case... lateral sensors on the line: turn 90 degrees
    if (accY > 0) {      // check the slope and turn the robot to go down
      setLeftSpeed(15);
      setRightSpeed(-15);
      setGreenLed(1, 1);
      setGreenLed(7, 0);
    }
    else {
      setLeftSpeed(-15);
      setRightSpeed(15);
      setGreenLed(1, 0);
      setGreenLed(7, 1);
    }
    startTempAction(PAUSE_500_MSEC);
  }
  else {
    setGreenLed(1, 0);
    setGreenLed(7, 0);

    //setLeftSpeed(10 - (proximityResult[10]>>6));  //stay over the line
    //setRightSpeed(10 - (proximityResult[9]>>6));
    //setLeftSpeed( forward_speed - front_diff);      //follow the line when over it
    //setRightSpeed(forward_speed + front_diff);
    setLeftSpeed( CONSTANT_SPEED_FOLLOW - front_diff);      //follow the line when over it
    setRightSpeed(CONSTANT_SPEED_FOLLOW + front_diff);
  }
}

void startTempAction(unsigned long duration) {
  tempActionStarted = true;
  tempActionStartTime = getTime100MicroSec();
  tempActionDuration = duration;
}

boolean isEndOfTempAction() {
  if (tempActionStarted) {
    if ((getTime100MicroSec() - tempActionStartTime) >= tempActionDuration) {
      tempActionStarted = false;
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return true;
  }
}


//255 --> is zero ; 0--> mean full color
void setLEDcolor(unsigned char newRed, unsigned char newGreen, unsigned char newBlue) {
  if (newRed != pwm_red) {
    pwm_red = newRed;
    updateRedLed(newRed);
  }
  if (newGreen != pwm_green) {
    pwm_green = newGreen;
    updateGreenLed(newGreen);
  }
  if (newBlue != pwm_blue) {
    pwm_blue = newBlue;
    updateBlueLed(newBlue);
  }
}


//rotationDirection : 0 --> clockwise ; 1 --> cunterclokwise
void rotate(int rotationDirection, unsigned int rotationSpeed) {
  switch (rotationDirection) {

    case 0:
      setLeftSpeed(rotationSpeed);
      setRightSpeed(- rotationSpeed);
      break;
    case 1:
      setLeftSpeed(- rotationSpeed);
      setRightSpeed(rotationSpeed);
      break;
  }
}




unsigned long int currentTime = 0;
unsigned long int start = 0;
unsigned int dance_1_state = 0;
void dance_1() {

  switch (dance_1_state) {
    case 0:
      enableObstacleAvoidance();

      setLEDcolor(255, 0, 57); //turquoise
      setLeftSpeed(NORMAL_SPEED);
      setRightSpeed(NORMAL_SPEED);

      start = getTime100MicroSec();
      dance_1_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(0, 63, 255); //dark yellow
        rotate(1, HIGH_SPEED);
        dance_1_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {

        start = currentTime;
        setLEDcolor(171, 0, 255); //green
        setLeftSpeed(- NORMAL_SPEED);
        setRightSpeed(- NORMAL_SPEED);

        dance_1_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(240, 255, 0); //blue
        rotate(0, HIGH_SPEED);
        dance_1_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        setLEDcolor(0, 159, 255); //dark orange
        setLeftSpeed(0);
        setRightSpeed(0);

        dance_1_state = 5;

      }
    case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        robotState = LOW_BATTERY;
        dance_1_state = 0;

      }
      break;
  }
}


void loop() {

  if (isEndOfTempAction()) {
    //updateAccelerometer();
    switch (robotState) {
      case BASE_MODE:
        dance_1();
        break;
      case LOW_BATTERY:
        braitenbergLineFollower();
        break;
    }
  }

  updateMotorSpeeds();
}

