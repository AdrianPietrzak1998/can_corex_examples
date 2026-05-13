#include "logs.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void LOG_WriteBytes(const uint8_t *data, uint16_t size)
{
    if ((data == NULL) || (size == 0U))
    {
        return;
    }

    (void)HAL_UART_Transmit(&LOG_UART_HANDLE, (uint8_t *)data, size, LOG_UART_TIMEOUT);
}

void LOG_Write(const char *text)
{
    if (text == NULL)
    {
        return;
    }

    LOG_WriteBytes((const uint8_t *)text, (uint16_t)strlen(text));
}

void LOG_WriteLine(const char *text)
{
    LOG_Write(text);
    LOG_Write("\r\n");
}

void LOG_Printf(const char *format, ...)
{
    char buffer[LOG_BUFFER_SIZE];
    va_list args;
    int length;

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0)
    {
        return;
    }

    if ((uint32_t)length >= sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    LOG_WriteBytes((const uint8_t *)buffer, (uint16_t)length);
}
