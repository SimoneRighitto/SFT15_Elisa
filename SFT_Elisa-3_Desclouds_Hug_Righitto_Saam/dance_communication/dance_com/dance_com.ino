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


const unsigned int LOW_SPEED = 8;
const unsigned int NORMAL_SPEED = 13;
const unsigned int HIGH_SPEED = 20;

//robot state constants
unsigned int robotState;
const unsigned int INIT = 99;
const unsigned int BEFORE_DANCE = 0;
const unsigned int DANCE_1 = 1;
const unsigned int DANCE_2 = 2;

unsigned long int currentTime = 0;
unsigned long int start;

void updateMotorSpeeds_2() {
  pwm_intermediate_right_desired = pwm_right_desired;
  pwm_intermediate_left_desired = pwm_left_desired;

  handleMotorsWithSpeedController();
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

void setup() {
  initPeripherals();
  calibrateSensors();
  initBehaviors();
  //irCommInit();

  speedStepCounter = getTime100MicroSec();

  enableObstacleAvoidance();
  setLeftSpeed(0);
  setRightSpeed(0);

  robotState = INIT;

  pwm_red = 255;
  pwm_green = 255;
  pwm_blue = 255;

  updateRedLed(pwm_red);
  updateGreenLed(pwm_green);
  updateBlueLed(pwm_blue);

  startTime = getTime100MicroSec();
}



void checkStart() {
  //to be able to start with the pc antenna
  handleRFCommands();

  if (pwm_red == 0 ) {
    //danceStateReceived=1;
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

void setRandomColor() {

  unsigned char randomRed = (0 + rand()) % 255;
  unsigned char randomGreen = (0 + rand()) % 255;
  unsigned char randomBlue = (0 + rand()) % 255;

  setLEDcolor(randomRed, randomGreen, randomBlue);


}
unsigned int wait_state = 0;
unsigned long int waitStart;
bool wait(unsigned long int time) {
  switch (wait_state) {
    case 0:
      waitStart = getTime100MicroSec();
      wait_state = 1;
    case 1:
      if (getTime100MicroSec() - waitStart < time) {
        return true;
      }
      else {
        wait_state = 0;

        return false;
      }
      break;

  }
}



unsigned int index = 0;
unsigned int beforeGreenLedState = 0;
void beforeDance() {


  int beforeDanceDelay[] = {PAUSE_1_SEC, PAUSE_750_MSEC, PAUSE_500_MSEC, PAUSE_300_MSEC, PAUSE_300_MSEC, PAUSE_300_MSEC, PAUSE_250_MSEC, PAUSE_250_MSEC, PAUSE_100_MSEC};


  if (index >= (sizeof(beforeDanceDelay) / sizeof(int))) {
    robotState = DANCE_1;
  }

  else {


    switch (beforeGreenLedState) {
      case 0:
        turnOnGreenLeds();
        beforeGreenLedState = 1;
        break;
      case 1:


        if (!wait(beforeDanceDelay[index])) {
          turnOffGreenLeds();
          beforeGreenLedState = 2;
        }

        break;
      case 2:
        if (!wait(beforeDanceDelay[index])) {
          beforeGreenLedState = 0;
          index++;
        }
        break;

    }

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


//turnDirection : 0 --> clockwise ; 1 --> cunterclokwise
void turn(int turnDirection, unsigned int turnSpeed) {
  switch (turnDirection) {

    case 0:
      setLeftSpeed(turnSpeed);
      setRightSpeed(0);
      break;
    case 1:
      setLeftSpeed(0);
      setRightSpeed(turnSpeed);
      break;
  }
}

unsigned int dance_1_state = 0;
void dance_1() {

  switch (dance_1_state) {
    case 0:
      setRandomColor();
      setLeftSpeed(NORMAL_SPEED);
      setRightSpeed(NORMAL_SPEED);

      start = getTime100MicroSec();
      dance_1_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setRandomColor();
        rotate(1, HIGH_SPEED);
        dance_1_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {

        start = currentTime;
        setRandomColor();
        setLeftSpeed(- NORMAL_SPEED);
        setRightSpeed(- NORMAL_SPEED);

        dance_1_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setRandomColor();
        rotate(0, HIGH_SPEED);
        dance_1_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        setRandomColor();
        setLeftSpeed(0);
        setRightSpeed(0);

        robotState = DANCE_2;

      }
      break;
  }
}

unsigned int dance_2_state = 0;
void dance_2() {

  switch (dance_2_state) {
    case 0:
      setRandomColor();
      turn(0, HIGH_SPEED);
      start = getTime100MicroSec();
      dance_2_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        turn(0, -HIGH_SPEED);
        dance_2_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setRandomColor();
        turn(1, HIGH_SPEED);

        dance_2_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        turn(1, -HIGH_SPEED);
        dance_2_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        setLeftSpeed(NORMAL_SPEED);
        setRightSpeed(NORMAL_SPEED);

        dance_2_state = 5;

      }
      break;
    case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setRandomColor();
        rotate(1, NORMAL_SPEED);
        dance_2_state = 6;

      }
      break;
    case 6:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        turn(0, HIGH_SPEED);
        dance_2_state = 7;

      }
      break;
    case 7:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        turn(0, -HIGH_SPEED);
        dance_2_state = 8;

      }
      break;
    case 8:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setRandomColor();
        turn(1, HIGH_SPEED);

        dance_2_state = 9;

      }
      break;
    case 9:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        turn(1, -HIGH_SPEED);
        dance_2_state = 10;

      }
      break;
    case 10:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setRandomColor();
        setLeftSpeed(0);
        setRightSpeed(0);

        robotState = BEFORE_DANCE;

      }
      break;

  }
}

void loop() {

  //this one is necessary, if not present the proximity sensors will not work !  YOU HAVE TO CALL IT ONCE INSIDE THE CODE, NOT OBLIGATORY HERE, you can call it in a different function
  //irCommTasks();
  /*
    if(getTime100MicroSec()-startTime > PAUSE_10_SEC){
      setRandomColor();
      startTime=getTime100MicroSec();
    }
    */


  //beforeDance();
  switch (robotState) {
    case INIT:
      //checkStart();
      //manually starting
      robotState = DANCE_2;
      //robotState = DANCE_1;
      break;
    case BEFORE_DANCE:
      beforeDance();
      //robotState = DANCE_1;
      break;
    case DANCE_1:
      dance_1();
      //robotState = DANCE_2;
      break;
    case DANCE_2:
      dance_2();
      break;
  }

  updateMotorSpeeds();
}

