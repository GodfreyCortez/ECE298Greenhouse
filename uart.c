
#include "uart.h"

void displayConsole(uint8_t *message, uint8_t length) {
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 13);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 10);
    uint8_t i = 0;
    while(i < length) {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, message[i]);
        i++;
    }
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 13);
    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, 10);
}
