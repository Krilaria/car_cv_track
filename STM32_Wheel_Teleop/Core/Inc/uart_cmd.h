#ifndef __UART_CMD_H
#define __UART_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* USART1: PA9 = TX, PA10 = RX, 115200 8N1. */
void UartCmd_Init(void);

/* Call from the main loop: reads received bytes, runs finished command lines. */
void UartCmd_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* __UART_CMD_H */
