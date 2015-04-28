/*
 -------------------------------------------------------------------------------
 Fichier     : main.c
 Auteur(s)   : Righitto Simone
 Date        : 27.04.2015

 But         : Start up the first dancer setting him to the full red color.

 Remarque(s) :

 -------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include "elisa3-lib.h"
#include <windows.h>


// received from robots
int robotAddress[1];


// sent to robot
unsigned char robRedLed=0, robGreenLed=0, robBlueLed=0;

//various
unsigned int delayCounter=0;
char current_speed=0;
unsigned char exitProg=0;

void setFullRed(address) {
    robRedLed = 100;
    robBlueLed = 0;
    robGreenLed = 0;
    setRed(address, robRedLed);
    setBlue(address, robBlueLed);
    setGreen(address, robGreenLed);
}
void setFullBlue(address) {
    printf("Set robot %d to blue\n", address);
    robRedLed = 0;
    robBlueLed = 100;
    robGreenLed = 0;
    setRed(address, robRedLed);
    setBlue(address, robBlueLed);
    setGreen(address, robGreenLed);
}

void setFullGreen(address) {
    printf("Set robot %d to green\n", address);
    robRedLed = 0;
    robBlueLed = 0;
    robGreenLed = 100;
    setRed(address, robRedLed);
    setBlue(address, robBlueLed);
    setGreen(address, robGreenLed);
}

void setFullYellow(address) {
    printf("Set robot %d to yellow\n", address);
    robRedLed = 80;
    robBlueLed = 0;
    robGreenLed = 100;
    setRed(address, robRedLed);
    setBlue(address, robBlueLed);
    setGreen(address, robGreenLed);
}



int main(int argc, char *argv[]) {

    printf("\r\nInsert the robot address: ");
    scanf("%d", &robotAddress[0]);


    printf("Start communication...\n");
    startCommunication(robotAddress, 1);

    setFullRed(robotAddress[0]);
    Sleep(2000);


    stopCommunication();


	return 0;

}

