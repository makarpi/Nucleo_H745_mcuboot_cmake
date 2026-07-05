/*
 * os_malloc.c
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */


#include "os_malloc.h"
#include <stdlib.h>

void *os_malloc(size_t size)
{
    return malloc(size);
}

void os_free(void *ptr)
{
    free(ptr);
}
