
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



// rechargement
unsigned long int startTime = 0, endTime = 0;
//unsigned long speedStepCounter;

// contamination
unsigned int contaminated = 0; // 0 neutral, 1 victime, 2 zombie

const unsigned int DESIRED_BATTERY_LEVEL = 890;

const unsigned int LOW_SPEED=5;
const unsigned int NORMAL_SPEED=10;
const unsigned int HIGH_SPEED=15;



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

	speedStepCounter = getTime100MicroSec();


        //les variables "demo" sont utilisées pour le rechargement de la batterie
        //demoStartTime est l'heure à laquelle on démarre le rechargement alors que startTime est l'heure à laquelle on démarre tout le programme        
        demoStartTime = getTime100MicroSec();
        
        demoState = 1;
        
        pwm_red = 255;
        pwm_green = 255;
        pwm_blue = 255;
      

}


/*
* Fonction gérant le rechargement du robot.
* Le robot recherche une ligne noire sur fond blanc, la suit, s'arrête à la borne de rechargement un moment puis repart.
* La led principale est allumée en fonction de l'état du programme de rechargement.
* Dans cette fonction, on utilise 254 comme valeur de couleur et pas 255 pour ne pas risquer d'interférer avec la fonction de contamination.
*/
void rechargeBattery() {
			
	switch(demoState) {
		case 1:	// search for a line
			turnOffGreenLeds();
			GREEN_LED2_ON;
			//GREEN_LED3_ON;
			outOfLine = 0;
			enableObstacleAvoidance();
			setRightSpeed(NORMAL_SPEED);
			setLeftSpeed(NORMAL_SPEED);
			//proximityResult vaut 550 quand sur du noir, 700 quand sur du blanc, les constantes LINE_IN et LINE_OUT ont été changées
			if(proximityResult[9]<LINE_IN_THR || proximityResult[10]<LINE_IN_THR || proximityResult[8]<LINE_IN_THR || proximityResult[11]<LINE_IN_THR) {
				lineFound++;
				if(lineFound > 10) {
					outOfLine = 0;
					chargeContact = 0;
					demoStartTime = getTime100MicroSec();
					demoState = 2;
					break;
				}
			} else {
				lineFound = 0;
			}
			
                        //si le robot est placé sur le chargeur, on s'arrête (passer à l'état 3)
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
			
                       //led green
			pwm_red = 254;
			pwm_green = 1;
			pwm_blue = 254;
			break;

		case 2:	// line found, follow it
			turnOffGreenLeds();
			GREEN_LED4_ON;
			//GREEN_LED5_ON;
                        //l'évitement d'obstacle est désactivé !
			disableObstacleAvoidance();

			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_20_SEC)) {	// the robot seems to be blocked somehow
				// go back for a while
				setRightSpeed(-HIGH_SPEED);
				setLeftSpeed(-HIGH_SPEED);
				demoStartTime = getTime100MicroSec();
				demoState = 4;
				break;
			}

                        //si le robot est placé sur le chargeur, on s'arrête (passer à l'état 3)
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

                                //si la ligne perdue on recommence à en chercher une
				if(proximityResult[9]>LINE_OUT_THR && proximityResult[10]>LINE_OUT_THR) {
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
			
			if(proximityResult[8]<LINE_OUT_THR && proximityResult[9]>LINE_OUT_THR && proximityResult[10]>LINE_OUT_THR && proximityResult[11]>LINE_OUT_THR) {	// left ground is the only within the black line => turn left
				setLeftSpeed(-NORMAL_SPEED);
				setRightSpeed(HIGH_SPEED);
			} else if(proximityResult[11]<LINE_OUT_THR && proximityResult[8]>LINE_OUT_THR && proximityResult[9]>LINE_OUT_THR && proximityResult[10]>LINE_OUT_THR) {	// right ground is the only within the black line => turn right
				setLeftSpeed(HIGH_SPEED);
				setRightSpeed(-NORMAL_SPEED);
			} else if(proximityResult[9]>LINE_OUT_THR) {	// center left is leaving the line => turn right
				setLeftSpeed(HIGH_SPEED);
				setRightSpeed(-LOW_SPEED);
				//outOfLine++;
				//if(outOfLine > 250) {
				//	demoState = 1;
				//}
			} else if(proximityResult[10]>LINE_OUT_THR) {	// center right is leaving the line => turn left
				setLeftSpeed(-LOW_SPEED);
				setRightSpeed(HIGH_SPEED);
				//outOfLine++;
				//if(outOfLine > 250) {
				//	demoState = 1;
				//}
			} else {
				setRightSpeed(HIGH_SPEED);
				setLeftSpeed(HIGH_SPEED);
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
                       //led blue
			pwm_red = 254;
			pwm_green = 254;
			pwm_blue = 1;
			break;

		case 3:	// charge for some time
			turnOffGreenLeds();
			GREEN_LED6_ON;
			//GREEN_LED7_ON;
			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_30_SEC)) { // battery value checked every 30 seconds
				
				if(batteryLevel < DESIRED_BATTERY_LEVEL) {//860) {	// stay in charge if too much discharged (consider the fact that the robot
								// is still in charge thus the battery value measured is higher)
					demoStartTime = getTime100MicroSec();
					break;
				} else {
					setRightSpeed(-HIGH_SPEED);
					setLeftSpeed(-HIGH_SPEED);
					demoStartTime = getTime100MicroSec();
					demoState = 4;
					break;
				}
			}
                        //si le robot est déconnecté du chargeur, recommencer à suivre la ligne (passer à l'état 2)
			if(!CHARGE_ON) {
				chargeContact = 0;
				outOfLine = 0;
				demoState = 2;
				demoStartTime = getTime100MicroSec();
				break;			
			}	
                       //led pink
			pwm_red = 1;
			pwm_green = 254;
			pwm_blue = 1;
			break;
		
		case 4: // go back from charger
			turnOffGreenLeds();
			GREEN_LED6_ON;
			GREEN_LED7_ON;
                        setRightSpeed(-HIGH_SPEED);
			setLeftSpeed(-HIGH_SPEED);

			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_1_SEC)) {
				setRightSpeed(HIGH_SPEED);
				setLeftSpeed(-HIGH_SPEED);								
				demoStartTime = getTime100MicroSec();
				demoState = 5;							
			}	
                        //led yellow
			pwm_red = 1;
			pwm_green = 1;
			pwm_blue = 254;													
			break;

		case 5:	// turn around
			turnOffGreenLeds();
			GREEN_LED6_ON;
			GREEN_LED7_ON;
			setRightSpeed(HIGH_SPEED);
			setLeftSpeed(-HIGH_SPEED);		

			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_750_MSEC)) {
				demoStartTime = getTime100MicroSec();
				demoState = 1;							
			}	
                        //led cyan
			pwm_red = 254;
			pwm_green = 1;
			pwm_blue = 1;													
			break;							
	}

	updateRedLed(pwm_red);
	updateGreenLed(pwm_green);
	updateBlueLed(pwm_blue);
}

