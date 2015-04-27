
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

unsigned long int startTime = 0, endTime = 0, turnOffLedsTime = 0;
unsigned char prevSelector=0;
unsigned int i=0;
unsigned int currRand=0, currRand2=0;
float targetAngle=0;
unsigned long counterPerso = 0;
unsigned int line_in = 0, line_out = 0;
bool testBool =0;

void calibrate(){
  if(calibrateOdomFlag==0) {
                           if((getTime100MicroSec()-speedStepCounter) >= SPEED_STEP_DELAY) {
				speedStepCounter = getTime100MicroSec();

				if(softAccEnabled) {
					if(pwm_right_desired == 0) {
						pwm_intermediate_right_desired = 0;
					} else if((pwm_right_desired*pwm_intermediate_right_desired) < 0) {
						pwm_intermediate_right_desired = 0;
					} else if(pwm_right_desired > pwm_intermediate_right_desired) {
						pwm_intermediate_right_desired += speedStep;
						if(pwm_intermediate_right_desired > pwm_right_desired) {
							pwm_intermediate_right_desired = pwm_right_desired;
						}
					} else if(pwm_right_desired < pwm_intermediate_right_desired) {
						pwm_intermediate_right_desired -= speedStep;
						if(pwm_intermediate_right_desired < pwm_right_desired) {
							pwm_intermediate_right_desired = pwm_right_desired;
						}					
					}
	
					if(pwm_left_desired == 0) {
						pwm_intermediate_left_desired = 0;
					} else if((pwm_left_desired*pwm_intermediate_left_desired) < 0) {
						pwm_intermediate_left_desired = 0;
					} else if(pwm_left_desired > pwm_intermediate_left_desired) {
						pwm_intermediate_left_desired += speedStep;
						if(pwm_intermediate_left_desired > pwm_left_desired) {
							pwm_intermediate_left_desired = pwm_left_desired;
						}
					} else if(pwm_left_desired < pwm_intermediate_left_desired) {
						pwm_intermediate_left_desired -= speedStep;
						if(pwm_intermediate_left_desired < pwm_left_desired) {
							pwm_intermediate_left_desired = pwm_left_desired;
						}					
					}
				} else {
					pwm_intermediate_right_desired = pwm_right_desired;
					pwm_intermediate_left_desired = pwm_left_desired;
				}

			}
		}
  
  
  }


void setup() {
  // put your setup code here, to run once:


	initPeripherals();

	calibrateSensors();

	initBehaviors();

	startTime = getTime100MicroSec();

        //speedStepCounterPersoPerso = getTime100MicroSec();

	speedStepCounter = getTime100MicroSec();

        demoState=1;
        sleep(2);
        line_out = proximityResult[8];
        line_in = line_out-50;
        
}

