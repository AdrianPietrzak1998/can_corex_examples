#ifndef INC_LOGS_H_
#define INC_LOGS_H_

#include "main.h"
#include <stdint.h>

#ifndef LOG_UART_HANDLE
#define LOG_UART_HANDLE huart2
#endif

#ifndef LOG_UART_TIMEOUT
#define LOG_UART_TIMEOUT 100U
#endif

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE 128U
#endif

void LOG_Write(const char *text);
void LOG_WriteLine(const char *text);
void LOG_Printf(const char *format, ...);
void LOG_WriteBytes(const uint8_t *data, uint16_t size);

#endif
