#include "greenhouse.h"

Timer_A_outputPWMParam param2; //Timer configuration data structure for PWM
Timer_A_initUpModeParam param = { 0 };
Timer_A_initCompareModeParam initComp1Param = { 0 };
Timer_A_initCompareModeParam initComp2Param = { 0 };

char ADCState = 0; //Busy state of the ADC
int16_t ADCResult = 0; //Storage for the ADC conversion result
int16_t sensorSamples[5][SAMPLING_COUNT];
int16_t averages[5];
int16_t soilMax = 0xFF;
char displayTemp = 1;
int sampled = 0;
char zoneSelected = '1';
char displayString[32];
char buttonState1 = 0; //Current button press state (to allow edge detection)
char buttonState2 = 0;
int sensorToMeasure;
static int16_t soilThreshold = 0; //anything below this will activate it
static int16_t tempThreshold = 0; // anything above this in centigrade
static int16_t lightThreshold = 0x40; // anything less than this means its night time

uint8_t uartmsg[32];
uint8_t uartmsgindex = 0;

char TEMP1ON = 0;
char TEMP2ON = 0;
char SOIL1ON = 0;
char SOIL2ON = 0;

//overrides by UART the device will not control the sensors if activated by the computer
char TEMP1UART = 0;
char SOIL1UART = 0;
char TEMP2UART = 0;
char SOIL2UART = 0;

char uartInput = 0;
int main(void)
{
    LaunchpadInit();

    displayScrollText("SESSION 4 GRP 12");

    uint8_t msg[256] = {0};
    sprintf((char *) msg, "Hello, from UART!!!");
    displayConsole(msg, strlen((char *) msg));

    __delay_cycles(500000);
    strcpy((char *) msg, "");
    sprintf((char *) msg, "Threshold for Soil? (0-100)");
    displayConsole(msg, strlen((char *) msg));

    while (soilThreshold == 0);

    strcpy((char *) msg, "");
    sprintf((char *) msg, "Threshold for Temperature? (degrees Celsius)");
    displayConsole(msg, strlen((char *) msg));

    while (tempThreshold == 0);

    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2); //standby
    Timer_A_outputPWM(TIMER_A0_BASE, &param2);

    while (1)
    {
        if(uartInput) {
            strcpy((char *) msg, " ");
            displayConsole(msg, strlen((char *) msg));
            uartInput = 0;
        }
        int i = 0;
        if (SOIL1ON)
        {
            for (i = 0; i < 10; i++)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
                __delay_cycles(1000);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
                __delay_cycles(19000);
            }
            __delay_cycles(500000);
            for (i = 0; i < 10; i++)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN4);
                __delay_cycles(2000);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN4);
                __delay_cycles(18000);
            }
        }

        if (SOIL2ON)
        {
            for (i = 0; i < 10; i++)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
                __delay_cycles(1000);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
                __delay_cycles(19000);
            }
            __delay_cycles(500000);
            for (i = 0; i < 10; i++)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN5);
                __delay_cycles(2000);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN5);
                __delay_cycles(18000);
            }
        }

        // sample each sensor 5 times and take the average
        for (i = 0; i < 5; i++)
        {
            //will select in this order LIGHT, TEMP1, TEMP2, SOIL1, SOIL2
            selectSensor(i);
            sampled = 0;
            startADC();
            while (sampled < SAMPLING_COUNT)
            {
                if (ADCState == 0)
                {
                    sensorSamples[i][sampled] = ADCResult;
                    startADC();
                    sampled++;
                }
            }

            // after sampling 5 times store the result
            int j = 0;
            for (j = 0; j < SAMPLING_COUNT; j++)
            {
                averages[i] += sensorSamples[i][j];
            }

            averages[i] /= SAMPLING_COUNT;

        }

        changeZone(); //check if they pressed the pb to change zone

        // display info depending on the zone
        displayInfo();

        //turn on the corresponding motors when necessary
        char nighttime = averages[LIGHT] < lightThreshold;
        if (nighttime)
        {
            // irrigation motors only during the night
            // if the soil is not moist enough turn on the irrigation motors
            // do not do anything if UART is controlling the motors
            if (!TEMP1UART)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
                TEMP1ON = 0;
            }
            if (!TEMP2UART)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                TEMP2ON = 0;
            }
            if (calculateSoil(averages[SOIL1]) < soilThreshold && !SOIL1UART
                    && !SOIL1ON)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
                SOIL1ON = 1;
            }
            else if (calculateSoil(averages[SOIL1]) >= soilThreshold
                    && !SOIL1UART && SOIL1ON)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                SOIL1ON = 0;
            }

            if (calculateSoil(averages[SOIL2]) < soilThreshold && !SOIL2UART
                    && !SOIL2ON)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);
                SOIL2ON = 1;
            }
            else if (calculateSoil(averages[SOIL2]) >= soilThreshold
                    && !SOIL2UART && SOIL2ON)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
                SOIL2ON = 0;
            }
        }
        else
        {
            // ventilation motors only during the day
            //if it gets hot enough turn on the fans
            if (!SOIL1UART)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                SOIL1ON = 0;
            }

            if (!SOIL2UART)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
                SOIL2ON = 0;
            }

            if (calculateTemp(averages[TEMP1]) > tempThreshold && !TEMP1UART
                    && !TEMP1ON)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6);
                TEMP1ON = 1;
            }
            else if (calculateTemp(averages[TEMP1]) <= tempThreshold
                    && !TEMP1UART && TEMP1ON)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
                TEMP1ON = 0;
            }

            if (calculateTemp(averages[TEMP2]) > tempThreshold && !TEMP2UART
                    && !TEMP2ON)
            {
                GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                TEMP2ON = 1;
            }
            else if (calculateTemp(averages[TEMP2]) <= tempThreshold
                    && !TEMP2UART && TEMP2ON)
            {
                GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
                GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                TEMP2ON = 0;
            }

        }
    }
    return 0;
}

