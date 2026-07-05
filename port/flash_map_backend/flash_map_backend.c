/*
 * flash_map_backend.c
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */


#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"
#include "mcuboot_config/mcuboot_logging.h"

#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdint.h>

#define STM32_FLASH_BASE_ADDR        0x08000000UL
#define STM32_FLASH_BANK1_BASE_ADDR  0x08000000UL
#define STM32_FLASH_BANK2_BASE_ADDR  0x08100000UL
#define STM32_FLASH_TOTAL_SIZE       0x00200000UL
#define STM32_FLASH_BANK_SIZE        0x00100000UL
#define STM32_FLASH_SECTOR_SIZE      0x00020000UL
#define STM32_FLASH_WRITE_ALIGN      32U

/*
 * Docelowy layout flash STM32H745 2 MB:
 *
 * 0x08000000  MCUboot              128 KB
 * 0x08020000  slot0 primary        896 KB
 * 0x08100000  slot1 secondary      896 KB
 * 0x081E0000  scratch              128 KB
 *
 * fa_off jest offsetem względem 0x08000000.
 */

static int stm32_flash_addr_to_bank_sector(uint32_t abs_addr,
                                            uint32_t *bank,
                                            uint32_t *sector)
{
    if (bank == NULL || sector == NULL) {
        return -1;
    }

    if (abs_addr >= STM32_FLASH_BANK1_BASE_ADDR &&
        abs_addr <  STM32_FLASH_BANK2_BASE_ADDR) {

        *bank = FLASH_BANK_1;
        *sector = (abs_addr - STM32_FLASH_BANK1_BASE_ADDR) / STM32_FLASH_SECTOR_SIZE;
        return 0;
    }

    if (abs_addr >= STM32_FLASH_BANK2_BASE_ADDR &&
        abs_addr <  (STM32_FLASH_BASE_ADDR + STM32_FLASH_TOTAL_SIZE)) {

        *bank = FLASH_BANK_2;
        *sector = (abs_addr - STM32_FLASH_BANK2_BASE_ADDR) / STM32_FLASH_SECTOR_SIZE;
        return 0;
    }

    return -1;
}

static void stm32_flash_clear_flags(void)
{
#if defined(FLASH_FLAG_EOP_BANK1) && defined(FLASH_FLAG_EOP_BANK2)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP_BANK1 | FLASH_FLAG_EOP_BANK2);
#endif

#if defined(FLASH_FLAG_ALL_ERRORS_BANK1) && defined(FLASH_FLAG_ALL_ERRORS_BANK2)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS_BANK1 | FLASH_FLAG_ALL_ERRORS_BANK2);
#elif defined(FLASH_FLAG_ALL_ERRORS)
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
#endif
}

static int stm32_flash_is_erased(uint32_t abs_addr, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)abs_addr;

    for (uint32_t i = 0; i < len; i++) {
        if (p[i] != 0xFFU) {
            return 0;
        }
    }

    return 1;
}

static const struct flash_area flash_map[] = {
    {
        .fa_id = FLASH_AREA_BOOTLOADER,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00000000UL,
        .fa_size = 0x00020000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_0_PRIMARY,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00020000UL,
        .fa_size = 0x000E0000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_0_SECONDARY,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x00100000UL,
        .fa_size = 0x000E0000UL,
    },
    {
        .fa_id = FLASH_AREA_IMAGE_SCRATCH,
        .fa_device_id = 0,
        .pad16 = 0,
        .fa_off = 0x001E0000UL,
        .fa_size = 0x00020000UL,
    },
};

static uint32_t flash_abs_addr(const struct flash_area *fa, uint32_t off)
{
    return STM32_FLASH_BASE_ADDR + fa->fa_off + off;
}

int flash_area_open(uint8_t id, const struct flash_area **fa)
{
    if (fa == NULL) {
        return -1;
    }

    for (uint32_t i = 0; i < (sizeof(flash_map) / sizeof(flash_map[0])); i++) {
        if (flash_map[i].fa_id == id) {
            *fa = &flash_map[i];
            return 0;
        }
    }

    *fa = NULL;
    return -1;
}

void flash_area_close(const struct flash_area *fa)
{
    (void)fa;
}

int flash_area_read(const struct flash_area *fa, uint32_t off, void *dst, uint32_t len)
{
    if (fa == NULL || dst == NULL) {
        return -1;
    }

    if ((off + len) > fa->fa_size) {
        return -1;
    }

    memcpy(dst, (const void *)flash_abs_addr(fa, off), len);
    return 0;
}

