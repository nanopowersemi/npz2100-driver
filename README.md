# nPZ2100 Platform-Agnostic C Driver

**Device:** Nanopower Semiconductor nPZ2100 power-saving IC  
**Driver version:** 0.8  
**Language:** C99  
**Dependencies:** none — no OS, no HAL, no standard library beyond `<stdint.h>` and `<stdbool.h>`

---

## Package contents

```
NPZ2100/
├── Inc/
│   ├── npz2100_hal.h          ← HAL layer: error type, I2C callbacks, register primitives
│   ├── npz2100_mid.h          ← Mid layer: regmap, shadow, typed config helpers (public API)
│   ├── npz2100_regs_system.h  ← Register addresses and bitfield macros: system / global
│   ├── npz2100_regs_io.h      ← Register addresses and bitfield macros: I/O and power switches
│   ├── npz2100_regs_periph.h  ← Register addresses and bitfield macros: peripheral banks
│   └── npz2100_regs_adc_log.h ← Register addresses and bitfield macros: ADC, logging, counter
└── Src/
    ├── npz2100.c              ← HAL layer implementation
    └── npz2100_mid.c          ← Mid layer implementation
```

These two source files and their headers are the complete, self-contained driver.  
They contain **no platform-specific code** — no STM32, no Zephyr, no RTOS.  
A port file (one per platform) connects the driver to the target's I2C subsystem.

---

## Architecture

```
┌─────────────────────────────────────────────────┐
│                 Application                     │
│  (calls mid-layer API or port-layer wrappers)   │
└───────────────────┬─────────────────────────────┘
                    │
┌───────────────────▼─────────────────────────────┐
│              Mid layer  (npz2100_mid.h/.c)      │
│                                                 │
│  • Regmap apply / readback / validate           │
│  • Register shadow  (compile-time bypassable)   │
│  • Typed config helpers (sys, periph, ADC …)    │
│  • SRAM access, status read, control            │
└───────────────────┬─────────────────────────────┘
                    │  calls
┌───────────────────▼─────────────────────────────┐
│              HAL layer  (npz2100_hal.h/.c)      │
│                                                 │
│  • npz2100_reg_write / read                     │
│  • npz2100_reg_burst_write / read               │
│  • npz2100_reg_rmw                              │
│  • I2C callbacks (user-supplied, stored in      │
│    npz2100_hal_t)                               │
└───────────────────┬─────────────────────────────┘
                    │  user callbacks
┌───────────────────▼─────────────────────────────┐
│          Platform I2C  (user-supplied)           │
│  (STM32 HAL / Zephyr I2C / bare register …)     │
└─────────────────────────────────────────────────┘
```

---

## Porting to a new platform — 3 steps

### Step 1 — Implement the two I2C callbacks

```c
/* Write: buf[0] = register address, buf[1..len-1] = data.
 * Issues one I2C START/STOP for the whole buffer. */
static npz2100_err_t my_i2c_write(uint8_t        i2c_addr,
                                   const uint8_t *buf,
                                   size_t         len,
                                   void          *ctx)
{
    /* Call your platform's I2C write API here. */
    /* Return NPZ2100_OK on success, NPZ2100_ERR_IO on any error. */
}

/* Read: write register pointer, then read len bytes.
 * Issues: START addr+W, reg, rSTART addr+R, buf[0..len-1], STOP. */
static npz2100_err_t my_i2c_read(uint8_t  i2c_addr,
                                  uint8_t  reg,
                                  uint8_t *buf,
                                  size_t   len,
                                  void    *ctx)
{
    /* Call your platform's I2C read API here. */
    /* Return NPZ2100_OK on success, NPZ2100_ERR_IO on any error. */
}
```

### Step 2 — Populate the HAL struct

```c
npz2100_hal_t hal = {
    .write    = my_i2c_write,
    .read     = my_i2c_read,
    .i2c_addr = 0x3C,    /* 7-bit address — factory default */
    .ctx      = NULL,    /* passed through to every callback */
};
```

