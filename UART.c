#include <stdio.h>
#include <stdint.h>
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_nvic.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "prcm.h"
#include "rom.h"
#include "prcm.h"
#include "utils.h"
#include "systick.h"
#include "rom_map.h"
#include "interrupt.h"
#include "gpio.h"
#include "utils.h"
#include "uart_if.h"
#include "uart.h"
#include "pin_mux_config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
extern void (* const g_pfnVectors[])(void);

typedef struct PinSetting {
    unsigned long port;
    unsigned int pin;
} PinSetting;

static void BoardInit(void);

static void BoardInit(void) {
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}

volatile int uart_intflag = 0;
char character;

#define MAX_SENTENCE_LENGTH 100

char gprmc_sentence[MAX_SENTENCE_LENGTH];
int gprmc_received = 0;
int rec_len = 0;

void UARTIntHandler(void)
{
    unsigned long ulStatus;
    ulStatus = UARTIntStatus(UARTA1_BASE, true);

    while(UARTCharsAvail(UARTA1_BASE))
    {
        char character = UARTCharGetNonBlocking(UARTA1_BASE);

        if (gprmc_received) {
            if (character == '\n' || character == '\r') {
                gprmc_sentence[rec_len] = '\0'; 
                gprmc_received = 0; 
                rec_len = 0; 
                if (strncmp(gprmc_sentence, "$GPRMC", 6) == 0) {
                    printf("Received GPRMC sentence: %s\n", gprmc_sentence);
                    char *token;
                    token = strtok(gprmc_sentence, ",");
                    int i = 0;
                    char latitude[15], longitude[15];
                    while (token != NULL) {
                        if (i == 3) { // Latitude
                            strcpy(latitude, token);
                        } else if (i == 5) { // Longitude
                            strcpy(longitude, token);
                        }
                        token = strtok(NULL, ",");
                        i++;
                    }
                    printf("Latitude: %s, Longitude: %s\n", latitude, longitude);
                }
            }
            else {
                if (rec_len < MAX_SENTENCE_LENGTH - 1) {
                    gprmc_sentence[rec_len++] = character;
                }
            }
        }
        else if (character == '$') {
            rec_len = 0; 
            gprmc_sentence[rec_len++] = character;
            gprmc_received = 1;
        }

    }

    UARTIntClear(UARTA1_BASE, ulStatus);
}



void UART_Communication(void)
{
    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(PRCM_UARTA1),
                            9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                       UART_CONFIG_PAR_NONE));
    MAP_UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_TX );
    UARTFIFODisable(UARTA1_BASE);
    UARTFIFOLevelSet(UARTA1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
    UARTIntClear(UARTA1_BASE, UART_INT_RX);
}


int main() {
    BoardInit();
    PinMuxConfig();
    UART_Communication();
    InitTerm();
    ClearTerm();
    Message("\t\t****************************************************\n\r");
    Message("\t\t\t\tGPS Data\n\r\n\r");
    Message("\t\t****************************************************\n\r");
    Message("\n\n\n\r");

    while(1){

    }
}