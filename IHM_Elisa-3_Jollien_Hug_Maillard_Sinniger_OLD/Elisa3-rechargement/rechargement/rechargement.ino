/*
 -------------------------------------------------------------------------------
 Fichier     : rechargement.ino
 Auteur(s)   : Maillard Stéphane, Marcel Sinniger, Jollien Dominique, Hug Auriana
 Date        : 12.01.2015

 But         : Programme gérant le rechargement automatique  pour le robot
 Elisa-3. Le robot doit être placé sur une surface blanche avec des lignes noires
 qui le guide vers la borne de rechargement.
 Ce programme est une adaptation de celui fournit avec la démo de base pour Elisa-3.

 Remarque(s) : 

 -------------------------------------------------------------------------------
 */
#include <avr\io.h>
#include <avr\interrupt.h>

#include <stdlib.h>
#include "variables.h"
#include "utility.h"
#include "speed_control.h"
#include "nRF24L01.h"
#include "behaviors.h"
#include "sensors.h"

unsigned long int startTime = 0, endTime = 0, turnOffLedsTime = 0;

//déclaration home-made
unsigned long speedStepCounter;

void setup() {
  	initPeripherals();

	calibrateSensors();

	initBehaviors();

	startTime = getTime100MicroSec();

	speedStepCounter = getTime100MicroSec();

        demoStartTime = getTime100MicroSec();
        
        demoState = 0;
}

void loop() {
	unsigned int i=0;
	unsigned int currRand=0, currRand2=0;
		
	readAccelXYZ();						// update accelerometer values to compute the angle

	computeAngle();

	endTime = getTime100MicroSec();
	if((endTime-startTime) >= (PAUSE_2_SEC)) {
		readBatteryLevel();				// the battery level is updated every two seconds
		startTime = getTime100MicroSec();
	}


	//if(calibrateOdomFlag==0) {
		handleIRRemoteCommands();
	//}


	//if(calibrateOdomFlag==0) {
		handleRFCommands();
	//}

      /*
      * Gestion du rechargement du robot.
      * Le robot recherche une ligne noire sur fond blanc, la suit, s'arrête à la borne de rechargement un moment puis repart.
      * La led principale est allumée en fonction de l'état du programme de rechargement.
      * Dans cette fonction, on utilise 254 comme valeur de couleur et pas 255 pour ne pas risquer d'interférer avec la fonction de contamination.
      */	
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
                  	//if((demoEndTime-demoStartTime) >= (PAUSE_20_SEC)) {
                        //pour les tests on met 1 seconde de mouvement aléatoire
			if((demoEndTime-demoStartTime) >= (PAUSE_1_SEC)) {
				demoState = 1;
			}
                        //led rouge
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
			if((demoEndTime-demoStartTime) >= (PAUSE_20_SEC)) {	// the robot seems to be blocked somehow
				// go back for a while
				setRightSpeed(-20);
				setLeftSpeed(-20);
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
				setLeftSpeed(-10);
				setRightSpeed(15);
			} else if(proximityResult[11]<LINE_OUT_THR && proximityResult[8]>LINE_OUT_THR && proximityResult[9]>LINE_OUT_THR && proximityResult[10]>LINE_OUT_THR) {	// right ground is the only within the black line => turn right
				setLeftSpeed(15);
				setRightSpeed(-10);
			} else if(proximityResult[9]>LINE_OUT_THR) {	// center left is leaving the line => turn right
				setLeftSpeed(15);
				setRightSpeed(-5);
				//outOfLine++;
				//if(outOfLine > 250) {
				//	demoState = 1;
				//}
			} else if(proximityResult[10]>LINE_OUT_THR) {	// center right is leaving the line => turn left
				setLeftSpeed(-5);
				setRightSpeed(15);
				//outOfLine++;
				//if(outOfLine > 250) {
				//	demoState = 1;
				//}
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
                        //led blue
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
                        //si le robot est déconnecté du chargeur, recommencer à suivre la ligne (passer à l'état 2)
			if(!CHARGE_ON) {
				chargeContact = 0;
				outOfLine = 0;
				demoState = 2;
				demoStartTime = getTime100MicroSec();
				break;			
			}	
                        //led pink
			pwm_red = 0;
			pwm_green = 255;
			pwm_blue = 0;
			break;
		
		case 4: // go back from charger
			turnOffGreenLeds();
			GREEN_LED6_ON;
			GREEN_LED7_ON;
                        setRightSpeed(-13);
			setLeftSpeed(-13);

			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_1_SEC)) {
				setRightSpeed(20);
				setLeftSpeed(-20);								
				demoStartTime = getTime100MicroSec();
				demoState = 5;							
			}	
                        //led yellow
			pwm_red = 0;
			pwm_green = 0;
			pwm_blue = 255;													
			break;

		case 5:	// turn around
			turnOffGreenLeds();
			GREEN_LED6_ON;
			GREEN_LED7_ON;
			setRightSpeed(20);
  			setLeftSpeed(-20);		

			demoEndTime = getTime100MicroSec();
			if((demoEndTime-demoStartTime) >= (PAUSE_750_MSEC)) {
				demoStartTime = getTime100MicroSec();
				demoState = 0;							
			}	
                        //led cyan
			pwm_red = 255;
			pwm_green = 0;
			pwm_blue = 0;													
			break;							
	}

	updateRedLed(pwm_red);
	updateGreenLed(pwm_green);
	updateBlueLed(pwm_blue);

	handleMotorsWithSpeedController();
}
