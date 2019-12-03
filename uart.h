#ifndef UART_H_
#define UART_H_
#include <msp430.h>
#include <stdint.h>
#include "driverlib/driverlib.h"

void displayConsole(uint8_t *message, uint8_t length);

#endif /* UART_H_ */
