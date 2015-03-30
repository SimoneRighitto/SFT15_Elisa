
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

int state = 0;
unsigned long int fredStartTime;
unsigned long int fredEndTime;

unsigned long int startTime = 0, endTime = 0, turnOffLedsTime = 0;
unsigned char prevSelector=0;
unsigned int i=0;
unsigned int currRand=0, currRand2=0;
float targetAngle=0;


void calibrate()
{
    if(calibrateOdomFlag==0)
    {
        if((getTime100MicroSec()-speedStepCounter) >= SPEED_STEP_DELAY)
        {
            speedStepCounter = getTime100MicroSec();

            if(softAccEnabled)
            {
                if(pwm_right_desired == 0)
                {
                    pwm_intermediate_right_desired = 0;
                }
                else if((pwm_right_desired*pwm_intermediate_right_desired) < 0)
                {
                    pwm_intermediate_right_desired = 0;
                }
                else if(pwm_right_desired > pwm_intermediate_right_desired)
                {
                    pwm_intermediate_right_desired += speedStep;
                    if(pwm_intermediate_right_desired > pwm_right_desired)
                    {
                        pwm_intermediate_right_desired = pwm_right_desired;
                    }
                }
                else if(pwm_right_desired < pwm_intermediate_right_desired)
                {
                    pwm_intermediate_right_desired -= speedStep;
                    if(pwm_intermediate_right_desired < pwm_right_desired)
                    {
                        pwm_intermediate_right_desired = pwm_right_desired;
                    }
                }

                if(pwm_left_desired == 0)
                {
                    pwm_intermediate_left_desired = 0;
                }
                else if((pwm_left_desired*pwm_intermediate_left_desired) < 0)
                {
                    pwm_intermediate_left_desired = 0;
                }
                else if(pwm_left_desired > pwm_intermediate_left_desired)
                {
                    pwm_intermediate_left_desired += speedStep;
                    if(pwm_intermediate_left_desired > pwm_left_desired)
                    {
                        pwm_intermediate_left_desired = pwm_left_desired;
                    }
                }
                else if(pwm_left_desired < pwm_intermediate_left_desired)
                {
                    pwm_intermediate_left_desired -= speedStep;
                    if(pwm_intermediate_left_desired < pwm_left_desired)
                    {
                        pwm_intermediate_left_desired = pwm_left_desired;
                    }
                }
            }
            else
            {
                pwm_intermediate_right_desired = pwm_right_desired;
                pwm_intermediate_left_desired = pwm_left_desired;
            }

        }
    }


}


void setup()
{
    // put your setup code here, to run once:



    initPeripherals();

    calibrateSensors();

    initBehaviors();

    startTime = getTime100MicroSec();

    speedStepCounter = getTime100MicroSec();




}



void loop()
{
    // put your main code here, to run repeatedly:






    currentSelector = getSelector();	// update selector position



    calibrate();

    endTime = getTime100MicroSec();
    if((endTime-startTime) >= (PAUSE_2_SEC))
    {
        readBatteryLevel();				// the battery level is updated every two seconds

        startTime = getTime100MicroSec();
    }


    handleMotorsWithSpeedController();

    setRightSpeed(15);
    setLeftSpeed(15);

    switch(state)
    {
    case 0:
        updateRedLed(0);
        updateGreenLed(0);
        updateBlueLed(254);
        setRightSpeed(15);
        setLeftSpeed(15);
        fredEndTime = getTime100MicroSec();
        if ((fredEndTime-fredStartTime) >= (PAUSE_2_SEC))
        {
            state=1;
            fredStartTime = getTime100MicroSec();
        }
        break;

    case 1:
        updateRedLed(0);
        updateGreenLed(254);
        updateBlueLed(0);
        setRightSpeed(-15);
        setLeftSpeed(-15);
        fredEndTime = getTime100MicroSec();
        if ((fredEndTime-fredStartTime) >= (PAUSE_2_SEC))
        {
            state=2;
            fredStartTime = getTime100MicroSec();
        }
        break;

    case 2:
        updateRedLed(254);
        updateGreenLed(0);
        updateBlueLed(0);
        setRightSpeed(15);
        setLeftSpeed(-15);
        fredEndTime = getTime100MicroSec();
        if ((fredEndTime-fredStartTime) >= (PAUSE_2_SEC))
        {
            state=0;
            fredStartTime = getTime100MicroSec();
        }
        break;
    }


}
