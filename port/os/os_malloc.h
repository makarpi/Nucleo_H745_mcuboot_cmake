/*
 * os_malloc.h
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#ifndef OS_MALLOC_H_
#define OS_MALLOC_H_
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *os_malloc(size_t size);
void os_free(void *ptr);

#ifdef __cplusplus
}
#endif


#endif /* OS_MALLOC_H_ */
