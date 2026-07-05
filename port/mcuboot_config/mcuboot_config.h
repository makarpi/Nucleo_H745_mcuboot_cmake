/*
 * mcuboot_config.h
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#ifndef MCUBOOT_CONFIG_H_
#define MCUBOOT_CONFIG_H_

/*
 * Etap 1:
 * - jeden obraz
 * - podpis ECDSA P-256
 * - TinyCrypt
 * - tylko primary slot
 * - bez swapa / rollbacku
 */

#define MCUBOOT_SIGN_EC256
#define MCUBOOT_USE_TINYCRYPT

#define MCUBOOT_VALIDATE_PRIMARY_SLOT

#define MCUBOOT_IMAGE_NUMBER 1
#define MCUBOOT_PRIMARY_ONLY 1

#define MCUBOOT_BOOT_MAX_ALIGN 32
#define MCUBOOT_MAX_IMG_SECTORS 8

#define MCUBOOT_WATCHDOG_FEED() do { } while (0)

#define MCUBOOT_SWAP_USING_SCRATCH 1

/*
 * Logi MCUboot:
 * 0 = off
 * 1 = err
 * 2 = wrn
 * 3 = inf
 * 4 = dbg
 */
#define MCUBOOT_HAVE_LOGGING
#define MCUBOOT_LOG_LEVEL 4
/*
 * na produkcje LEVEL 2 zmienic - MKA
 */


#endif /* MCUBOOT_CONFIG_H_ */