### Step 3 — Initialise the shadow and use the API

```c
npz2100_config_t shadow;
npz2100_config_init_defaults(&shadow);

/* Verify the device is present (ID register = 0x74). */
npz2100_probe_ll(&hal);

/* Boot sequence — every boot. */
uint8_t sta1, sta2, sta3;
npz2100_status_read(&hal, &sta1, &sta2, &sta3);  /* read + kick watchdog */
npz2100_map_readback(&hal, &shadow);             /* sync shadow from device */
npz2100_map_apply(&hal, &shadow, regmap, sizeof(regmap));  /* diff-apply */

/* Re-enter idle — host power will be cut. */
npz2100_enter_idle_ll(&hal);
```

---

## HAL layer API  (`npz2100_hal.h` / `npz2100.c`)

The HAL layer provides five register-level primitives.  
All functions take a `const npz2100_hal_t *hal` as first argument.

### Error type

```c
typedef enum {
    NPZ2100_OK          =  0,   /* Success                          */
    NPZ2100_ERR_IO      = -1,   /* I2C bus error (NAK, timeout …)  */
    NPZ2100_ERR_ARG     = -2,   /* Invalid argument                 */
    NPZ2100_ERR_DEV     = -3,   /* Device not found / ID mismatch  */
    NPZ2100_ERR_TIMEOUT = -4,   /* Operation timed out             */
    NPZ2100_ERR_STATE   = -5,   /* Device in wrong state           */
} npz2100_err_t;
```

### HAL descriptor

```c
typedef struct {
    npz2100_i2c_write_fn  write;     /* I2C write callback (user-supplied) */
    npz2100_i2c_read_fn   read;      /* I2C read callback  (user-supplied) */
    uint8_t               i2c_addr;  /* 7-bit I2C address (default: 0x3C)  */
    void                 *ctx;       /* Passed to every callback            */
} npz2100_hal_t;
```

### Register primitives

| Function | Description |
|---|---|
| `npz2100_reg_write(hal, addr, value)` | Write one register — one I2C transaction |
| `npz2100_reg_read(hal, addr, &value)` | Read one register |
| `npz2100_reg_burst_write(hal, start, buf, len)` | Write N consecutive registers |
| `npz2100_reg_burst_read(hal, start, buf, len)` | Read N consecutive registers |
| `npz2100_reg_rmw(hal, addr, mask, value)` | Read-modify-write one register |

---

## Mid layer API  (`npz2100_mid.h` / `npz2100_mid.c`)

The mid layer operates on a `npz2100_config_t` shadow struct (an in-memory
mirror of the device register state) and an `npz2100_hal_t` for I2C access.

### Shadow feature

The shadow tracks the last known device state.  `npz2100_map_apply()` diffs
the desired register map against the shadow and writes only changed registers,
minimising I2C bus traffic across repeated boot cycles.

**Compile-time bypass:** define `NPZ2100_SHADOW_ENABLE=0` to disable shadow
tracking entirely.  Every register in the regmap is then written unconditionally.

| `NPZ2100_SHADOW_ENABLE` | `map_apply` | `readback` | `shadow_flush` | `get_shadow` |
|---|---|---|---|---|
| `1` (default) | Diff + write changed only | Syncs shadow | Pushes shadow | Returns pointer |
| `0` | Writes all unconditionally | No-op | No-op | Returns NULL |

### Initialisation

| Function | Description |
|---|---|
| `npz2100_config_init_defaults(cfg)` | Seed shadow with power-on reset defaults |

### Regmap operations

| Function | Description |
|---|---|
| `npz2100_map_apply(hal, cfg, map, len)` | Apply byte-stream regmap — diff-only writes |
| `npz2100_map_readback(hal, cfg)` | Read all device registers into shadow |
| `npz2100_map_diff_count(cfg, map, len)` | Count registers that differ (no I2C) |
| `npz2100_map_validate(map, len)` | Validate regmap byte-stream framing (no I2C) |
| `npz2100_shadow_write_reg(hal, cfg, addr, val)` | Write one register and update shadow |

