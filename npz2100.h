/**
 * @file npz2100.h
 * @brief Umbrella header for the nPZ2100 C driver.
 *
 * Include this single header in application code to access every part of the
 * driver:
 *
 * @code
 * #include "npz2100.h"
 * @endcode
 *
 * Alternatively, include only the specific headers needed to keep compile
 * units lean on resource-constrained targets.
 *
 * File structure
 * --------------
 * | Header                     | Contents                                  |
 * |----------------------------|-------------------------------------------|
 * | npz2100_hal.h              | Error type, HAL callbacks, primitives     |
 * | npz2100_regs_system.h      | IDLE_RST, ID, STA1-3, SYSCFG, TOUT, GCT,  |
 * |                            | GCTALM, WDOG, GTC_CFG, PA_CFG             |
 * | npz2100_regs_io.h          | IOCFG1–IOCFG5                             |
 * | npz2100_regs_periph.h      | P_BANK, CFGP, IOP, MODP, PERP, NCMDP,     |
 * |                            | ADDRP, RREGP, THROVP, THRUNP, TWTP,       |
 * |                            | TCFGP, VALP                               |
 * | npz2100_regs_adc_log.h     | ADCCFG, THROVA/THRUNA/VAL_ADC[1-3],       |
 * |                            | LOGCFG, LOGSADDR, LOGCADDR, CNTVAL,       |
 * |                            | CNTCFG, CNTTRIG, SRAM_BANK                |
 * | npz2100_mid.h              | npz2100_regmap_t, npz2100_config_t,       |
 * |                            | map apply/readback/diff, typed helpers    |
 *
 * @version 0.7
 * @date    2026-05-06
 * @author  Nanopower Semiconductor AS
 */

#ifndef NPZ2100_H
#define NPZ2100_H

/* HAL interface and error type — always first */
#include "npz2100_hal.h"

/* Register definitions by functional block */
#include "npz2100_regs_system.h"
#include "npz2100_regs_io.h"
#include "npz2100_regs_periph.h"
#include "npz2100_regs_adc_log.h"

/* Mid-level API: register map, shadow struct, typed config helpers */
#include "npz2100_mid.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup npz2100_version Driver version
 * @{
 */
#define NPZ2100_DRIVER_VERSION_MAJOR   (0u)
#define NPZ2100_DRIVER_VERSION_MINOR   (7u)
#define NPZ2100_DRIVER_VERSION_PATCH   (0u)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NPZ2100_H */
