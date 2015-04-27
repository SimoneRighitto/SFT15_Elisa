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

unsigned long int startTime = 0, endTime = 0;


const unsigned int LOW_SPEED=5;
const unsigned int NORMAL_SPEED=10;
const unsigned int HIGH_SPEED=15;


  void updateMotorSpeeds()  {
  if((getTime100MicroSec()-speedStepCounter) >= SPEED_STEP_DELAY) {
    speedStepCounter = getTime100MicroSec();

    if(softAccEnabled) {
      if(pwm_right_desired == 0) {
        pwm_intermediate_right_desired = 0;
      }
      else if((pwm_right_desired*pwm_intermediate_right_desired) < 0) {
        pwm_intermediate_right_desired = 0;
      }
      else if(pwm_right_desired > pwm_intermediate_right_desired) {
        pwm_intermediate_right_desired += speedStep;
        if(pwm_intermediate_right_desired > pwm_right_desired) {
          pwm_intermediate_right_desired = pwm_right_desired;
        }
      }
      else if(pwm_right_desired < pwm_intermediate_right_desired) {
        pwm_intermediate_right_desired -= speedStep;
        if(pwm_intermediate_right_desired < pwm_right_desired) {
          pwm_intermediate_right_desired = pwm_right_desired;
        }
      }
      if(pwm_left_desired == 0) {
        pwm_intermediate_left_desired = 0;
      }
      else if((pwm_left_desired*pwm_intermediate_left_desired) < 0) {
        pwm_intermediate_left_desired = 0;
      }
      else if(pwm_left_desired > pwm_intermediate_left_desired) {
        pwm_intermediate_left_desired += speedStep;
        if(pwm_intermediate_left_desired > pwm_left_desired) {
          pwm_intermediate_left_desired = pwm_left_desired;
        }
      }
      else if(pwm_left_desired < pwm_intermediate_left_desired) {
        pwm_intermediate_left_desired -= speedStep;
        if(pwm_intermediate_left_desired < pwm_left_desired) {
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


void senseOtherRobots() {
  unsigned char received;
  irCommTasks();
  if (irCommDataSent() == 1) {
    irCommSendData(1);
  }
  if (irCommDataAvailable()==1) {
    received = irCommReadData();
    pwm_blue = 128;
    //turnOnGreenLeds();
  }
  else {
    if (pwm_blue < 255) {
      pwm_blue++;
    }
    //turnOffGreenLeds();
  }
  updateBlueLed(pwm_blue);
}


void setup() {
  // put your setup code here, to run once:

 initPeripherals();
  calibrateSensors();
  initBehaviors();
  irCommInit();

  speedStepCounter = getTime100MicroSec();

  enableObstacleAvoidance();


        pwm_red = 100;
        pwm_green = 0;
        pwm_blue = 255;

  startTime=getTime100MicroSec();
}

/*
* Fonction gérant la jeu de la contamination.
*/
void contamination() {


    endTime = getTime100MicroSec();

    if(endTime-startTime > PAUSE_5_SEC){
        pwm_red=0;
        pwm_green=255;
        pwm_blue=255;
        setLeftSpeed(NORMAL_SPEED);
        setRightSpeed(NORMAL_SPEED);

    }
    if(endTime-startTime > PAUSE_10_SEC){

         pwm_red=255;
        pwm_green=0;
        pwm_blue=255;
        setRightSpeed(0);

    }
    if(endTime-startTime > PAUSE_20_SEC){
          pwm_red=255;
        pwm_green=255;
        pwm_blue=0;
        setRightSpeed(0);
        setLeftSpeed(0);
        startTime=getTime100MicroSec();
    }

    updateRedLed(pwm_red);
    updateGreenLed(pwm_green);
    updateBlueLed(pwm_blue);

}




void loop() {
  // put your main code here, to run repeatedly:
    handleRFCommands();

    contamination();
    updateMotorSpeeds();

}