### Typed configuration helpers

These helpers modify fields of the `npz2100_config_t` shadow **in memory only** —
no I2C transaction is issued.  Call `npz2100_map_apply()` or `npz2100_periph_apply()`
afterwards to push the changes to the device.

| Function | Configures |
|---|---|
| `npz2100_sys_set(cfg, sys_cfg)` | Global system: wake sources, clock, time-out |
| `npz2100_timer_set(cfg, timer_cfg)` | Global time counter, alarm, watchdog |
| `npz2100_pa_set(cfg, pa_cfg)` | Power-aware mode source and behaviour |
| `npz2100_io_set(cfg, io_cfg)` | I/O: power switch modes, INT pin modes, pull-ups |
| `npz2100_periph_set(cfg, slot, periph_cfg)` | Peripheral slot 0–5: polling, thresholds, timing |
| `npz2100_adc_set(cfg, adc_cfg)` | ADC channels, thresholds, sampling rate |
| `npz2100_log_set(cfg, log_cfg)` | SRAM data logging: enable, rotation, start address |
| `npz2100_counter_set(cfg, cnt_cfg)` | Event counter source and trigger value |
| `npz2100_periph_apply(hal, cfg, slot)` | Push one peripheral bank's shadow to device |

### Device control and data read

| Function | Description |
|---|---|
| `npz2100_probe_ll(hal)` | Verify ID register = 0x74 |
| `npz2100_status_read(hal, &sta1, &sta2, &sta3)` | Burst-read STA1–STA3 + kick watchdog |
| `npz2100_periph_read_value_ll(hal, cfg, slot, &val)` | Read last sampled value for peripheral slot |
| `npz2100_adc_read(hal, ch, &val)` | Read one ADC channel value |
| `npz2100_sram_write_ll(hal, cfg, addr, data, len)` | Write to nPZ2100 SRAM (handles bank boundary) |
| `npz2100_sram_read_ll(hal, cfg, addr, data, len)` | Read from nPZ2100 SRAM |
| `npz2100_enter_idle_ll(hal)` | Write idle command — host power will be cut |
| `npz2100_soft_reset_ll(hal)` | Soft reset (config and SRAM preserved) |

---

## Register map byte-stream format

The `npz2100_map_apply()` and related functions consume a flat byte stream
produced by the Nanopower configuration tool.  The format is:

```
[length] [start_addr] [data_0] [data_1] ... [data_(length-2)]
```

where `length = 1 (start_addr) + N (data bytes)`.  Segments are concatenated
with no separator.  Example — write 3 bytes starting at 0x05:

```c
static const uint8_t regmap[] = {
    4, 0x05,       /* length=4: 1 addr + 3 data bytes */
    0x00,          /* 0x05 IOCFG1 */
    0xFF,          /* 0x06 IOCFG2 */
    0x00,          /* 0x07 IOCFG3 */
};
```

Call `npz2100_map_validate(regmap, sizeof(regmap))` to check framing before
applying.  Returns `NPZ2100_OK` if the stream is well-formed, `NPZ2100_ERR_ARG`
if any segment length byte is inconsistent.

---

## Register headers

The four register header files expose every register address and bitfield:

| Header | Covers |
|---|---|
| `npz2100_regs_system.h` | `IDLE_RST`, `ID`, `STA1–3`, `SYSCFG1–2`, `TOUT`, `GCT`, `WDOG`, `GTC_CFG`, `PA_CFG` |
| `npz2100_regs_io.h` | `IOCFG1–5`, `P_BANK`, `CFGP`, `IOP`, `MODP`, `PERP`, `NCMDP`, `ADDRP`, `RREGP`, `THROVP`, `THRUNP`, `TWTP`, `TCFGP`, `VALP` |
| `npz2100_regs_periph.h` | SPI mode, peripheral power modes, polling modes, data types |
| `npz2100_regs_adc_log.h` | `ADCCFG`, `THROVA/THRUNA[1–3]`, `VAL_ADC[1–3]`, `LOGCFG`, `LOGSADDR`, `LOGCADDR`, `CNTVAL`, `CNTCFG`, `CNTTRIG`, `SRAM_BANK` |

