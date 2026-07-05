/*
 * mcuboot_logging.h
 *
 *  Created on: Jul 2, 2026
 *      Author: mkarp
 */

#ifndef MCUBOOT_LOGGING_H_
#define MCUBOOT_LOGGING_H_
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void mcuboot_platform_log(const char *level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#ifndef MCUBOOT_LOG_LEVEL
#define MCUBOOT_LOG_LEVEL 3
#endif

#define MCUBOOT_LOG_MODULE_DECLARE(...)
#define MCUBOOT_LOG_MODULE_REGISTER(...)

#if MCUBOOT_LOG_LEVEL >= 1
#define MCUBOOT_LOG_ERR(...) \
    do { mcuboot_platform_log("ERR", __VA_ARGS__); } while (0)
#else
#define MCUBOOT_LOG_ERR(...) do { } while (0)
#endif

#if MCUBOOT_LOG_LEVEL >= 2
#define MCUBOOT_LOG_WRN(...) \
    do { mcuboot_platform_log("WRN", __VA_ARGS__); } while (0)
#else
#define MCUBOOT_LOG_WRN(...) do { } while (0)
#endif

#if MCUBOOT_LOG_LEVEL >= 3
#define MCUBOOT_LOG_INF(...) \
    do { mcuboot_platform_log("INF", __VA_ARGS__); } while (0)
#else
#define MCUBOOT_LOG_INF(...) do { } while (0)
#endif

#if MCUBOOT_LOG_LEVEL >= 4
#define MCUBOOT_LOG_DBG(...) \
    do { mcuboot_platform_log("DBG", __VA_ARGS__); } while (0)
#else
#define MCUBOOT_LOG_DBG(...) do { } while (0)
#endif
#endif /* MCUBOOT_LOGGING_H_ */
