/*
 * mcuboot_platform_logging.c
 *
 *  Created on: Jul 3, 2026
 *      Author: mkarp
 */


#include "stm32h7xx_hal.h"

#include <stdarg.h>
#include <stdio.h>

void mcuboot_platform_log(const char *level, const char *fmt, ...)
{
    va_list args;

    printf("[%08lu] %s: ", (unsigned long)HAL_GetTick(), level);

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\r\n");
}