Bitfield macros follow the pattern:

```c
NPZ2100_STA1_RST_SRC_GET(reg)    /* extract RST_SRC field from STA1 value */
NPZ2100_STA1_FTOUT_MSK           /* bitmask for the FTOUT flag in STA1    */
NPZ2100_SYSCFG1_WUPMOD(val)      /* encode WUPMOD field for SYSCFG1       */
```

---

## Shadow bypass — compile-time

```c
/* Default — shadow enabled: */
npz2100_map_apply(&hal, &shadow, regmap, sizeof(regmap));
/*   → reads shadow, diffs, writes only changed registers */

/* Shadow disabled (NPZ2100_SHADOW_ENABLE=0): */
npz2100_map_apply(&hal, NULL, regmap, sizeof(regmap));
/*   → writes every register unconditionally, shadow ignored */
```

Define `NPZ2100_SHADOW_ENABLE=0` in your build system before including any
driver header:

- **STM32CubeIDE:** Project → Properties → C/C++ Build → Settings →
  MCU GCC Compiler → Preprocessor → Defined symbols: `NPZ2100_SHADOW_ENABLE=0`
- **NCS/Zephyr:** `prj.conf`: `CONFIG_NPZ2100_SHADOW=n`
- **Make/CMake:** `-DNPZ2100_SHADOW_ENABLE=0`

---

## Typical boot sequence

Every host boot is caused by the nPZ2100 re-asserting the host power switch.
The host must execute this sequence on every boot before application logic:

```c
/* 1. Read wake reason (also kicks watchdog). */
uint8_t sta1, sta2, sta3;
npz2100_status_read(&hal, &sta1, &sta2, &sta3);

uint8_t rst_src = NPZ2100_STA1_RST_SRC_GET(sta1);
bool    timeout = (sta1 & NPZ2100_STA1_FTOUT_MSK) != 0u;
uint8_t periph  = sta2 & 0x3Fu;   /* bit N-1 set if peripheral N triggered */
uint8_t nak     = sta3 & 0x3Fu;   /* bit N-1 set if peripheral N NAK'd     */

/* 2. Sync shadow from device (retains config while host is off). */
npz2100_map_readback(&hal, &shadow);

/* 3. Apply desired configuration — only changed registers written. */
npz2100_map_apply(&hal, &shadow, regmap, sizeof(regmap));

/* 4. On cold boot: write sensor init commands to SRAM. */
if (rst_src == NPZ2100_RST_SRC_POR) {
    npz2100_sram_write_ll(&hal, &shadow, 0x00, init_cmds, sizeof(init_cmds));
}

/* 5. Handle wake reason (application logic). */

/* 6. Optionally modify config for next cycle using typed helpers. */
npz2100_sys_set(&shadow, &new_sys_cfg);
npz2100_periph_apply(&hal, &shadow, 0);  /* push peripheral 0 to device */

/* 7. Re-enter idle — host power cut — does not return. */
npz2100_enter_idle_ll(&hal);
```

---

## Adding to a platform-specific port

The platform port file is the only file that includes platform headers.
It populates an `npz2100_hal_t` with callbacks and calls the mid-layer API.
Examples are provided in the separate STM32 and NCS packages:

| Port | File | Platform |
|---|---|---|
| STM32 HAL | `npz2100_stm32.c` / `npz2100_stm32.h` | STM32CubeL0 HAL |
| NCS/Zephyr | `npz2100_zephyr.c` | Zephyr I2C subsystem |

To port to a new platform, implement the two I2C callbacks (see **Porting**, above)
and optionally wrap the mid-layer functions in platform-style names and return types.

---

## Contact

Nanopower Semiconductor AS — www.nanopowersemi.com — info@nanopowersemi.com