int16_t calculateSoil(int16_t average)
{
    double soilPercentage = (double) average / (double) soilMax;
    soilPercentage *= 100;
    return (int16_t) soilPercentage;
}
int16_t calculateTemp(int16_t average)
{
    return (average) / 10 - 6;
}
void changeZone(void)
{
    //Buttons SW1 and SW2 are active low (1 until pressed, then 0)
    if ((GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 0) && buttonState2 == 1)
    {
        buttonState2 = 0;
        zoneSelected++;
        if (zoneSelected >= '3')
        {
            zoneSelected = '1';
        }
        showChar(zoneSelected, pos1);

    }

    if ((GPIO_getInputPinValue(SW2_PORT, SW2_PIN) == 1) && buttonState2 == 0)
    {
        showChar(zoneSelected, pos1);
        buttonState2 = 1;
    }
}
void displayInfo(void)
{

    if ((GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 0) && buttonState1 == 1)
    {
        displayTemp ^= 0x1;
        buttonState1 = 0;
    }

    if ((GPIO_getInputPinValue(SW1_PORT, SW1_PIN) == 1) & (buttonState1 == 0)) //Look for rising edge
    {
        buttonState1 = 1;                //Capture new button state
    }

    //display the temperature of the sensor by default
    if (displayTemp)
    {
        int16_t temp =
                (zoneSelected == '1') ? averages[TEMP1] : averages[TEMP2];
        temp = calculateTemp(temp);
        showChar('T', pos3);
        char temperatureString[3];
        sprintf(temperatureString, "%d", temp);
        showChar(temperatureString[0], pos4);
        if (temp > 9)
            showChar(temperatureString[1], pos5);
        else
            showChar(' ', pos5);
        showChar('C', pos6);
    }
    else
    {
        int16_t soil =
                (zoneSelected == '1') ? averages[SOIL1] : averages[SOIL2];
        int16_t soilPercentage = calculateSoil(soil);
        showChar('S', pos3);
        char soilString[3];
        sprintf(soilString, "%d", soilPercentage);
        showChar(soilString[0], pos4);
        if (soilPercentage > 9)
        {
            showChar(soilString[1], pos5);
        }
        else
        {
            showChar(' ', pos5);
        }
        showChar(' ', pos6);
    }

}

void startADC(void)
{
    ADCState = 1;
    ADC_startConversion(ADC_BASE, ADC_SINGLECHANNEL);
}

