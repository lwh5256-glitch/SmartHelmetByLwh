#ifndef __USART1_H
#define __USART1_H

#include "main.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

void MX_USART1_UART_Init(void);
void MX_USART3_UART_Init(void);

#endif /* __USART1_H */
