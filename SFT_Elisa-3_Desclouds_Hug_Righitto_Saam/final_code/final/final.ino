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

#define NOISE_THR_PERSONAL 8

unsigned long int startTime = 0, endTime = 0;

const unsigned int LOW_SPEED = 10;
const unsigned int NORMAL_SPEED = 15;
const unsigned int HIGH_SPEED = 25;

//robot state constants
unsigned int robotState;
const unsigned int INIT = 99;
const unsigned int LOW_BATTERY = 88;
const unsigned int IN_CHARGER = 77;
const unsigned int GOING_OFF_CHARGER = 66;
const unsigned int DANCE_1 = 1;
const unsigned int DANCE_2 = 2;
const unsigned int BASE_MODE = 3;



//battery constants
//const unsigned int BATTERY_LEVEL_STOP_CHARGING = 950;
//const unsigned int BATTERY_LEVEL_START_CHARGING = 750;
//const unsigned int BATTERY_LEVEL_MORE_CHARGING = 850;
const unsigned int BATTERY_LEVEL_STOP_CHARGING = 950;
const unsigned int BATTERY_LEVEL_START_CHARGING = 650;
const unsigned int BATTERY_LEVEL_MORE_CHARGING = 750;

unsigned long robotStartedTime;  // stores the time when the robot is started
unsigned long int currentTime = 0;
unsigned long int start;
unsigned int batteryStart;         // stores the last time the battery was updated
unsigned int accelerometerStart;   // stores the last time the accelerometer was updated

boolean inLine;
boolean previouslyCharging;

boolean tempActionStarted;
unsigned long tempActionDuration;   // stores the desired duration of the temporal action
unsigned long tempActionStartTime;  // stores the time when the temporal action starts

unsigned long int startDance = 0;
unsigned int base_state = 0;

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


void startTempAction(unsigned long duration) {
  tempActionStarted = true;
  tempActionStartTime = getTime100MicroSec();
  tempActionDuration = duration;
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

    if (batteryLevel <= BATTERY_LEVEL_START_CHARGING) {
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

unsigned int chargeContactDetected = 0;
boolean charging() {
  if (batteryLevel > BATTERY_LEVEL_STOP_CHARGING) {
    return false;
  }
  else if (CHARGE_ON) {
    chargeContactDetected++;
    if (chargeContactDetected > 20) {
      return true;
    }
    return false;
  }
  else {
    chargeContactDetected = 0;
    return false;
  }
}

void setup() {
  initPeripherals();
  calibrateSensors();
  initBehaviors();

  readBatteryLevel();


  enableObstacleAvoidance();
  setLeftSpeed(0);
  setRightSpeed(0);

  robotState = INIT;
  inLine = false;
  previouslyCharging = false;

  pwm_red = 255;
  pwm_green = 255;
  pwm_blue = 255;

  updateRedLed(pwm_red);
  updateGreenLed(pwm_green);
  updateBlueLed(pwm_blue);

  robotStartedTime = startTime = tempActionStartTime = speedStepCounter = batteryStart = accelerometerStart = getTime100MicroSec();
  tempActionDuration = 0;
  startTempAction(PAUSE_1_SEC);
  while (!isEndOfTempAction) {};
}

#define FRONT_PROX 35
#define BACK_PROX 40
#define LATERAL_PROX 25

boolean checkNearbyObjects() {
  unsigned int proximityResultFront = proximityResultLinear[0] + proximityResultLinear[1] + proximityResultLinear[7]   ;
  unsigned int proximityResultBack = proximityResultLinear[3] + proximityResultLinear[4] + proximityResultLinear[5]   ;

  if (proximityResultFront >= FRONT_PROX || proximityResultBack >= BACK_PROX) {
    return true;
  }
  else if ( proximityResultLinear[2] > LATERAL_PROX || proximityResultLinear [6] > LATERAL_PROX) {
    return true;
  }
  else {
    return false;
  }
}

void checkStart() {
  //to be able to start with the pc antenna
  handleRFCommands();
  turnOnGreenLeds();

  if (pwm_red == 0 && pwm_green == 255 && pwm_blue == 255) {
    robotState = DANCE_1;
    startDance = 0;
    robotStartedTime = getTime100MicroSec();
    turnOffGreenLeds();
  }
  else if (checkNearbyObjects()) {
    robotState = BASE_MODE;
    robotStartedTime = getTime100MicroSec();
    startDance = 0;
    turnOffGreenLeds();
  } else if (robotState == LOW_BATTERY) {
    robotStartedTime = getTime100MicroSec();
    turnOffGreenLeds();
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

      setLEDcolor(255, 0, 57); //turquoise
      rotate(0, HIGH_SPEED);

      start = getTime100MicroSec();
      dance_1_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_4_SEC) {
        start = currentTime;
        setLEDcolor(0, 63, 255); //dark yellow
        turnOnGreenLeds();
        setRightSpeed(NORMAL_SPEED);
        setLeftSpeed(NORMAL_SPEED);
        dance_1_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        turnOffGreenLeds();
        start = currentTime;
        setLEDcolor(171, 0, 255); //green
        rotate(1, HIGH_SPEED);

        dance_1_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_4_SEC) {
        start = currentTime;
        setLEDcolor(240, 255, 0); //blue
        turnOnGreenLeds();
        setRightSpeed(-NORMAL_SPEED);
        setLeftSpeed(-NORMAL_SPEED);
        dance_1_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(0, 159, 255); //dark orange
        turnOffGreenLeds();
        setLeftSpeed(0);
        setRightSpeed(0);
        turnOnGreenLeds();
        dance_1_state = 5;

      }
    case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_5_SEC) {
        start = currentTime;
        robotState = BASE_MODE;
        turnOffGreenLeds();
        dance_1_state = 0;
        startDance = currentTime;
        base_state = 0;

      }
      break;
  }
}