void Init_GPIO(void)
{
    // Set all GPIO pins to output low to prevent floating input and reduce power consumption
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P1,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P2,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P3,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P4,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P5,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P6,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P7,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setOutputLowOnPin(
            GPIO_PORT_P8,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);

    GPIO_setAsOutputPin(
            GPIO_PORT_P1,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P2,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P3,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P4,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P5,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P6,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P7,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);
    GPIO_setAsOutputPin(
            GPIO_PORT_P8,
            GPIO_PIN0 | GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3 | GPIO_PIN4
                    | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7);

    //Set LaunchPad switches as inputs - they are active low, meaning '1' until pressed
    GPIO_setAsInputPinWithPullUpResistor(SW1_PORT, SW1_PIN);
    GPIO_setAsInputPinWithPullUpResistor(SW2_PORT, SW2_PIN);

    //Set LED1 and LED2 as outputs
    //GPIO_setAsOutputPin(LED1_PORT, LED1_PIN); //Comment if using UART
    GPIO_setAsOutputPin(LED2_PORT, LED2_PIN);
}

/* Clock System Initialization */
void Init_Clock(void)
{
    /*
     * The MSP430 has a number of different on-chip clocks. You can read about it in
     * the section of the Family User Guide regarding the Clock System ('cs.h' in the
     * driverlib).
     */

    /*
     * On the LaunchPad, there is a 32.768 kHz crystal oscillator used as a
     * Real Time Clock (RTC). It is a quartz crystal connected to a circuit that
     * resonates it. Since the frequency is a power of two, you can use the signal
     * to drive a counter, and you know that the bits represent binary fractions
     * of one second. You can then have the RTC module throw an interrupt based
     * on a 'real time'. E.g., you could have your system sleep until every
     * 100 ms when it wakes up and checks the status of a sensor. Or, you could
     * sample the ADC once per second.
     */
    //Set P4.1 and P4.2 as Primary Module Function Input, XT_LF
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4,
                                               GPIO_PIN1 + GPIO_PIN2,
                                               GPIO_PRIMARY_MODULE_FUNCTION);

    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768);
    // Set ACLK = XT1
    CS_initClockSignal(CS_ACLK, CS_XT1CLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Initializes the XT1 crystal oscillator
    CS_turnOnXT1LF(CS_XT1_DRIVE_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLKDIV_SELECT, CS_CLOCK_DIVIDER_1);

}
/* UART Initialization */
void Init_UART(void)
{
    /* UART: It configures P1.0 and P1.1 to be connected internally to the
     * eSCSI module, which is a serial communications module, and places it
     * in UART mode. This let's you communicate with the PC via a software
     * COM port over the USB cable. You can use a console program, like PuTTY,
     * to type to your LaunchPad. The code in this sample just echos back
     * whatever character was received.
     */

    //Configure UART pins, which maps them to a COM port over the USB cable
    //Set P1.0 and P1.1 as Secondary Module Function Input.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN1,
                                               GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P1, GPIO_PIN0,
                                                GPIO_PRIMARY_MODULE_FUNCTION);

    /*
     * UART Configuration Parameter. These are the configuration parameters to
     * make the eUSCI A UART module to operate with a 9600 baud rate. These
     * values were calculated using the online calculator that TI provides at:
     * http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
     */

    //ACLK = 32.768kHz, Baudrate = 9600
    //UCBRx = 3, UCBRFx = 0, UCBRSx = 146, UCOS16 = 0
    //SMCLK = 1MHz, Baudrate = 115200
    //UCBRx = 8, UCBRFx = 0, UCBRSx = 214, UCOS16 = 0
    //SMCLK = 8MHz, Baudrate = 115200
    //UCBRx = 4, UCBRFx = 5, UCBRSx = 85, UCOS16 = 1
    //SMCLK = 1MHz, Baudrate = 9600
    //UCBRx = 6, UCBRFx = 8, UCBRSx = 17, UCOS16 = 1
    EUSCI_A_UART_initParam param3 = { 0 };
    param3.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param3.clockPrescalar = 8;
    param3.firstModReg = 0;
    param3.secondModReg = 214;
    param3.parity = EUSCI_A_UART_NO_PARITY;
    param3.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param3.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param3.uartMode = EUSCI_A_UART_MODE;
    param3.overSampling = 0;

    if (STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param3))
    {
        return;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    EUSCI_A_UART_clearInterrupt(
            EUSCI_A0_BASE,
            EUSCI_A_UART_RECEIVE_INTERRUPT | EUSCI_A_UART_TRANSMIT_INTERRUPT);

    // Enable EUSCI_A0 RX and TX interrupt
    EUSCI_A_UART_enableInterrupt(
            EUSCI_A0_BASE,
            EUSCI_A_UART_RECEIVE_INTERRUPT | EUSCI_A_UART_TRANSMIT_INTERRUPT);
}

