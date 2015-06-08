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
const unsigned int LOW_BATTERY = 88;
const unsigned int IN_CHARGER = 77;

const unsigned int BEFORE_DANCE = 0;
const unsigned int DANCE_1 = 1;
const unsigned int DANCE_2 = 2;
const unsigned int BASE_MODE = 3;

//communication constatns
unsigned char NO_DATA = 0;

//battery constants
const unsigned int BATTERY_LEVEL_STOP_CHARGING = 950;
const unsigned int BATTERY_LEVEL_START_CHARGING = 940;


unsigned long int currentTime = 0;
unsigned long int start;
unsigned int batteryStart;         // stores the last time the battery was updated
unsigned int accelerometerStart;   // stores the last time the accelerometer was updated

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

// update the battery level
// if the level is below the minimal level, the robot enter the charging state
void updateBatteryLevel() {
  if (getTime100MicroSec() > (batteryStart + PAUSE_5_SEC)) {    // update the battery level every 5 seconds
    readBatteryLevel();
    batteryStart = getTime100MicroSec();

    if(batteryLevel <= BATTERY_LEVEL_START_CHARGING){
        //the robot must now go to charging state
        robotState = LOW_BATTERY;
    }
  }
}

void updateAccelerometer() {
  if (getTime100MicroSec() > (accelerometerStart + PAUSE_200_MSEC)) {    // update the accelerometer every 200 ms
    readAccelXYZ();
    accelerometerStart = getTime100MicroSec();
  }
}

boolean charging() {
  if (batteryLevel > BATTERY_LEVEL_STOP_CHARGING) {
    return false;
  }
  else if (CHARGE_ON) {
    return true;
  }
  else {
    return false;
  }
}

void setup() {
  initPeripherals();
  calibrateSensors();
  initBehaviors();
  irCommInit();
  readBatteryLevel();
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
    unsigned char sensed = NO_DATA;
  //to be able to start with the pc antenna
  handleRFCommands();
  turnOnGreenLeds();
  if (pwm_red == 0 && pwm_green == 255 && pwm_blue == 255) {
    robotState=BASE_MODE;
    turnOffGreenLeds();
  }
  sensed = senseCommunication();

      if(sensed!=NO_DATA){
        setLEDcolor(255,0,255);
        robotState = BEFORE_DANCE;

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
unsigned int randDance=0;
void beforeDance() {


  int beforeDanceDelay[] = {PAUSE_1_SEC, PAUSE_750_MSEC, PAUSE_500_MSEC, PAUSE_300_MSEC, PAUSE_300_MSEC, PAUSE_300_MSEC, PAUSE_250_MSEC, PAUSE_250_MSEC, PAUSE_100_MSEC};

    setLeftSpeed(0);
    setRightSpeed(0);

  if (index >= (sizeof(beforeDanceDelay) / sizeof(int))) {
    index = 0;
    beforeGreenLedState = 0;
    randDance = rand()%2;
    switch(randDance){
    case 0:
        robotState = DANCE_1;
        break;
    case 1:
        robotState = DANCE_2;
        break;
    default:
        robotState = DANCE_1;
    }

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
      enableObstacleAvoidance();

      setLEDcolor(255,0,57); //turquoise
      setLeftSpeed(NORMAL_SPEED);
      setRightSpeed(NORMAL_SPEED);

      start = getTime100MicroSec();
      dance_1_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(0,63,255); //dark yellow
        rotate(1, HIGH_SPEED);
        dance_1_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {

        start = currentTime;
         setLEDcolor(171,0,255); //green
        setLeftSpeed(- NORMAL_SPEED);
        setRightSpeed(- NORMAL_SPEED);

        dance_1_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(240,255,0); //blue
        rotate(0, HIGH_SPEED);
        dance_1_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        setLEDcolor(0,159,255); //dark orange
        setLeftSpeed(0);
        setRightSpeed(0);

         dance_1_state=5;

      }
          case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        robotState = INIT;
        dance_1_state=0;

      }
      break;
  }
}

unsigned int dance_2_state = 0;
void dance_2() {

  switch (dance_2_state) {
    case 0:
      enableObstacleAvoidance();
      setLEDcolor(0,255,240);
      turn(0, HIGH_SPEED);
      start = getTime100MicroSec();
      dance_2_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(20,255,220);
        turn(0, -HIGH_SPEED);
        dance_2_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setLEDcolor(40,255,200);
        turn(1, HIGH_SPEED);

        dance_2_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(60,255,180);
        turn(1, -HIGH_SPEED);
        dance_2_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(80,255,160);
        setLeftSpeed(NORMAL_SPEED);
        setRightSpeed(NORMAL_SPEED);

        dance_2_state = 5;

      }
      break;
    case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(100,255,140);
        rotate(1, NORMAL_SPEED);
        dance_2_state = 6;

      }
      break;
    case 6:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(120,255,100);
        turn(0, HIGH_SPEED);
        dance_2_state = 7;

      }
      break;
    case 7:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(140,255,60);
        turn(0, -HIGH_SPEED);
        dance_2_state = 8;

      }
      break;
    case 8:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setLEDcolor(160,255,40);
        turn(1, HIGH_SPEED);

        dance_2_state = 9;

      }
      break;
    case 9:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(180,255,20);
        turn(1, -HIGH_SPEED);
        dance_2_state = 10;

      }
      break;
    case 10:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(200,255,0);
        setLeftSpeed(0);
        setRightSpeed(0);

        robotState = INIT;
        dance_2_state = 0;

      }
      break;

  }
}

void sendToRobots(unsigned char toSend) {
  if (irCommDataSent() == 1) {
    irCommSendData(toSend);
  }
  irCommTasks();
}

unsigned char senseCommunication() {
  unsigned char received = NO_DATA;

  irCommTasks();
  if (irCommDataAvailable() == 1) {
    received = irCommReadData();
  }

  return received;
}


/*
* Basic robot behaviour: random moving whit obstacle avoidance and random RBG colors and auto charging
*/
unsigned long int startChangeColor = 0;
unsigned int base_state=0;
unsigned char received=NO_DATA;
void baseMode(){


    switch(base_state){

case 0:
        setRandomColor();
		enableObstacleAvoidance();
		setLeftSpeed(NORMAL_SPEED);
		setRightSpeed(NORMAL_SPEED);
		base_state=1;
        break;
case 1:


    currentTime = getTime100MicroSec();

      if((currentTime -startChangeColor) >= (PAUSE_5_SEC)) {
        startChangeColor = currentTime;
        setRandomColor();

      }

      //handle others robot contact
      sendToRobots(1);
      received = senseCommunication();

      if(received!=NO_DATA){
        setLEDcolor(255,0,255);
        robotState = BEFORE_DANCE;
        base_state=0;
      }

      }

}

void braitenbergLineFollower() {
setLEDcolor(255,0,0);
}


void loop() {
  //update battery level
  updateBatteryLevel();


  switch (robotState) {
    case INIT:
      //checkStart();
      //manually starting
      robotState = BASE_MODE;
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
    case BASE_MODE:
        baseMode();
        break;
    case LOW_BATTERY:
        //implement the charging routine

        // reinitialise all basic variables
        base_state=0;
         //update accelerometer
        updateAccelerometer();

        //TODO here follow the line and go charging
        braitenbergLineFollower();
        break;
    case IN_CHARGER:
      setLEDcolor(255, 255, 255);
      turnOnGreenLeds();
      setLeftSpeed(0);
      setRightSpeed(0);
      if(!charging()){
        robotState = BASE_MODE;
        turnOffGreenLeds();
      }
      break;
  }

  updateMotorSpeeds();
}
