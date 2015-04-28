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


const unsigned int LOW_SPEED=8;
const unsigned int NORMAL_SPEED=15;
const unsigned int HIGH_SPEED=20;

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

void setup() {
  initPeripherals();
  calibrateSensors();
  initBehaviors();
  irCommInit();

  speedStepCounter = getTime100MicroSec();

  enableObstacleAvoidance();
    setLeftSpeed(0);
    setRightSpeed(0);

     pwm_red = 255;
     pwm_green = 0;
     pwm_blue = 255;

    updateRedLed(pwm_red);
    updateGreenLed(pwm_green);
    updateBlueLed(pwm_blue);

     startTime=getTime100MicroSec();
}

int danceStateReceived = 0;


//the arg is used to know if is a receiver or a sender robot (0:receiver, 1:senrer)
void senseOtherRobots(int sender) {
  irCommTasks();
  switch(sender){
    case 0:
      if (irCommDataAvailable()==1) {
      danceStateReceived = irCommReadData();
      setLEDcolor(100,100,100);
      sleep(8);
      }
      break;

   case 1:
       //turnOffGreenLeds();
       if (irCommDataSent() == 1) {
        irCommSendData(1);
      }
      break;
    }
}


void checkStart(){
  //to be able to start with the pc antenna
  handleRFCommands();

  if(pwm_red==0 ){
    danceStateReceived=1;
    }
  }

//255 --> is zero ; 0--> mean full color
void setLEDcolor(unsigned char newRed, unsigned char newGreen, unsigned char newBlue){
    if(newRed!=pwm_red){
        pwm_red= newRed;
        updateRedLed(newRed);
    }
    if(newGreen!=pwm_green){
        pwm_green= newGreen;
        updateGreenLed(newGreen);
    }
    if(newBlue!=pwm_blue){
        pwm_blue= newBlue;
        updateBlueLed(newBlue);
    }
}

void dance(){

  switch(danceStateReceived){
    case 0:
      senseOtherRobots(0);
      checkStart();
      break;

    case 1:
      enableObstacleAvoidance();

      setLEDcolor(255,255,0);

      setLeftSpeed(NORMAL_SPEED);
      setRightSpeed(NORMAL_SPEED);
      senseOtherRobots(1);
      break;
    }

}

void loop() {

//this one is necessary, if not present the proximity sensors will not work !  YOU HAVE TO CALL IT ONCE INSIDE THE CODE, NOT OBLIGATORY HERE, you can call it in a different function
//irCommTasks();

  dance();
  updateMotorSpeeds();
}

