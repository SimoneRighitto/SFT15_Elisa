#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include "elisa3-lib.h"
#include "terminal.h"
#include "input.h"

// received from robot
int robotAddress[2];
unsigned int robProx[2][8];
unsigned int robGround[2][4];

// sent to robot
char robLSpeed=0, robRSpeed=0;
unsigned char robRedLed=0, robGreenLed=0, robBlueLed=0;

// various
unsigned int robotId=0;
unsigned int delayCounter=0;
char current_speed=0;
unsigned char exitProg=0;

int main(int argc, char *argv[]) {

    int i=0;

    for(i=0; i<2; i++) {
        printf("\r\nInsert the robot address %d: ", i);
        scanf("%d", &robotAddress[i]);
    }

    initTerminal();

    // init the communication with the robots; set the addresses and number of the robots to control
    startCommunication(robotAddress, 2);

    while(!exitProg) {

        handleKeyboardInput();

        // set new data for the current robot controlled through the keyboard
        setLeftSpeed(robotAddress[robotId], robLSpeed);
        setRightSpeed(robotAddress[robotId], robRSpeed);
        setRed(robotAddress[robotId], robRedLed);
        setBlue(robotAddress[robotId], robBlueLed);
        setGreen(robotAddress[robotId], robGreenLed);

        // update sensors data for all 4 robots
        getAllProximityFromAll(robProx);
        getAllGroundFromAll(robGround);

        printRobotInfo();

    }

    stopCommunication();
	closeTerminal();

	return 0;

}