unsigned int dance_2_state = 0;
void dance_2() {

  switch (dance_2_state) {
    case 0:
      enableObstacleAvoidance();
      setLEDcolor(0, 255, 240);
      turn(0, HIGH_SPEED);
      start = getTime100MicroSec();
      dance_2_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(20, 255, 220);
        turn(0, -HIGH_SPEED);
        dance_2_state = 2;

      }
      break;
    case 2:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setLEDcolor(40, 255, 200);
        turn(1, HIGH_SPEED);

        dance_2_state = 3;

      }
      break;
    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(60, 255, 180);
        turn(1, -HIGH_SPEED);
        dance_2_state = 4;

      }
      break;
    case 4:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(80, 255, 160);
        setLeftSpeed(NORMAL_SPEED);
        setRightSpeed(NORMAL_SPEED);

        dance_2_state = 5;

      }
      break;
    case 5:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_2_SEC) {
        start = currentTime;
        setLEDcolor(100, 255, 140);
        rotate(1, NORMAL_SPEED);
        dance_2_state = 6;

      }
      break;
    case 6:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(120, 255, 100);
        turn(0, HIGH_SPEED);
        dance_2_state = 7;

      }
      break;
    case 7:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(140, 255, 60);
        turn(0, -HIGH_SPEED);
        dance_2_state = 8;

      }
      break;
    case 8:

      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {

        start = currentTime;
        setLEDcolor(160, 255, 40);
        turn(1, HIGH_SPEED);

        dance_2_state = 9;

      }
      break;
    case 9:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(180, 255, 20);
        turn(1, -HIGH_SPEED);
        dance_2_state = 10;

      }
      break;
    case 10:
      currentTime = getTime100MicroSec();
      if (currentTime - start > PAUSE_1_SEC) {
        start = currentTime;
        setLEDcolor(200, 255, 0);
        setLeftSpeed(0);
        setRightSpeed(0);

        robotState = BASE_MODE;
        dance_2_state = 0;
        startDance = currentTime;
        base_state = 0;
      }
      break;

  }
}


/*
* Basic robot behaviour: random moving whit obstacle avoidance and random RBG colors and auto charging
*/
unsigned long int startChangeColor = 0;
unsigned int randDance = 0;

