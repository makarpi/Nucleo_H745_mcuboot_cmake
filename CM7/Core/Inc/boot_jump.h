/*
 * boot_jump.h
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#ifndef INC_BOOT_JUMP_H_
#define INC_BOOT_JUMP_H_

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void boot_jump_to_image(uint32_t image_addr);

#ifdef __cplusplus
}
#endif

#endif /* INC_BOOT_JUMP_H_ */