int flash_area_write(const struct flash_area *fa, uint32_t off, const void *src, uint32_t len)
{
    if (fa == NULL || src == NULL) {
        return -1;
    }

    if (len == 0U) {
        return 0;
    }

    if ((off + len) > fa->fa_size) {
        return -1;
    }

    /*
     * STM32H7 programuje FLASHWORD = 32 bajty.
     * Wymuszamy zgodność z MCUBOOT_BOOT_MAX_ALIGN = 32.
     */
    if (((flash_abs_addr(fa, off) % STM32_FLASH_WRITE_ALIGN) != 0U) ||
        ((len % STM32_FLASH_WRITE_ALIGN) != 0U)) {
        return -1;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return -1;
    }

    const uint8_t *in = (const uint8_t *)src;

    for (uint32_t pos = 0U; pos < len; pos += STM32_FLASH_WRITE_ALIGN) {
        uint32_t abs_addr = flash_abs_addr(fa, off + pos);

        /*
         * Na STM32H7 ze względu na ECC najbezpieczniej programować
         * tylko świeżo skasowane 32-bajtowe jednostki.
         */
        if (!stm32_flash_is_erased(abs_addr, STM32_FLASH_WRITE_ALIGN)) {
            HAL_FLASH_Lock();
            return -1;
        }

        uint8_t flashword[STM32_FLASH_WRITE_ALIGN] __attribute__((aligned(32)));
        memcpy(flashword, &in[pos], STM32_FLASH_WRITE_ALIGN);

        stm32_flash_clear_flags();

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              abs_addr,
                              (uint32_t)flashword) != HAL_OK) {
            HAL_FLASH_Lock();
            return -1;
        }
    }

    if (HAL_FLASH_Lock() != HAL_OK) {
        return -1;
    }

    return 0;
}

int flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    MCUBOOT_LOG_DBG("flash_area_erase: fa_id=%d fa_off=0x%08lX fa_size=0x%08lX off=0x%08lX len=0x%08lX",
                    fa ? fa->fa_id : -1,
                    fa ? (unsigned long)fa->fa_off : 0,
                    fa ? (unsigned long)fa->fa_size : 0,
                    (unsigned long)off,
                    (unsigned long)len);

    if (fa == NULL) {
        MCUBOOT_LOG_ERR("flash_area_erase: fa NULL");
        return -1;
    }

    if (len == 0U) {
        return 0;
    }

    if ((off + len) > fa->fa_size) {
        MCUBOOT_LOG_ERR("flash_area_erase: range error off=0x%08lX len=0x%08lX fa_size=0x%08lX",
                        (unsigned long)off,
                        (unsigned long)len,
                        (unsigned long)fa->fa_size);
        return -1;
    }

    if ((off % STM32_FLASH_SECTOR_SIZE) != 0U ||
        (len % STM32_FLASH_SECTOR_SIZE) != 0U) {
        MCUBOOT_LOG_ERR("flash_area_erase: unaligned erase off=0x%08lX len=0x%08lX",
                        (unsigned long)off,
                        (unsigned long)len);
        return -1;
    }

    if (HAL_FLASH_Unlock() != HAL_OK) {
        MCUBOOT_LOG_ERR("flash_area_erase: HAL_FLASH_Unlock failed");
        return -1;
    }

    uint32_t erased = 0U;

    while (erased < len) {
        uint32_t abs_addr = flash_abs_addr(fa, off + erased);
        uint32_t bank = 0U;
        uint32_t sector = 0U;
        uint32_t sector_error = 0U;

        if (stm32_flash_addr_to_bank_sector(abs_addr, &bank, &sector) != 0) {
            MCUBOOT_LOG_ERR("flash_area_erase: addr_to_bank_sector failed addr=0x%08lX",
                            (unsigned long)abs_addr);
            HAL_FLASH_Lock();
            return -1;
        }

        MCUBOOT_LOG_INF("flash erase: addr=0x%08lX bank=%lu sector=%lu",
                        (unsigned long)abs_addr,
                        (unsigned long)bank,
                        (unsigned long)sector);

        FLASH_EraseInitTypeDef erase = {0};

        erase.TypeErase = FLASH_TYPEERASE_SECTORS;
        erase.Banks = bank;
        erase.Sector = sector;
        erase.NbSectors = 1U;

#if defined(FLASH_VOLTAGE_RANGE_3)
        erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
#endif

        stm32_flash_clear_flags();

        HAL_StatusTypeDef st = HAL_FLASHEx_Erase(&erase, &sector_error);

        if (st != HAL_OK) {
            MCUBOOT_LOG_ERR("flash erase failed: st=%ld sector_error=0x%08lX HAL_Error=0x%08lX",
                            (long)st,
                            (unsigned long)sector_error,
                            (unsigned long)HAL_FLASH_GetError());
            HAL_FLASH_Lock();
            return -1;
        }

        erased += STM32_FLASH_SECTOR_SIZE;
    }

    HAL_FLASH_Lock();
    return 0;
}

