/*
 -------------------------------------------------------------------------------
 Fichier     : main.c
 Auteur(s)   : Maillard Stéphane, Marcel Sinniger, Jollien Dominique, Hug Auriana
 Date        : 12.01.2015

 But         : Programme exécuté sur le pc en ayant branché l'antenne de communication
 RF avec le robot Elisa-3. Permet de paramétrer le jeu de la contamination en
 désignant un robot contaminé est des robots victimes.

 Remarque(s) : 

 -------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include "elisa3-lib.h"
#include "terminal.h"
#include "input.h"
#include <windows.h>

#define NUMBER_OF_ROBOTS 3

// received from robots
int robotAddress[NUMBER_OF_ROBOTS] = {3356, 3307, 3402};
int gamePoolRobotAddresses[NUMBER_OF_ROBOTS];
int gamePoolSize = 0; // number of non-charging robots
unsigned int robProx[NUMBER_OF_ROBOTS][8];
unsigned int robGround[NUMBER_OF_ROBOTS][4];

// sent to robot
char robLSpeed=0, robRSpeed=0;
unsigned char robRedLed=0, robGreenLed=0, robBlueLed=0;

// various
unsigned int robotId=0;
unsigned int delayCounter=0;
char current_speed=0;
unsigned char exitProg=0;

// iterator vars
int i = 0;
int j = 0;

// user interaction
int firstContaminatedRobot;

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


int main(int argc, char *argv[]) {

    initTerminal(); // printf's work after this instruction only!!!

    // exclude charging robots from the game
    printf("Start communication...\n");
    startCommunication(robotAddress, NUMBER_OF_ROBOTS);
    Sleep(500);
    printf("Communication started\n");
    for (i=0, j=0; i<NUMBER_OF_ROBOTS; i++) {
        // at battery level 890 the robots are looking for a recharging station
        if (getBatteryAdc(robotAddress[i]) > 890) {
            gamePoolRobotAddresses[j] = robotAddress[i];
            gamePoolSize++;
            printf("Robot non-charging: %d \n", gamePoolRobotAddresses[j]);
            j++;
        }
    }
    printf("Stop communication...\n");
    Sleep(2000); // wait a little bit, if not, the messages may not have been received by the robots
    stopCommunication();
    Sleep(1000); // wait a little bit, if not, the communication channels are not ready for the restart
    printf("Communication stopped\n");
    printf("Game pool size: %d\n", gamePoolSize);

    startCommunication(gamePoolRobotAddresses, gamePoolSize);

    // firstly, set all robots to blue (victime)
    for(i=0; i<gamePoolSize; i++) {
        setFullBlue(gamePoolRobotAddresses[i]);
    }

    // chose the first contaminated robot (exclude charging robots)
    printf("Enter the address of the first contaminated robot : %d\n", gamePoolSize);
    printf("Robot address: ");
    scanf("%d", &firstContaminatedRobot);
    setFullRed(firstContaminatedRobot);


    Sleep(500); // wait a little bit, if not, the messages may not have been received by the robots
    stopCommunication();
	closeTerminal();

	return 0;

}