/*
* Fonction gérant la jeu de la contamination.
*/
void contamination() {

    // start game
    if (pwm_red == 0) {
        contaminated = 2;
    } else if (pwm_blue == 0) {
        contaminated = 1;
    }


    // check if a contamined robot is nearby
    if (contaminated == 1) {
        unsigned int proximityResultLinearSum = 0;
        int i = 0;
        for(i=0; i<8; i++) {
            if(proximityResultLinear[i] < NOISE_THR) {
                proximityResultLinear[i] = 0;
            }
            proximityResultLinearSum += proximityResultLinear[i];
        }

        // contamination
        if (proximityResultLinearSum > 100) {
            // become a zombie
            contaminated = 2;
        }
    }

    // contamination - victime
    if(contaminated == 1) {
        setLeftSpeed(0);
        setRightSpeed(0);
        pwm_red = 255;
        pwm_green = 255;
        pwm_blue = 0;
    // contamination - zombie
    } else if (contaminated == 2) {
        pwm_red = 0;
        pwm_green = 255;
        pwm_blue = 255;
        setLeftSpeed(HIGH_SPEED);
        setRightSpeed(HIGH_SPEED);
    }
    updateRedLed(pwm_red);
    updateGreenLed(pwm_green);
    updateBlueLed(pwm_blue);

}


                               

void loop() {
  // put your main code here, to run repeatedly:
  
	currentSelector = getSelector();	// update selector position

        calibrate();





      readAccelXYZ();                             // update accelerometer values to compute the angle

      computeAngle();

      handleIRRemoteCommands();

      handleRFCommands();

      usart0Transmit(currentSelector,0);                                // send the current selector position through uart as debug info

      handleMotorsWithSpeedController();
      enableObstacleAvoidance();
      
      endTime = getTime100MicroSec();
      if((endTime-startTime) >= (PAUSE_2_SEC)) {
            readBatteryLevel();				// the battery level is updated every two seconds
            startTime = getTime100MicroSec();
      }
    
      if (getTime100MicroSec() > 50000) { // il faut inclure utility.h dans ce fichier!!!!
            if(batteryLevel < DESIRED_BATTERY_LEVEL || demoState != 1) {
              //handle the battery recharge
              //rechargeBattery();
            } else {
               // handle the contamination game
              contamination();
      
      }

    
    }


}
