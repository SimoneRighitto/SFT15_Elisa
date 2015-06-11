
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

#define LINE_IN_THR_BK 575
#define LINE_OUT_THR_BK 550

#define CONSTANT_SPEED_FOLLOW 16

boolean inLine;
boolean tempActionStarted;
unsigned long tempActionDuration;   // stores the desired duration of the temporal action
unsigned long tempActionStartTime;  // stores the time when the temporal action starts
unsigned long experienceStartTime;  // stores the time when the current run starts

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

void loop() {

  if (isEndOfTempAction()) {
    //updateAccelerometer();
    braitenbergLineFollower();

  }

  updateMotorSpeeds();
}