/* EUSCI A0 UART ISR - Echoes data back to PC host */
#pragma vector=USCI_A0_VECTOR
__interrupt
void EUSCIA0_ISR(void)
{

    uint8_t RxStatus = EUSCI_A_UART_getInterruptStatus(
            EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG);
    uint8_t TxStatus = EUSCI_A_UART_getInterruptStatus(
            EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);

    if (RxStatus)
    {
        uint8_t input = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);
        if (input == '\r')
        {
            if (soilThreshold == 0)
            {
                soilThreshold = atoi((char *) uartmsg);
            }
            else if (tempThreshold == 0)
            {
                tempThreshold = atoi((char *) uartmsg);
            }
            else if (strstr((char *) uartmsg, "S1"))
            {
                if (SOIL1UART == 0)
                {
                    SOIL1UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
                    SOIL1ON = 1;
                }
                else
                {
                    SOIL1ON = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                    SOIL1UART = 0;
                }
            }
            else if (strstr((char *) uartmsg, "S2"))
            {
                if (SOIL2UART == 0)
                {
                    SOIL2UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);
                    SOIL2ON = 1;
                }
                else
                {
                    SOIL2ON = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
                    SOIL2UART = 0;
                }
            }
            else if (strstr((char *) uartmsg, "T1"))
            {
                if (TEMP1UART == 0)
                {
                    TEMP1UART = 1;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6);
                }
                else
                {
                    TEMP1UART = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
                }
            }
            else if (strstr((char *) uartmsg, "T2"))
            {
                if (TEMP2UART == 0)
                {
                    TEMP2UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                }
                else
                {
                    TEMP2UART = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                }
            }
            else if (strstr((char *) uartmsg, "ALL"))
            {
                if (TEMP2UART == 0)
                {
                    TEMP2UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                }
                else
                {
                    TEMP2UART = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN3);
                }
                if (TEMP1UART == 0)
                {
                    TEMP1UART = 1;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6);
                }
                else
                {
                    TEMP1UART = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0);
                    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6);
                }

                if (SOIL2UART == 0)
                {
                    SOIL2UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN2);
                    SOIL2ON = 1;
                }
                else
                {
                    SOIL2ON = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN2);
                    SOIL2UART = 0;
                }
                if (SOIL1UART == 0)
                {
                    SOIL1UART = 1;
                    GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN5);
                    SOIL1ON = 1;
                }
                else
                {
                    SOIL1ON = 0;
                    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN5);
                    SOIL1UART = 0;
                }
            }
            memset(uartmsg, 0, uartmsgindex + 1);
            uartmsgindex = 0;
            uartInput = 1;
        }
        else
        {
            uartmsg[uartmsgindex] = input;
            uartmsgindex++;
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, input);
        }
    }

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, RxStatus);

    EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, TxStatus);

}

