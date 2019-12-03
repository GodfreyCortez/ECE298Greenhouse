#include "sensors.h"

void selectSensor(int selection)
{
    if (selection == LIGHT)
    {
        //check light sensor in mux A4 (100)
        GPIO_setOutputHighOnPin(MUX_SELECT_2_PORT, MUX_SELECT_2_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_1_PORT, MUX_SELECT_1_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_0_PORT, MUX_SELECT_0_PIN);
    }
    else if (selection == TEMP1)
    {
        // check temperature sensor in mux A2 (010)
        GPIO_setOutputLowOnPin(MUX_SELECT_2_PORT, MUX_SELECT_2_PIN);
        GPIO_setOutputHighOnPin(MUX_SELECT_1_PORT, MUX_SELECT_1_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_0_PORT, MUX_SELECT_0_PIN);
    }
    else if (selection == SOIL1)
    {
        // check soil sensor in mux A0 (000)
        GPIO_setOutputLowOnPin(MUX_SELECT_2_PORT, MUX_SELECT_2_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_1_PORT, MUX_SELECT_1_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_0_PORT, MUX_SELECT_0_PIN);
    }
    else if (selection == TEMP2)
    {
        // check temperature sensor in mux A1 (001)
        GPIO_setOutputLowOnPin(MUX_SELECT_2_PORT, MUX_SELECT_2_PIN);
        GPIO_setOutputLowOnPin(MUX_SELECT_1_PORT, MUX_SELECT_1_PIN);
        GPIO_setOutputHighOnPin(MUX_SELECT_0_PORT, MUX_SELECT_0_PIN);
    }
    else if (selection == SOIL2)
    {
        // check soil sensor in mux A3 (011)
        GPIO_setOutputLowOnPin(MUX_SELECT_2_PORT, MUX_SELECT_2_PIN);
        GPIO_setOutputHighOnPin(MUX_SELECT_1_PORT, MUX_SELECT_1_PIN);
        GPIO_setOutputHighOnPin(MUX_SELECT_0_PORT, MUX_SELECT_0_PIN);
    }

}

