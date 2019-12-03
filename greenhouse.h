/*
 * greenhouse.h
 *
 *  Created on: Nov 20, 2019
 *      Author: Godfr
 */

#ifndef GREENHOUSE_H_
#define GREENHOUSE_H_
#include <msp430.h>
#include <msp430fr4133.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sensors.h"
#include "uart.h"
#include "driverlib/driverlib.h"
#include "hal_LCD.h"

#define SAMPLING_COUNT 5

#define TIMER_A_PERIOD  1000 //T = 1/f = (TIMER_A_PERIOD * 1 us)
#define HIGH_COUNT      500  //Number of cycles signal is high (Duty Cycle = HIGH_COUNT / TIMER_A_PERIOD)

//Output pin to buzzer
#define PWM_PORT        GPIO_PORT_P1
#define PWM_PIN         GPIO_PIN7
//LaunchPad LED1 - note unavailable if UART is used
#define LED1_PORT       GPIO_PORT_P1
#define LED1_PIN        GPIO_PIN0
//LaunchPad LED2
#define LED2_PORT       GPIO_PORT_P4
#define LED2_PIN        GPIO_PIN0
//LaunchPad Pushbutton Switch 1
#define SW1_PORT        GPIO_PORT_P1
#define SW1_PIN         GPIO_PIN2
//LaunchPad Pushbutton Switch 2
#define SW2_PORT        GPIO_PORT_P2
#define SW2_PIN         GPIO_PIN6
//Input to ADC - in this case input A9 maps to pin P8.1
#define ADC_IN_PORT     GPIO_PORT_P8
#define ADC_IN_PIN      GPIO_PIN1
#define ADC_IN_CHANNEL  ADC_INPUT_A9


#define MUX_SELECT_2_PORT GPIO_PORT_P2
#define MUX_SELECT_2_PIN GPIO_PIN7
#define MUX_SELECT_1_PORT GPIO_PORT_P8
#define MUX_SELECT_1_PIN GPIO_PIN0
#define MUX_SELECT_0_PORT GPIO_PORT_P5
#define MUX_SELECT_0_PIN GPIO_PIN1

#define LIGHT 0
#define TEMP1 1
#define TEMP2 2
#define SOIL1 3
#define SOIL2 4

void Init_GPIO(void);
void Init_Clock(void);
void Init_UART(void);
void Init_PWM(void);
void Init_ADC(void);
void LaunchpadInit(void);
void calculateADCResults(void);
void displayInfo(void);
void changeZone(void);
void startADC(void);
int16_t calculateSoil(int16_t average);
int16_t calculateTemp(int16_t average);




#endif /* GREENHOUSE_H_ */