/* PWM Initialization */
void Init_PWM(void)
{
    /*
     * The internal timers (TIMER_A) can auto-generate a PWM signal without needing to
     * flip an output bit every cycle in software. The catch is that it limits which
     * pins you can use to output the signal, whereas manually flipping an output bit
     * means it can be on any GPIO. This function populates a data structure that tells
     * the API to use the timer as a hardware-generated PWM source.
     *
     */
//    //Generate PWM - Timer runs in Up-Down mode
    param2.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    param2.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param2.timerPeriod = TIMER_A_PERIOD; //Defined in main.h
    param2.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    param2.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    param2.dutyCycle = HIGH_COUNT; //Defined in main.h

    //PWM_PORT PWM_PIN (defined in main.h) as PWM output

    GPIO_setAsPeripheralModuleFunctionOutputPin(PWM_PORT, PWM_PIN,
                                                GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P8, GPIO_PIN3,
                                                GPIO_PRIMARY_MODULE_FUNCTION);
    param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK;
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_1;
    param.timerPeriod = TIMER_A_PERIOD;
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    param.captureCompareInterruptEnable_CCR0_CCIE =
            TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = true;

    Timer_A_initUpMode(TIMER_A1_BASE, &param);

    Timer_A_initCompareModeParam initComp1Param = { 0 };
    initComp1Param.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_1;
    initComp1Param.compareInterruptEnable =
            TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    initComp1Param.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    initComp1Param.compareValue = HIGH_COUNT;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initComp1Param);

    //Initialize compare mode to generate PWM2
    Timer_A_initCompareModeParam initComp2Param = { 0 };
    initComp2Param.compareRegister = TIMER_A_CAPTURECOMPARE_REGISTER_2;
    initComp2Param.compareInterruptEnable =
            TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE;
    initComp2Param.compareOutputMode = TIMER_A_OUTPUTMODE_RESET_SET;
    initComp2Param.compareValue = HIGH_COUNT;
    Timer_A_initCompareMode(TIMER_A1_BASE, &initComp2Param);
}

void Init_ADC(void)
{
    /*
     * To use the ADC, you need to tell a physical pin to be an analog input instead
     * of a GPIO, then you need to tell the ADC to use that analog input. Defined
     * these in main.h for A9 on P8.1.
     */

    //Set ADC_IN to input direction
    GPIO_setAsPeripheralModuleFunctionInputPin(ADC_IN_PORT, ADC_IN_PIN,
                                               GPIO_PRIMARY_MODULE_FUNCTION);

    //Initialize the ADC Module
    /*wock divider of 1
     */
    ADC_init(ADC_BASE,
    ADC_SAMPLEHOLDSOURCE_SC,
             ADC_CLOCKSOURCE_ADCOSC,
             ADC_CLOCKDIVIDER_1);

    ADC_enable(ADC_BASE);

    /*
     * Base Address for the ADC Module
     * Sample/hold for 16 clock cycles
     * Do not enable Multiple Sampling
     */
    ADC_setupSamplingTimer(ADC_BASE,
    ADC_CYCLEHOLD_16_CYCLES,
                           ADC_MULTIPLESAMPLESDISABLE);

    //Configure Memory Buffer
    /*
     * Base Address for the ADC Module
     * Use input ADC_IN_CHANNEL
     * Use positive reference of AVcc
     * Use negative reference of AVss
     */
    ADC_configureMemory(ADC_BASE,
    ADC_IN_CHANNEL,
                        ADC_VREFPOS_AVCC,
                        ADC_VREFNEG_AVSS);

    ADC_clearInterrupt(ADC_BASE,
    ADC_COMPLETED_INTERRUPT);

    //Enable Memory Buffer interrupt
    ADC_enableInterrupt(ADC_BASE,
    ADC_COMPLETED_INTERRUPT);
}

//ADC interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt
void ADC_ISR(void)
{
    uint8_t ADCStatus = ADC_getInterruptStatus(ADC_BASE,
                                               ADC_COMPLETED_INTERRUPT_FLAG);

    ADC_clearInterrupt(ADC_BASE, ADCStatus);

    if (ADCStatus)
    {
        ADCState = 0; //Not busy anymore
        ADCResult = ADC_getResults(ADC_BASE);
    }
}

void LaunchpadInit(void)
{
    __disable_interrupt();

    WDT_A_hold(WDT_A_BASE);

    // Initializations - see functions for more detail
    Init_GPIO();    //Sets all pins to output low as a default
    Init_PWM();     //Sets up a PWM output
    Init_ADC();     //Sets up the ADC to sample
    Init_Clock();   //Sets up the necessary system clocks
    Init_UART();    //Sets up an echo over a COM port
    Init_LCD();     //Sets up the LaunchPad LCD display

    PMM_unlockLPM5(); //Disable the GPIO power-on default high-impedance mode to activate previously configured port settings

    //All done initializations - turn interrupts back on.
    __enable_interrupt();
}
