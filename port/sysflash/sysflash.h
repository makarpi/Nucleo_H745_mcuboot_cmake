/*
 * sysflash.h
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#ifndef SYSFLASH_H_
#define SYSFLASH_H_

/*
 * ID obszarów flash widziane przez MCUboot.
 *
 * Etap 1 używa tylko FLASH_AREA_IMAGE_0_PRIMARY.
 * Secondary i scratch zostawiamy już zdefiniowane,
 * bo później dojdzie update/swap/rollback.
 */

#define FLASH_AREA_BOOTLOADER              0
#define FLASH_AREA_IMAGE_0_PRIMARY         1
#define FLASH_AREA_IMAGE_0_SECONDARY       2
#define FLASH_AREA_IMAGE_SCRATCH           3

#define FLASH_AREA_IMAGE_PRIMARY(x)        FLASH_AREA_IMAGE_0_PRIMARY
#define FLASH_AREA_IMAGE_SECONDARY(x)      FLASH_AREA_IMAGE_0_SECONDARY

#endif /* SYSFLASH_H_ */