void baseMode() {


  switch (base_state) {

    case 0:
      setRandomColor();
      enableObstacleAvoidance();
      setLeftSpeed(NORMAL_SPEED);
      setRightSpeed(NORMAL_SPEED);
      base_state = 1;
      break;
    case 1:


      currentTime = getTime100MicroSec();

      if ((currentTime - startChangeColor) >= (PAUSE_5_SEC)) {
        startChangeColor = currentTime;
        setRandomColor();

      }

      if ((currentTime - startDance) >= (PAUSE_40_SEC)) {
        //startDance = currentTime; this will be updated at the end of the dance in order to let the base mode continue PAUSE_40_SEC
        randDance = rand() % 2;
        switch (randDance) {
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
  }

}

void updateObstacleAvoidance() {
  if (inLine) {
    if ( (proximityResult[9] > LINE_OUT_THR) && (proximityResult[10] > LINE_OUT_THR) ) {
      inLine = false;
      setGreenLed(0, 1);
      enableObstacleAvoidance();
    }
  }
  else {
    if ( (proximityResult[9] < LINE_IN_THR) || (proximityResult[10] < LINE_IN_THR) ) {
      inLine = true;
      setGreenLed(0, 0);
      disableObstacleAvoidance();
    }
  }
}

int front_diff = 0;
void braitenbergLineFollower() {
  // proximityResult [0 - 1024], 1024 -> white, 0 -> black
  // proximityResult[8]  -> side left
  // proximityResult[9]  -> front left
  // proximityResult[10] -> front right
  // proximityResult[11] -> side right

  front_diff = ((int)proximityResult[9] - (int)proximityResult[10]) >> 4;
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


unsigned long int startGoingOff = 0;
unsigned int goingOff_state = 0;
void goingOffCharger() {

  switch (goingOff_state) {
    case 0:

      setLEDcolor(0, 250, 250);

      setLeftSpeed(0);
      setRightSpeed(0);

      startGoingOff = getTime100MicroSec();
      goingOff_state = 1;
      break;
    case 1:
      currentTime = getTime100MicroSec();
      if (currentTime - startGoingOff > PAUSE_2_SEC) {

        setLeftSpeed(-NORMAL_SPEED);
        setRightSpeed(- NORMAL_SPEED);
        goingOff_state = 2;
        startGoingOff = currentTime;

      }
      break;
    case 2:
      currentTime = getTime100MicroSec();
      if (currentTime - startGoingOff > PAUSE_2_SEC) {

        rotate(1, NORMAL_SPEED);
        goingOff_state = 3;
        startGoingOff = currentTime;

      }
      break;

    case 3:
      currentTime = getTime100MicroSec();
      if (currentTime - startGoingOff > PAUSE_1_SEC) {
        enableObstacleAvoidance();
        setLeftSpeed(0);
        setRightSpeed(0);
        robotState = BASE_MODE;
        startDance = currentTime;
        goingOff_state = 0;

      }
      break;

  }
}
unsigned int low_battery_state = 0;
void lowBattery() {

  if (previouslyCharging) {
    //only push forward untill connected
    setLeftSpeed(NORMAL_SPEED);
    setRightSpeed(NORMAL_SPEED);

    if (charging()) {
      robotState = IN_CHARGER;
      low_battery_state = 0;
    }

  } else {
    //update accelerometer
    updateAccelerometer();



    switch (low_battery_state) {

      case 0:
        setLEDcolor(0, 63, 255); //dark yellow


        // reinitialise all basic variables
        base_state = 0;
        dance_1_state = 0;
        dance_2_state = 0;
        wait_state = 0;
        low_battery_state = 1;
        break;

      case 1:
        //follow the line and go charging
        braitenbergLineFollower();
        if (charging()) {
          robotState = IN_CHARGER;
          low_battery_state = 0;
        }

        break;

    }
  }
}



void loop() {
  //update battery level
  updateBatteryLevel();

  if (!((getTime100MicroSec() - robotStartedTime) >= (2 * PAUSE_60_SEC))) { //check if is the end of the experience

    if (isEndOfTempAction()) {
      updateAccelerometer();
      switch (robotState) {
        case INIT:
          checkStart();
          //manually starting
          //robotState = LOW_BATTERY;
          break;

        case DANCE_1:
          dance_1();
          break;
        case DANCE_2:
          dance_2();
          break;
        case BASE_MODE:
          baseMode();
          break;
        case LOW_BATTERY:
          lowBattery();
          break;
        case IN_CHARGER:


          setLEDcolor(255, 0, 255);

          setLeftSpeed(0);
          setRightSpeed(0);

          if (!charging()) {

            if (batteryLevel < BATTERY_LEVEL_MORE_CHARGING) { //we need more charging
              setLEDcolor(255, 255, 0);
              previouslyCharging = true;
              robotState = LOW_BATTERY;
            }
            else {
              previouslyCharging = false;
              robotState = GOING_OFF_CHARGER;
            }
          }

          break;
        case GOING_OFF_CHARGER:
          goingOffCharger();
          break;


      }
    }

  }
  else {
    setLEDcolor(255, 255, 255);
    turnOffGreenLeds();
    setRightSpeed(0);
    setLeftSpeed(0);
    checkStart();
  }

  updateMotorSpeeds();
}