uint32_t flash_area_align(const struct flash_area *fa)
{
    (void)fa;

    /*
     * STM32H7 flash programuje się szerokimi jednostkami.
     * Dla MCUboot na H7 przyjmujemy align 32.
     */
    return 32U;
}

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
    (void)fa;
    return 0xFFU;
}

int flash_area_get_sectors(int fa_id, uint32_t *count, struct flash_sector *sectors)
{
    const struct flash_area *fa = NULL;

    if (count == NULL || sectors == NULL) {
        return -1;
    }

    if (flash_area_open((uint8_t)fa_id, &fa) != 0) {
        return -1;
    }

    if ((fa->fa_size % STM32_FLASH_SECTOR_SIZE) != 0U) {
        return -1;
    }

    uint32_t sector_count = fa->fa_size / STM32_FLASH_SECTOR_SIZE;

    if (*count < sector_count) {
        return -1;
    }

    for (uint32_t i = 0; i < sector_count; i++) {
        /*
         * Ważne:
         * fs_off ma być offsetem względem flash_area,
         * nie offsetem względem całego flasha.
         */
        sectors[i].fs_off = i * STM32_FLASH_SECTOR_SIZE;
        sectors[i].fs_size = STM32_FLASH_SECTOR_SIZE;
    }

    *count = sector_count;
    return 0;
}

int flash_area_get_id(const struct flash_area *fa)
{
    if (fa == NULL) {
        return -1;
    }

    return fa->fa_id;
}

int flash_area_get_device_id(const struct flash_area *fa)
{
    if (fa == NULL) {
        return -1;
    }

    return fa->fa_device_id;
}

uint32_t flash_area_get_off(const struct flash_area *fa)
{
    if (fa == NULL) {
        return 0U;
    }

    return fa->fa_off;
}

uint32_t flash_area_get_size(const struct flash_area *fa)
{
    if (fa == NULL) {
        return 0U;
    }

    return fa->fa_size;
}

int flash_area_to_sectors(int idx, int *cnt, struct flash_area *ret)
{
    const struct flash_area *fa = NULL;

    if (cnt == NULL || ret == NULL) {
        return -1;
    }

    if (*cnt < 0) {
        return -1;
    }

    if (flash_area_open((uint8_t)idx, &fa) != 0) {
        return -1;
    }

    if ((fa->fa_size % STM32_FLASH_SECTOR_SIZE) != 0U) {
        return -1;
    }

    uint32_t sector_count = fa->fa_size / STM32_FLASH_SECTOR_SIZE;

    if ((uint32_t)(*cnt) < sector_count) {
        return -1;
    }

    for (uint32_t i = 0; i < sector_count; i++) {
        ret[i].fa_id = fa->fa_id;
        ret[i].fa_device_id = fa->fa_device_id;
        ret[i].pad16 = 0;

        /*
         * Ważne:
         * Dla boot_sector_t offset sektora ma być względny względem area.
         * Scratch sektor 0 => fa_off = 0, nie 0x001E0000.
         */
        ret[i].fa_off = i * STM32_FLASH_SECTOR_SIZE;
        ret[i].fa_size = STM32_FLASH_SECTOR_SIZE;
    }

    *cnt = (int)sector_count;
    return 0;
}

int flash_area_get_sector(const struct flash_area *fa, uint32_t off, struct flash_sector *sector)
{
    if (fa == NULL || sector == NULL) {
        return -1;
    }

    if (off >= fa->fa_size) {
        return -1;
    }

    uint32_t sector_index = off / STM32_FLASH_SECTOR_SIZE;

    /*
     * Offset sektora względem flash_area.
     */
    sector->fs_off = sector_index * STM32_FLASH_SECTOR_SIZE;
    sector->fs_size = STM32_FLASH_SECTOR_SIZE;

    return 0;
}

uint32_t flash_sector_get_off(const struct flash_sector *sector)
{
    if (sector == NULL) {
        return 0U;
    }

    return sector->fs_off;
}

uint32_t flash_sector_get_size(const struct flash_sector *sector)
{
    if (sector == NULL) {
        return 0U;
    }

    return sector->fs_size;
}

int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
    if (image_index != 0) {
        return -1;
    }

    if (slot == 0) {
        return FLASH_AREA_IMAGE_0_PRIMARY;
    }

    if (slot == 1) {
        return FLASH_AREA_IMAGE_0_SECONDARY;
    }

    return -1;
}

int flash_area_id_to_multi_image_slot(int image_index, int area_id)
{
    if (image_index != 0) {
        return -1;
    }

    if (area_id == FLASH_AREA_IMAGE_0_PRIMARY) {
        return 0;
    }

    if (area_id == FLASH_AREA_IMAGE_0_SECONDARY) {
        return 1;
    }

    return -1;
}
