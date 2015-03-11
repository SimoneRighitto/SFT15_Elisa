/*
 -------------------------------------------------------------------------------
 Fichier     : contamination.ino
 Auteur(s)   : Maillard Stéphane, Marcel Sinniger, Jollien Dominique, Hug Auriana
 Date        : 12.01.2015

 But         : Programme gérant le jeu de la contamination  pour le robot
 Elisa-3. Le robot doit recevoir une instruction au début du programme par
 l'antenne RF via le programme prévu pour savoir s'il est contaminé.

 Remarque(s) : 

 -------------------------------------------------------------------------------
 */
#include <sensors.h>
#include <spi.h>
#include <behaviors.h>
#include <adc.h>
#include <twimaster.h>
#include <usart.h>
#include <mirf.h>
#include <leds.h>
#include <nRF24L01.h>
#include <constants.h>
#include <ir_remote_control.h>
#include <variables.h>
#include <motors.h>
#include <utility.h>
#include <ports_io.h>
#include <speed_control.h>


unsigned char prevSelector = 0;

// contamination
unsigned int contaminated = 0; // 0 neutral, 1 victime, 2 zombie

void setup() {

    initPeripherals();
    calibrateSensors();
    initBehaviors();   

    pwm_red = 255;
    pwm_green = 255;
    pwm_blue = 255;

}

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
        setLeftSpeed(25);
        setRightSpeed(25);
    }
    updateRedLed(pwm_red);
    updateGreenLed(pwm_green);
    updateBlueLed(pwm_blue);

}

void readBatterie() {
    static startTime = getTime100MicroSec();
    endTime = getTime100MicroSec();
	if((endTime-startTime) >= (PAUSE_2_SEC)) {
		readBatteryLevel(); // the battery level is updated every two seconds

		startTime = getTime100MicroSec();
	}
}


void loop() {

    currentSelector = getSelector();                        // update selector position

    readBatterie();

    readAccelXYZ();                             // update accelerometer values to compute the angle

    computeAngle();

    handleIRRemoteCommands();

    handleRFCommands();

    usart0Transmit(currentSelector,0);                                // send the current selector position through uart as debug info

    enableObstacleAvoidance();
    
    // handle the contamination game
    if (getTime100MicroSec() > 50000) { // il faut inclure utility.h dans ce fichier!!!!
        contamination();
    }
    


    handleMotorsWithSpeedController();


}