void loop() {
  // put your main code here, to run repeatedly
                calibrate();
                handleMotorsWithSpeedController();
		currentSelector = getSelector();	// update selector position

		readAccelXYZ();						// update accelerometer values to compute the angle

		computeAngle();
		
		//if(calibrateOdomFlag==0) {
			handleIRRemoteCommands();
		//}


		//if(calibrateOdomFlag==0) {
			handleRFCommands();
		//}

		switch(demoState) {
			case 0:	// move around
				turnOffGreenLeds();
				GREEN_LED0_ON;
				//GREEN_LED1_ON;
				lineFound = 0;
				enableObstacleAvoidance();
				setRightSpeed(20);
				setLeftSpeed(20);
				demoEndTime = getTime100MicroSec();
				if((demoEndTime-demoStartTime) >= (PAUSE_20_SEC)) {
					demoState = 1;
				}
				pwm_red = 0;
				pwm_green = 255;
				pwm_blue = 255;
				break;

			case 1:	// search for a line
				turnOffGreenLeds();
				GREEN_LED2_ON;
				//GREEN_LED3_ON;
				outOfLine = 0;
				enableObstacleAvoidance();
				setRightSpeed(15);
				setLeftSpeed(15);
				if(proximityResult[9]<line_in || proximityResult[10]<line_in || proximityResult[8]<line_in || proximityResult[11]<line_in) {
					lineFound++;
					if(lineFound > 5) {
						outOfLine = 0;
						chargeContact = 0;
						demoStartTime = getTime100MicroSec();
						demoState = 2;
						break;
					}
				} else {
					lineFound = 0;
				}
				/*
				if(CHARGE_ON) {
					chargeContact++;
					if(chargeContact > 20) {
						setLeftSpeed(0);
						setRightSpeed(0);
						demoStartTime = getTime100MicroSec();
						chargeContact = 0;
						demoState = 3;
					}
				} else {
					chargeContact = 0;
				}
				*/
				pwm_red = 255;
				pwm_green = 0;
				pwm_blue = 255;
				break;

			case 2:	// line found, follow it
				turnOffGreenLeds();
				GREEN_LED4_ON;
				//GREEN_LED5_ON;
				disableObstacleAvoidance();

				demoEndTime = getTime100MicroSec();
				if((demoEndTime-demoStartTime) >= (PAUSE_30_SEC)) {	// the robot seems to be blocked somehow
					// go back for a while
					setRightSpeed(-20);
					setLeftSpeed(-20);
					demoStartTime = getTime100MicroSec();
					demoState = 4;
					break;
				}

				if(CHARGE_ON) {
					outOfLine = 0;
					chargeContact++;
					if(chargeContact > 20) {
						setLeftSpeed(0);
						setRightSpeed(0);
						demoStartTime = getTime100MicroSec();
						demoState = 3;
						break;
					}
				} else {
					chargeContact = 0;

					if(proximityResult[9]>line_out && proximityResult[10]>line_out) {
						outOfLine++;
						if(outOfLine > 250) {
							chargeContact = 0;
							demoState = 1;
							break;
						}
					} else {
						outOfLine = 0;
					}
				}
				if (proximityResult[1] >= 700 || proximityResult[7] >= 700) {
                                        if ((getTime100MicroSec()-counterPerso) >= PAUSE_1_SEC) {
                                          counterPerso = getTime100MicroSec();
                                          if (testBool == 1) {testBool = 0;}
                                          else {testBool = 1;}
                                        }
                                        if (testBool == 1) {
                                          setRightSpeed(15);
					  setLeftSpeed(-5);
                                        } else {
                                          setRightSpeed(-5);
					  setLeftSpeed(15);
                                        }
                                }
				else if(proximityResult[9]>line_out) {	// center left is leaving the line => turn right
					setLeftSpeed(15);
					setRightSpeed(-5);
					//outOfLine++;
					//if(outOfLine > 250) {
					//	demoState = 1;
					//}
				} else if(proximityResult[10]>line_out) {	// center right is leaving the lnie => turn left
					setLeftSpeed(-5);
					setRightSpeed(15);
					//outOfLine++;
					//if(outOfLine > 250) {
					//	demoState = 1;
					//}
				} else if(proximityResult[8]<line_out && proximityResult[9]>line_out && proximityResult[10]>line_out && proximityResult[11]>line_out) {	// left ground is the only within the black line => turn left
					setLeftSpeed(-10);
					setRightSpeed(15);
				} else if(proximityResult[11]<line_out && proximityResult[8]>line_out && proximityResult[9]>line_out && proximityResult[10]>line_out) {	// right ground is the only within the black line => turn right
					setLeftSpeed(15);
					setRightSpeed(-10);
				} else {
					setRightSpeed(15);
					setLeftSpeed(15);
					//outOfLine = 0;
					/*
					if(CHARGE_ON) {
						outOfLine = 0;
						chargeContact++;
						if(chargeContact > 20) {
							setLeftSpeed(0);
							setRightSpeed(0);
							demoStartTime = getTime100MicroSec();
							demoState = 3;
						}
					} else {
						chargeContact = 0;
					}
					*/
				}
				pwm_red = 255;
				pwm_green = 255;
				pwm_blue = 0;
				break;

			case 3:	// charge for some time
				turnOffGreenLeds();
				GREEN_LED6_ON;
				//GREEN_LED7_ON;
				demoEndTime = getTime100MicroSec();
				if((demoEndTime-demoStartTime) >= (PAUSE_30_SEC)) {
					if(batteryLevel<890) {//860) {	// stay in charge if too much discharged (consider the fact that the robot
											// is still in charge thus the battery value measured is higher)
						demoStartTime = getTime100MicroSec();
						break;
					} else {
						setRightSpeed(-13);
						setLeftSpeed(-13);
						demoStartTime = getTime100MicroSec();
						demoState = 4;
						break;
					}
				}
				if(!CHARGE_ON) {
					chargeContact = 0;
					outOfLine = 0;
					demoState = 2;
					demoStartTime = getTime100MicroSec();
					break;						
				}	
				pwm_red = 0;
				pwm_green = 255;
				pwm_blue = 0;
				break;
			
			case 4: // go back from charger
				turnOffGreenLeds();
				GREEN_LED6_ON;
				GREEN_LED7_ON;
				demoEndTime = getTime100MicroSec();
				if((demoEndTime-demoStartTime) >= (PAUSE_1_SEC)) {
					setRightSpeed(20);
					setLeftSpeed(-20);								
					demoStartTime = getTime100MicroSec();
					demoState = 5;							
				}	
				pwm_red = 0;
				pwm_green = 0;
				pwm_blue = 255;													
				break;

			case 5:	// turn around
				turnOffGreenLeds();
				GREEN_LED6_ON;
				GREEN_LED7_ON;
				demoEndTime = getTime100MicroSec();
				if((demoEndTime-demoStartTime) >= (PAUSE_750_MSEC)) {
					demoStartTime = getTime100MicroSec();
					demoState = 0;							
				}	
				pwm_red = 255;
				pwm_green = 0;
				pwm_blue = 0;													
				break;							
		}

		updateRedLed(pwm_red);
		updateGreenLed(pwm_green);
		updateBlueLed(pwm_blue);

}
