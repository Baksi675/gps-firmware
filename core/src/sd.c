/**
 * @file sd.c
 * @author github.com/Baksi675
 * @brief SD card driver implementation file.
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <stdbool.h>
#include <string.h>

#include "sd.h"
#include "common.h"
#include "err.h"
#include "init.h"
#include "io.h"
#include "log.h"
#include "modules.h"
#include "cmd.h"
#include "configuration.h"
#include "stm32f401re.h"
#include "stm32f401re_gpio.h"
#include "stm32f401re_rcc.h"
#include "stm32f401re_spi.h"
#include "arm_cortex_m4_systick.h"

/**
 * @brief Holds the R1, R3, and R7 response bytes returned by the SD card after a command.
 *
 * @details
 * Every SD command returns at least an R1 byte. Some commands additionally
 * return a 4-byte trailing payload: R3 (OCR content, from CMD58) or
 * R7 (interface condition, from CMD8). Unused fields are zeroed by
 * @ref sd_send_cmd.
 */
typedef struct {
    uint8_t r1;     /**< R1 response byte. Bit 0 = idle state, bits 1-6 = error flags. */
    uint8_t r3[4];  /**< R3 trailing bytes (OCR register content). Valid after CMD58. */
    uint8_t r7[4];  /**< R7 trailing bytes (interface condition echo). Valid after CMD8. */
} CMD_RESPONSE_ts;

/**
 * @brief Bit positions within the 32-bit OCR register.
 *
 * @details
 * Used to extract individual fields from the OCR value reconstructed
 * from the R3 response of CMD58. Voltage range bits 4–23 each represent
 * a 100 mV window; bit 30 is the Card Capacity Status (0 = SDSC, 1 = SDHC);
 * bit 31 is the power-up completion flag.
 */
typedef enum {
    OCR_1_7V_1_6V     = 4,  /**< Voltage window 1.6–1.7 V supported. */
    OCR_1_8V_1_7V,           /**< Voltage window 1.7–1.8 V supported. */
    OCR_1_9V_1_8V,           /**< Voltage window 1.8–1.9 V supported. */
    OCR_2_0V_1_9V,           /**< Voltage window 1.9–2.0 V supported. */
    OCR_2_1V_2_0V,           /**< Voltage window 2.0–2.1 V supported. */
    OCR_2_2V_2_1V,           /**< Voltage window 2.1–2.2 V supported. */
    OCR_2_3V_2_2V,           /**< Voltage window 2.2–2.3 V supported. */
    OCR_2_4V_2_3V,           /**< Voltage window 2.3–2.4 V supported. */
    OCR_2_5V_2_4V,           /**< Voltage window 2.4–2.5 V supported. */
    OCR_2_6V_2_5V,           /**< Voltage window 2.5–2.6 V supported. */
    OCR_2_7V_2_6V,           /**< Voltage window 2.6–2.7 V supported. */
    OCR_2_8V_2_7V,           /**< Voltage window 2.7–2.8 V supported. */
    OCR_2_9V_2_8V,           /**< Voltage window 2.8–2.9 V supported. */
    OCR_3_0V_2_9V,           /**< Voltage window 2.9–3.0 V supported. */
    OCR_3_1V_3_0V,           /**< Voltage window 3.0–3.1 V supported. */
    OCR_3_2V_3_1V,           /**< Voltage window 3.1–3.2 V supported. */
    OCR_3_3V_3_2V,           /**< Voltage window 3.2–3.3 V supported. */
    OCR_3_4V_3_3V,           /**< Voltage window 3.3–3.4 V supported. */
    OCR_3_5V_3_4V,           /**< Voltage window 3.4–3.5 V supported. */
    OCR_3_6V_3_5V,           /**< Voltage window 3.5–3.6 V supported. */
    OCR_CAPACITY_STATUS = 30, /**< Card Capacity Status: 0 = SDSC (byte addressing), 1 = SDHC (block addressing). */
    OCR_PWRUP_STATUS          /**< Power-up status: 0 = still powering up, 1 = power-up complete. */
} OCR_te;

/**
 * @brief Minimum allowed operating voltage of the SD card, decoded from the OCR register.
 */
typedef enum {
    SD_MIN_OPERATING_VOLTAGE_3_5V, /**< Minimum operating voltage: 3.5 V. */
    SD_MIN_OPERATING_VOLTAGE_3_4V, /**< Minimum operating voltage: 3.4 V. */
    SD_MIN_OPERATING_VOLTAGE_3_3V, /**< Minimum operating voltage: 3.3 V. */
    SD_MIN_OPERATING_VOLTAGE_3_2V, /**< Minimum operating voltage: 3.2 V. */
    SD_MIN_OPERATING_VOLTAGE_3_1V, /**< Minimum operating voltage: 3.1 V. */
    SD_MIN_OPERATING_VOLTAGE_3_0V, /**< Minimum operating voltage: 3.0 V. */
    SD_MIN_OPERATING_VOLTAGE_2_9V, /**< Minimum operating voltage: 2.9 V. */
    SD_MIN_OPERATING_VOLTAGE_2_8V, /**< Minimum operating voltage: 2.8 V. */
    SD_MIN_OPERATING_VOLTAGE_2_7V, /**< Minimum operating voltage: 2.7 V. */
    SD_MIN_OPERATING_VOLTAGE_2_6V, /**< Minimum operating voltage: 2.6 V. */
    SD_MIN_OPERATING_VOLTAGE_2_5V, /**< Minimum operating voltage: 2.5 V. */
    SD_MIN_OPERATING_VOLTAGE_2_4V, /**< Minimum operating voltage: 2.4 V. */
    SD_MIN_OPERATING_VOLTAGE_2_3V, /**< Minimum operating voltage: 2.3 V. */
    SD_MIN_OPERATING_VOLTAGE_2_2V, /**< Minimum operating voltage: 2.2 V. */
    SD_MIN_OPERATING_VOLTAGE_2_1V, /**< Minimum operating voltage: 2.1 V. */
    SD_MIN_OPERATING_VOLTAGE_2_0V, /**< Minimum operating voltage: 2.0 V. */
    SD_MIN_OPERATING_VOLTAGE_1_9V, /**< Minimum operating voltage: 1.9 V. */
    SD_MIN_OPERATING_VOLTAGE_1_8V, /**< Minimum operating voltage: 1.8 V. */
    SD_MIN_OPERATING_VOLTAGE_1_7V, /**< Minimum operating voltage: 1.7 V. */
    SD_MIN_OPERATING_VOLTAGE_1_6V  /**< Minimum operating voltage: 1.6 V. */
} SD_MIN_OPERATING_VOLTAGE_te;

/**
 * @brief Maximum allowed operating voltage of the SD card, decoded from the OCR register.
 */
typedef enum {
    SD_MAX_OPERATING_VOLTAGE_3_6V, /**< Maximum operating voltage: 3.6 V. */
    SD_MAX_OPERATING_VOLTAGE_3_5V, /**< Maximum operating voltage: 3.5 V. */
    SD_MAX_OPERATING_VOLTAGE_3_4V, /**< Maximum operating voltage: 3.4 V. */
    SD_MAX_OPERATING_VOLTAGE_3_3V, /**< Maximum operating voltage: 3.3 V. */
    SD_MAX_OPERATING_VOLTAGE_3_2V, /**< Maximum operating voltage: 3.2 V. */
    SD_MAX_OPERATING_VOLTAGE_3_1V, /**< Maximum operating voltage: 3.1 V. */
    SD_MAX_OPERATING_VOLTAGE_3_0V, /**< Maximum operating voltage: 3.0 V. */
    SD_MAX_OPERATING_VOLTAGE_2_9V, /**< Maximum operating voltage: 2.9 V. */
    SD_MAX_OPERATING_VOLTAGE_2_8V, /**< Maximum operating voltage: 2.8 V. */
    SD_MAX_OPERATING_VOLTAGE_2_7V, /**< Maximum operating voltage: 2.7 V. */
    SD_MAX_OPERATING_VOLTAGE_2_6V, /**< Maximum operating voltage: 2.6 V. */
    SD_MAX_OPERATING_VOLTAGE_2_5V, /**< Maximum operating voltage: 2.5 V. */
    SD_MAX_OPERATING_VOLTAGE_2_4V, /**< Maximum operating voltage: 2.4 V. */
    SD_MAX_OPERATING_VOLTAGE_2_3V, /**< Maximum operating voltage: 2.3 V. */
    SD_MAX_OPERATING_VOLTAGE_2_2V, /**< Maximum operating voltage: 2.2 V. */
    SD_MAX_OPERATING_VOLTAGE_2_1V, /**< Maximum operating voltage: 2.1 V. */
    SD_MAX_OPERATING_VOLTAGE_2_0V, /**< Maximum operating voltage: 2.0 V. */
    SD_MAX_OPERATING_VOLTAGE_1_9V, /**< Maximum operating voltage: 1.9 V. */
    SD_MAX_OPERATING_VOLTAGE_1_8V, /**< Maximum operating voltage: 1.8 V. */
    SD_MAX_OPERATING_VOLTAGE_1_7V  /**< Maximum operating voltage: 1.7 V. */
} SD_MAX_OPERATIING_VOLTAGE_te;

/**
 * @brief Addressing mode used by the SD card for read/write sector addresses.
 */
typedef enum {
    SD_ADDR_MODE_BYTE,  /**< Byte addressing (SDSC cards). Address = sector × block length. */
    SD_ADDR_MODE_BLOCK  /**< Block addressing (SDHC/SDXC cards). Address = sector number directly. */
} SD_ADDR_MODE_te;

/**
 * @brief Power-up completion status decoded from the OCR register.
 */
typedef enum {
    SD_PWRUP_STATUS_BUSY, /**< Card is still completing its power-up sequence. */
    SD_PWRUP_STATUS_READY /**< Card has completed power-up and is ready for commands. */
} SD_PWRUP_STATUS_te;

/**
 * @brief Card type decoded from the OCR Capacity Status bit.
 */
typedef enum {
    SDSC, /**< Standard Capacity SD card (byte addressing, ≤ 2 GB). */
    SDHC  /**< High Capacity SD card (block addressing, > 2 GB). */
} SD_TYPE_te;

/**
 * @brief Decoded contents of the SD card CSD register.
 *
 * @details
 * Populated by @ref decode_csd_v1 or @ref decode_csd_v2 and used to
 * extract capacity, block size, and block count, which are then stored
 * in the @ref sd_handle_s for use by read/write and FatFs ioctl calls.
 */
typedef struct {
    uint32_t capacity_bytes;    /**< Total card capacity in bytes. */
    uint32_t capacity_mb;       /**< Total card capacity in megabytes. */
    uint16_t block_len;         /**< Read/write block length in bytes. */
    uint32_t block_count;       /**< Total number of blocks on the card. */
    uint8_t  tran_speed;        /**< Maximum data transfer speed encoded value. */
    uint16_t ccc;               /**< Card command classes bitmask. */
    uint8_t  csd_structure;     /**< CSD structure version: 0 = v1.0, 1 = v2.0. */

    /** CSD v1.0 specific fields (SDSC cards). */
    struct {
        uint16_t c_size;        /**< Device size (12-bit). Used to compute block count. */
        uint8_t  c_size_mult;   /**< Device size multiplier (3-bit). */
        uint8_t  read_bl_len;   /**< Maximum read block length exponent. */
    } v1;

    /** CSD v2.0 specific fields (SDHC/SDXC cards). */
    struct {
        uint32_t c_size;        /**< Device size (22-bit). Capacity = (c_size + 1) × 512 KB. */
    } v2;

    uint8_t taac;               /**< Time-dependent part of data access time. */
    uint8_t nsac;               /**< Clock-dependent part of data access time (in clock cycles × 100). */
    uint8_t read_bl_partial;    /**< Partial blocks allowed for read (1 = yes). */
    uint8_t write_blk_misalign; /**< Write block misalignment allowed (1 = yes). */
    uint8_t read_blk_misalign;  /**< Read block misalignment allowed (1 = yes). */
    uint8_t dsr_imp;            /**< Configurable driver stage implemented (1 = yes). */
    uint8_t erase_blk_en;       /**< Erase single block enabled (1 = yes). */
    uint8_t sector_size;        /**< Erasable sector size in blocks minus 1. */
    uint8_t wp_grp_size;        /**< Write-protect group size in erasable sectors minus 1. */
    uint8_t wp_grp_enable;      /**< Write-protect group enable (1 = yes). */
    uint8_t r2w_factor;         /**< Write speed factor relative to read speed. */
    uint8_t write_bl_len;       /**< Maximum write block length exponent. */
    uint8_t write_bl_partial;   /**< Partial blocks allowed for write (1 = yes). */
    uint8_t file_format_grp;    /**< File format group (0 = hard disk / DOS FAT). */
    uint8_t copy;               /**< Copy flag (0 = original). */
    uint8_t perm_write_protect;/**< Permanent write protection (1 = yes). */
    uint8_t tmp_write_protect;  /**< Temporary write protection (1 = yes). */
    uint8_t file_format;        /**< File format of the card. */
    uint8_t wp_upc;             /**< Write protect until power cycle. */
    uint8_t crc;                /**< CRC7 checksum of the CSD register. */
} SD_CSD_INFO_ts;

/**
 * @brief Internal structure representing a single SD card handle instance.
 *
 * @details
 * Mirrors the @ref SD_CFG_ts fields and adds runtime state populated
 * during @ref sd_init_handle: card type, addressing mode, power-up status,
 * and capacity information decoded from the CSD register.
 */
struct sd_handle_s {
    char name[CONFIG_SD_MAX_NAME_LEN];                  /**< Human-readable name (null-terminated). */
    SPI_REGDEF_ts *spi_instance;                        /**< SPI peripheral used for this card. */
    GPIO_REGDEF_ts *sclk_gpio_port;                     /**< GPIO port of the SCLK pin. */
    GPIO_REGDEF_ts *cs_gpio_port;                       /**< GPIO port of the CS pin. */
    GPIO_REGDEF_ts *miso_gpio_port;                     /**< GPIO port of the MISO pin. */
    GPIO_REGDEF_ts *mosi_gpio_port;                     /**< GPIO port of the MOSI pin. */
    GPIO_PIN_te sclk_gpio_pin;                          /**< GPIO pin number of SCLK. */
    GPIO_PIN_te cs_gpio_pin;                            /**< GPIO pin number of CS. */
    GPIO_PIN_te miso_gpio_pin;                          /**< GPIO pin number of MISO. */
    GPIO_PIN_te mosi_gpio_pin;                          /**< GPIO pin number of MOSI. */
    GPIO_ALTERNATE_FUNCTION_te gpio_alternate_function; /**< Alternate function for SCLK/MISO/MOSI. */
    SD_PWRUP_STATUS_te pwrup_status;                    /**< Power-up completion status from OCR. */
    SD_ADDR_MODE_te addr_mode;                          /**< Addressing mode determined from OCR. */
    SD_TYPE_te type;                                    /**< Card type (SDSC or SDHC). */
    SD_MIN_OPERATING_VOLTAGE_te min_operating_voltage;  /**< Minimum operating voltage from OCR (not used). */
    SD_MAX_OPERATIING_VOLTAGE_te max_operating_voltage; /**< Maximum operating voltage from OCR (not used). */
    uint32_t block_len;                                 /**< Block (sector) size in bytes, from CSD. */
    uint32_t block_count;                               /**< Total number of blocks, from CSD. */
    uint32_t capacity_mb;                               /**< Total capacity in megabytes, from CSD. */
    bool initialized;                                   /**< True after successful @ref sd_init_handle. */
    bool in_use;                                        /**< True when this pool slot is occupied. */
};

/**
 * @brief Internal state of the SD card subsystem.
 *
 * @details
 * Holds the pool of SD card handles, the count of active handles,
 * and the subsystem lifecycle flags.
 */
struct internal_state_s {
    /** Pool of SD card handle instances. */
    SD_HANDLE_ts sds[CONFIG_SD_MAX_OBJECTS];

    /** Module identifier used for log messages. */
    MODULES_te subsys;

    /** Active log level for this subsystem. */
    LOG_LEVEL_te log_level;

    /** Number of currently active (in-use) SD card handles. */
    uint32_t sd_num;

    /** True after @ref sd_init_subsys has completed successfully. */
    bool initialized;

    /** True after @ref sd_start_subsys has been called. */
    bool started;
};

/** @brief Singleton instance of the SD card subsystem internal state. */
struct internal_state_s internal_state;

/* ---- Forward declarations for internal helpers ---- */
static ERR_te sd_go_idle_state(SD_HANDLE_ts *sd_handle);
static ERR_te sd_send_if_cond(SD_HANDLE_ts *sd_handle, bool *match_o, bool *no_resp_o);
static ERR_te sd_app_send_op_cond(SD_HANDLE_ts *sd_handle, uint32_t arg);
static ERR_te sd_read_ocr(SD_HANDLE_ts *sd_handle);
static ERR_te sd_send_op_cond(SD_HANDLE_ts *sd_handle);
static ERR_te sd_set_blocklen(SD_HANDLE_ts *sd_handle);
static ERR_te sd_read_csd(SD_HANDLE_ts *sd_handle);
static ERR_te decode_csd_v1(const uint8_t *csd_raw, SD_CSD_INFO_ts *csd_info_o);
static ERR_te decode_csd_v2(const uint8_t *csd_raw, SD_CSD_INFO_ts *csd_info_o);
static ERR_te sd_send_cmd(SPI_REGDEF_ts *spi_instance, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o);
static ERR_te sd_cease_comms(SD_HANDLE_ts *sd_handle, bool deinit);

/* ---- Forward declarations for command handlers ---- */
static ERR_te sd_cmd_list_handler(uint32_t argc, char **argv);
static ERR_te sd_cmd_info_handler(uint32_t argc, char **argv);

/**
 * @brief Table of CLI commands registered by the SD card subsystem.
 *
 * @details
 * Registered with the command subsystem via @ref sd_cmd_client_info
 * during @ref sd_init_subsys.
 */
CMD_INFO_ts sd_cmds[] = {
    {
        .name = "list",
        .help = "Lists active sd objects, usage: sd list",
        .handler = sd_cmd_list_handler
    },
    {
        .name = "info",
        .help = "Shows sd object info, usage: sd info <sd object>",
        .handler = sd_cmd_info_handler
    }
};

/**
 * @brief Registration descriptor passed to the command subsystem.
 *
 * @details
 * Bundles the command table, its size, the subsystem name prefix used
 * on the CLI, and a pointer to the runtime log-level variable.
 */
CMD_CLIENT_INFO_ts sd_cmd_client_info = {
    .cmds_ptr = sd_cmds,
    .num_cmds = sizeof(sd_cmds) / sizeof(sd_cmds[0]),
    .log_level_ptr = &internal_state.log_level,
    .name = "sd"
};

/**
 * @defgroup sd_public_apis SD Card Public APIs
 * @{
 */

/** @brief Initializes the SD card subsystem. @see sd_init_subsys */
ERR_te sd_init_subsys(void) {
    ERR_te err;

    if(internal_state.initialized) {
        return ERR_MODULE_ALREADY_INITIALIZED;
    }

    internal_state = (struct internal_state_s){ 0 };

    internal_state.log_level = LOG_LEVEL_INFO;
    internal_state.subsys = MODULES_SD;
    internal_state.initialized = true;
    internal_state.started = false;

    init_log();
    init_systick();

    err = cmd_register(&sd_cmd_client_info);
    if(err != ERR_OK) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_init_subsys: cmd_register error"
        );
        return err;
    }
    LOG_INFO(
        internal_state.subsys,
        internal_state.log_level,
        "sd_init_subsys: subsys initialized"
    );

    return ERR_OK;
}

/** @brief Deinitializes the SD card subsystem. @see sd_deinit_subsys */
ERR_te sd_deinit_subsys(void) {
    if(internal_state.initialized) {
        internal_state = (struct internal_state_s){ 0 };

        cmd_deregister(&sd_cmd_client_info);
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_deinit_subsys: subsys is not initialized"
        );

        return ERR_DEINITIALIZATION_FAILURE;
    }
    LOG_INFO(
        internal_state.subsys,
        internal_state.log_level,
        "sd_deinit_subsys: subsys deinitialized"
    );

    return ERR_OK;
}

/** @brief Starts the SD card subsystem. @see sd_start_subsys */
ERR_te sd_start_subsys(void) {
    if(internal_state.initialized && !internal_state.started) {
        internal_state.started = true;

        LOG_INFO(
            internal_state.subsys,
            internal_state.log_level,
            "sd_start_subsys: subsys started"
        );
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_start_subsys: subsys not initialized or already started"
        );

        return ERR_UNKNOWN;
    }

    return ERR_OK;
}

/** @brief Stops the SD card subsystem. @see sd_stop_subsys */
ERR_te sd_stop_subsys(void) {
    if(internal_state.initialized && internal_state.started) {
        internal_state.started = false;

        LOG_INFO(
            internal_state.subsys,
            internal_state.log_level,
            "sd_stop_subsys: subsys stopped"
        );
    }
    else {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_stop_subsys: subsys not initialized or already already stopped"
        );

        return ERR_UNKNOWN;
    }

    return ERR_OK;
}

/** @brief Initializes an SD card handle and performs the full SPI-mode power-up sequence. @see sd_init_handle */
ERR_te sd_init_handle(SD_CFG_ts *sd_cfg, SD_HANDLE_ts **sd_handle_o) {
    ERR_te err;
    uint8_t free_index;

    if(!internal_state.initialized) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_init_handle: subsys not initialized"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    if(internal_state.sd_num == CONFIG_SD_MAX_OBJECTS) {
        LOG_CRITICAL(
            internal_state.subsys,
            internal_state.log_level,
            "sd_init_handle: subsystem out of memory space"
        );

        return ERR_NOT_ENOUGH_SPACE;
    }

    if(!sd_cfg) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_init_handle: invalid argument"
        );
        return ERR_INVALID_ARGUMENT;
    }

    bool found = false;

    for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
        if(!internal_state.sds[i].in_use) {
            free_index = i;
            found = true;
            break;
        }
    }

    if(!found)
        return ERR_NOT_ENOUGH_SPACE;

    GPIO_CFG_ts sd_sclk = { 0 };
    sd_sclk.port = sd_cfg->sclk_gpio_port;
    sd_sclk.pin = sd_cfg->sclk_gpio_pin;
    sd_sclk.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    sd_sclk.alternate_function = sd_cfg->gpio_alternate_function;
    gpio_init(&sd_sclk);

    GPIO_CFG_ts sd_cs = { 0 };
    sd_cs.port = sd_cfg->cs_gpio_port;
    sd_cs.pin = sd_cfg->cs_gpio_pin;
    sd_cs.mode = GPIO_MODE_OUTPUT;
    gpio_init(&sd_cs);

    GPIO_CFG_ts sd_mosi = { 0 };
    sd_mosi.port = sd_cfg->mosi_gpio_port;
    sd_mosi.pin = sd_cfg->mosi_gpio_pin;
    sd_mosi.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    sd_mosi.alternate_function = sd_cfg->gpio_alternate_function;
    gpio_init(&sd_mosi);

    GPIO_CFG_ts sd_miso = { 0 };
    sd_miso.port = sd_cfg->miso_gpio_port;
    sd_miso.pin = sd_cfg->miso_gpio_pin;
    sd_miso.mode = GPIO_MODE_ALTERNATE_FUNCTION;
    sd_miso.alternate_function = sd_cfg->gpio_alternate_function;
    gpio_init(&sd_miso);

    // Configure SPI
    SPI_CFG_ts sd_spi = { 0 };
    sd_spi.instance = sd_cfg->spi_instance;
    sd_spi.bit_first = SPI_BIT_FIRST_MSB;
    sd_spi.clock_phase = SPI_CLOCK_PHASE_FIRST_TRANSITION;
    sd_spi.clock_polarity = SPI_CLOCK_POLARITY_0_IDLE;
    sd_spi.data_frame_format = SPI_DATA_FRAME_FORMATE_8_BIT;
    sd_spi.slave_select_mode = SPI_SLAVE_SELECT_MODE_SW;
    sd_spi.mode = SPI_MODE_MASTER;

    IO_CFG_ts io_cs_conf = { 0 };
    IO_HANDLE_ts *io_cs_handle = { 0 };
    io_cs_conf.gpio_handle = &sd_miso;
    str_cpy(io_cs_conf.name, "sd_cs", get_str_len("sd_cs") + 1);
    io_init_subsys();
    io_init_handle(&io_cs_conf, &io_cs_handle);
    io_start_subsys();

    // Set SPI speed between 100 and 400 KHz for initialization, then initialize
    uint32_t spi_pclk = 0;

    if(sd_cfg->spi_instance == SPI1 || sd_cfg->spi_instance == SPI4) {
        spi_pclk = rcc_get_apb2_clk();
    }
    else if(sd_cfg->spi_instance == SPI2 || sd_cfg->spi_instance == SPI3) {
        spi_pclk = rcc_get_apb1_clk();
    }

    uint32_t spi_pclk_cpy = spi_pclk;
    uint32_t div_factor = 2;
    while(div_factor <= 256) {
        spi_pclk = spi_pclk / div_factor;

        if(spi_pclk >= 100000 && spi_pclk <= 400000) {
            break;
        }

        spi_pclk = spi_pclk_cpy;
        div_factor *= 2;
    }

    switch (div_factor) {
        case 2:   sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_2;   break;
        case 4:   sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_4;   break;
        case 8:   sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_8;   break;
        case 16:  sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_16;  break;
        case 32:  sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_32;  break;
        case 64:  sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_64;  break;
        case 128: sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_128; break;
        case 256: sd_spi.master_sclk_speed = SPI_MASTER_SCLK_SPEED_PCLK_DIV_256; break;
    }

    spi_init(&sd_spi);

    str_cpy(
        internal_state.sds[free_index].name,
        sd_cfg->name,
        get_str_len(sd_cfg->name) + 1
    );

    internal_state.sds[free_index].spi_instance = sd_cfg->spi_instance;
    internal_state.sds[free_index].sclk_gpio_port = sd_cfg->sclk_gpio_port;
    internal_state.sds[free_index].cs_gpio_port = sd_cfg->cs_gpio_port;
    internal_state.sds[free_index].sclk_gpio_pin = sd_cfg->sclk_gpio_pin;
    internal_state.sds[free_index].miso_gpio_port = sd_cfg->miso_gpio_port;
    internal_state.sds[free_index].mosi_gpio_port = sd_cfg->mosi_gpio_port;
    internal_state.sds[free_index].cs_gpio_pin = sd_cfg->cs_gpio_pin;
    internal_state.sds[free_index].sclk_gpio_pin = sd_cfg->sclk_gpio_pin;
    internal_state.sds[free_index].miso_gpio_pin = sd_cfg->miso_gpio_pin;
    internal_state.sds[free_index].mosi_gpio_pin = sd_cfg->mosi_gpio_pin;
    internal_state.sds[free_index].gpio_alternate_function = sd_cfg->gpio_alternate_function;
    internal_state.sds[free_index].in_use = true;

    *sd_handle_o = &internal_state.sds[free_index];

    internal_state.sd_num++;

    // Wait for >= 1 ms after supply reached at least 2.2V
    DELAY(CONFIG_SD_HW_INIT_WAIT_TIME);

    // Enable SPI communication
    spi_set_comm(sd_cfg->spi_instance, ENABLE);

    // 1. Start initialization — set CS high and send at least 74 dummy clocks
    gpio_write(sd_cfg->cs_gpio_port, sd_cfg->cs_gpio_pin, HIGH);

    uint8_t dummy_tx = 0xFF;
    for(uint8_t i = 0; i < 10; i++) {
        spi_send(sd_cfg->spi_instance, &dummy_tx, 1);
    }

    gpio_write(sd_cfg->cs_gpio_port, sd_cfg->cs_gpio_pin, LOW);

    // 2. Enter idle state
    err = sd_go_idle_state(&internal_state.sds[free_index]);
    if(err != ERR_OK) {
        LOG_CRITICAL(internal_state.subsys,
            internal_state.log_level,
            "sd_init_handle: failed to enter idle state, deinitializing handle"
        );

        sd_cease_comms(&internal_state.sds[free_index], true);

        return ERR_UNSUCCESFUL_ACTION;
    }

    // 3. Detect card type and version
    bool match = false;
    bool no_response = false;

    err = sd_send_if_cond(&internal_state.sds[free_index], &match, &no_response);
    if(err != ERR_OK) {
        LOG_CRITICAL(internal_state.subsys,
            internal_state.log_level,
            "sd_init_handle: failed to get SD card type, deinitializing handle"
        );

        sd_cease_comms(&internal_state.sds[free_index], true);

        return ERR_UNSUCCESFUL_ACTION;
    }

    // 3.1 SD Ver.2 — determine byte or block addressing from OCR
    if(match) {
        err = sd_app_send_op_cond(&internal_state.sds[free_index], 0x40000000);
        if(err != ERR_OK) {
            LOG_CRITICAL(internal_state.subsys,
                internal_state.log_level,
                "sd_init_handle: failed to initiate initialization of SD Ver.2, deinitializing handle"
            );

            sd_cease_comms(&internal_state.sds[free_index], true);

            return ERR_UNSUCCESFUL_ACTION;
        }

        err = sd_read_ocr(&internal_state.sds[free_index]);
        if(err != ERR_OK) {
            LOG_CRITICAL(internal_state.subsys,
                internal_state.log_level,
                "sd_init_handle: failed to obtain SD Ver 2. information, deinitializing handle"
            );

            return ERR_UNSUCCESFUL_ACTION;
        }
    }
    // 3.2 SD Ver.1 or MMC Ver.3
    else if(no_response) {
        err = sd_app_send_op_cond(&internal_state.sds[free_index], 0);
        // SD Ver.1
        if(err == ERR_OK) {
            internal_state.sds[free_index].addr_mode = SD_ADDR_MODE_BYTE;
        }
        // MMC Ver.3 or unknown
        else if(err == ERR_TIMEOUT) {
            err = sd_send_op_cond(&internal_state.sds[free_index]);
            // Unknown card
            if(err != ERR_OK) {
                LOG_CRITICAL(internal_state.subsys,
                    internal_state.log_level,
                    "sd_init_handle: unknown card, deinitializing handle"
                );

                sd_cease_comms(&internal_state.sds[free_index], true);

                return ERR_UNSUCCESFUL_ACTION;
            }
            // MMC Ver.3
            else {
                internal_state.sds[free_index].addr_mode = SD_ADDR_MODE_BYTE;
            }
        }
        // Unknown card
        else {
            LOG_CRITICAL(internal_state.subsys,
                internal_state.log_level,
                "sd_init_handle: unknown card, deinitializing handle"
            );

            sd_cease_comms(&internal_state.sds[free_index], true);

            return ERR_UNSUCCESFUL_ACTION;
        }
    }

    if(internal_state.sds[free_index].addr_mode == SD_ADDR_MODE_BYTE) {
        err = sd_set_blocklen(&internal_state.sds[free_index]);
        if(err != ERR_OK) {
            LOG_CRITICAL(internal_state.subsys,
                internal_state.log_level,
                "sd_init_handle: failed to initialize, deinitializing handle"
            );

            sd_cease_comms(&internal_state.sds[free_index], true);

            return ERR_UNSUCCESFUL_ACTION;
        }
    }

    internal_state.sds[free_index].initialized = true;

    LOG_INFO(
        internal_state.subsys,
        internal_state.log_level,
        "sd_init_handle: sd handle %s initialized",
        internal_state.sds[free_index].name
    );

    // Read CSD to populate capacity, block size, and block count
    sd_read_csd(&internal_state.sds[free_index]);

    sd_cease_comms(&internal_state.sds[free_index], false);

    // Ramp SPI to maximum speed after initialization
    spi_set_pclk_div(internal_state.sds[free_index].spi_instance, SPI_MASTER_SCLK_SPEED_PCLK_DIV_2);

    return ERR_OK;
}

/** @brief Deinitializes an SD card handle and resets the SPI peripheral. @see sd_deinit_handle */
ERR_te sd_deinit_handle(SD_HANDLE_ts *sd_handle) {
    if(!sd_handle->initialized) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_deinit_handle: handle not initialized"
        );
        return ERR_ILLEGAL_ACTION;
    }

    if(internal_state.sd_num == 0) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_deinit_handle: no such handle to deinitialize"
        );

        return ERR_UNINITIALZIED_OBJECT;
    }

    for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
        if(&internal_state.sds[i] == sd_handle) {
            uint8_t name_len = get_str_len(internal_state.sds[i].name) + 1;
            char name[name_len];
            str_cpy(name, internal_state.sds[i].name, name_len);

            spi_deinit(internal_state.sds[i].spi_instance);

            internal_state.sds[i] = (SD_HANDLE_ts){ 0 };

            internal_state.sd_num--;

            LOG_INFO(
                internal_state.subsys,
                internal_state.log_level,
                "sd_deinit_handle: sd handle %s deinitialized",
                name
            );

            break;
        }

        if(i == CONFIG_SD_MAX_OBJECTS - 1) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_deinit_handle: no such handle to deinitialize"
            );

            return ERR_UNINITIALZIED_OBJECT;
        }
    }

    return ERR_OK;
}

/** @brief Retrieves the initialization state of an SD card handle. @see sd_get_handle_init */
ERR_te sd_get_handle_init(SD_HANDLE_ts *sd_handle, bool *handle_init_o) {
    *handle_init_o = sd_handle->initialized;

    return ERR_OK;
}

/** @brief Retrieves the total sector (block) count of the SD card. @see sd_get_sector_count */
ERR_te sd_get_sector_count(SD_HANDLE_ts *sd_handle, uint32_t *sector_count_o) {
    *sector_count_o = sd_handle->block_count;

    return ERR_OK;
}

/** @brief Retrieves the sector (block) size of the SD card in bytes. @see sd_get_sector_size */
ERR_te sd_get_sector_size(SD_HANDLE_ts *sd_handle, uint32_t *sector_size_o) {
    *sector_size_o = sd_handle->block_len;

    return ERR_OK;
}

/** @brief Reads one or more sectors from the SD card. @see sd_read */
ERR_te sd_read(SD_HANDLE_ts *sd_handle, uint8_t *read_buf, uint32_t start_sector, uint32_t num_sectors) {
    ERR_te err;
    uint32_t start_time = 0;
    uint32_t elapsed_time = 0;
    CMD_RESPONSE_ts cmd_response = { 0 };

    if(!internal_state.initialized || !internal_state.started || !sd_handle->initialized) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_read: subsys or handle not initialized or started"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    if(sd_handle == NULL || num_sectors == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    spi_set_comm(sd_handle->spi_instance, ENABLE);
    gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, LOW);

    if(sd_handle->addr_mode == SD_ADDR_MODE_BYTE) {
        start_sector = start_sector * sd_handle->block_len;
    }

    if(num_sectors == 1) {
        err = sd_send_cmd(sd_handle->spi_instance, 17, start_sector, false, &cmd_response);
        if(err != ERR_OK) {
            sd_cease_comms(sd_handle, true);

            return err;
        }

        if(cmd_response.r1 != 0x0) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }

        uint8_t token = 0xFF;
        start_time = systick_get_ms();

        do {
            elapsed_time = systick_get_ms() - start_time;
            spi_receive(sd_handle->spi_instance, &token, 1);
        } while(token == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

        if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
            sd_cease_comms(sd_handle, true);

            return ERR_TIMEOUT;
        }

        if(token != 0xFE) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }

        spi_receive(sd_handle->spi_instance, read_buf, sd_handle->block_len);

        uint8_t crc[2];
        spi_receive(sd_handle->spi_instance, crc, 2);
    }
    else {
        err = sd_send_cmd(sd_handle->spi_instance, 18, start_sector, false, &cmd_response);
        if(err != ERR_OK) {
            sd_cease_comms(sd_handle, true);

            return err;
        }

        if(cmd_response.r1 != 0x0) {
            sd_cease_comms(sd_handle, true);
            return ERR_UNKNOWN;
        }

        for(uint32_t i = 0; i < num_sectors; i++) {
            uint8_t token = 0xFF;
            start_time = systick_get_ms();

            do {
                elapsed_time = systick_get_ms() - start_time;
                spi_receive(sd_handle->spi_instance, &token, 1);
            } while(token == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

            if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
                sd_cease_comms(sd_handle, true);

                return ERR_TIMEOUT;
            }

            if(token != 0xFE) {
                sd_cease_comms(sd_handle, true);

                return ERR_UNKNOWN;
            }

            spi_receive(sd_handle->spi_instance, read_buf + (i * sd_handle->block_len), sd_handle->block_len);

            uint8_t crc[2];
            spi_receive(sd_handle->spi_instance, crc, 2);
        }
        uint8_t retry = 0;
        do {
            err = sd_send_cmd(sd_handle->spi_instance, 12, 0, false, &cmd_response);
            if(err != ERR_OK) {
                sd_cease_comms(sd_handle, true);

                return err;
            }
        } while(cmd_response.r1 != 0x0 && retry++ < CONFIG_SD_INVALID_RESP_RETRY_NUM);

        if(cmd_response.r1 != 0x00) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }
    }

    sd_cease_comms(sd_handle, false);

    return ERR_OK;
}

/** @brief Writes one or more sectors to the SD card. @see sd_write */
ERR_te sd_write(SD_HANDLE_ts *sd_handle, uint8_t *write_buf, uint32_t start_sector, uint32_t num_sectors) {
    ERR_te err;
    uint32_t start_time = 0;
    uint32_t elapsed_time = 0;

    if(!internal_state.initialized || !internal_state.started) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_write: subsys not initialized or started"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    if(sd_handle == NULL || num_sectors == 0) {
        return ERR_INVALID_ARGUMENT;
    }

    spi_set_comm(sd_handle->spi_instance, ENABLE);
    gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, LOW);

    if(sd_handle->addr_mode == SD_ADDR_MODE_BYTE) {
        start_sector = start_sector * sd_handle->block_len;
    }

    CMD_RESPONSE_ts cmd_response = { 0 };

    if(num_sectors == 1) {
        err = sd_send_cmd(sd_handle->spi_instance, 24, start_sector, false, &cmd_response);
        if(err != ERR_OK) {
            sd_cease_comms(sd_handle, true);

            return err;
        }

        if(cmd_response.r1 != 0x0) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }

        uint8_t token = 0xFE;
        spi_send(sd_handle->spi_instance, &token, 1);
        spi_send(sd_handle->spi_instance, write_buf, sd_handle->block_len);

        uint8_t dummy_crc[2] = { 0xFF, 0xFF };
        spi_send(sd_handle->spi_instance, dummy_crc, 2);

        uint8_t data_resp = 0xFF;
        start_time = systick_get_ms();

        do {
            elapsed_time = systick_get_ms() - start_time;
            spi_receive(sd_handle->spi_instance, &data_resp, 1);
        } while(data_resp == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

        if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
            sd_cease_comms(sd_handle, true);

            return ERR_TIMEOUT;
        }

        if((data_resp & 0b11111) != 0b00101) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }

        // Wait until card releases MISO (busy signal)
        uint8_t busy = 0x00;
        start_time = systick_get_ms();
        do {
            spi_receive(sd_handle->spi_instance, &busy, 1);
            elapsed_time = systick_get_ms() - start_time;
        } while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

        if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
            sd_cease_comms(sd_handle, true);

            return ERR_TIMEOUT;
        }
    }
    else {
        err = sd_send_cmd(sd_handle->spi_instance, 25, start_sector, false, &cmd_response);
        if(err != ERR_OK) {
            sd_cease_comms(sd_handle, true);

            return err;
        }

        if(cmd_response.r1 != 0x0) {
            sd_cease_comms(sd_handle, true);

            return ERR_UNKNOWN;
        }

        for(uint32_t i = 0; i < num_sectors; i++) {
            uint8_t token = 0xFC;
            spi_send(sd_handle->spi_instance, &token, 1);
            spi_send(sd_handle->spi_instance, write_buf + (i * sd_handle->block_len), sd_handle->block_len);

            uint8_t dummy_crc[2] = { 0xFF, 0xFF };
            spi_send(sd_handle->spi_instance, dummy_crc, 2);

            uint8_t data_resp = 0xFF;
            start_time = systick_get_ms();

            do {
                elapsed_time = systick_get_ms() - start_time;
                spi_receive(sd_handle->spi_instance, &data_resp, 1);
            } while(data_resp == 0xFF && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

            if(elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT) {
                sd_cease_comms(sd_handle, true);

                return ERR_TIMEOUT;
            }

            if((data_resp & 0b11111) != 0b00101) {
                sd_cease_comms(sd_handle, true);

                return ERR_UNKNOWN;
            }

            uint8_t busy = 0x00;
            start_time = systick_get_ms();
            do {
                spi_receive(sd_handle->spi_instance, &busy, 1);
                elapsed_time = systick_get_ms() - start_time;
            } while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

            if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
                sd_cease_comms(sd_handle, true);

                return ERR_TIMEOUT;
            }
        }

        // Send stop transmission token
        uint8_t stop_tran_token = 0xFD;
        spi_send(sd_handle->spi_instance, &stop_tran_token, 1);

        uint8_t busy = 0x00;
        start_time = systick_get_ms();
        do {
            spi_receive(sd_handle->spi_instance, &busy, 1);
            elapsed_time = systick_get_ms() - start_time;
        } while(busy == 0x00 && elapsed_time <= CONFIG_SD_BUSY_TIMOUT);

        if(elapsed_time > CONFIG_SD_BUSY_TIMOUT) {
            sd_cease_comms(sd_handle, true);

            return ERR_TIMEOUT;
        }
    }

    sd_cease_comms(sd_handle, false);

    return ERR_OK;
}

/** @} */

/**
 * @defgroup sd_internal_helpers SD Internal Helpers
 * @{
 */

/**
 * @brief Transmits a single SPI-mode SD command and receives the response.
 *
 * @details
 * Builds the 6-byte command frame (start bit | index, 4-byte argument, CRC),
 * transmits it over SPI, and receives the R1 response byte. For application-
 * specific commands (ACMD), CMD55 is sent first automatically.
 *
 * Trailing response bytes are handled per command:
 * - R1 only: CMD0, CMD1, CMD9, CMD10, CMD16, CMD17, CMD18, CMD23, CMD24, CMD25, CMD41, CMD55
 * - R3 (4 extra bytes, OCR): CMD58
 * - R7 (4 extra bytes, interface condition): CMD8
 * - R1B (busy polling): CMD12 — discards stuff byte, then polls until card releases MISO
 *
 * CRC is required for CMD0 (0x95) and CMD8 (0x87); dummy CRC (0x00) is used
 * for all other commands, as CRC checking is disabled after initialization.
 *
 * @param[in]  spi_instance   Pointer to the SPI peripheral to use.
 * @param[in]  index          Command index (0–63, without the 0x40 start bit).
 * @param[in]  arg            32-bit command argument.
 * @param[in]  acmd           If true, CMD55 is sent before this command to
 *                            prefix it as an application-specific command.
 * @param[out] cmd_response_o Pointer to the response structure to populate.
 *
 * @return
 * - ERR_OK on success
 * - ERR_TIMEOUT if R1 or R1B is not received within the configured timeout
 * - Propagated error from the CMD55 prefix call if @p acmd is true
 */
static ERR_te sd_send_cmd(SPI_REGDEF_ts *spi_instance, uint8_t index, uint32_t arg, bool acmd, CMD_RESPONSE_ts *cmd_response_o) {
    ERR_te err;
    uint32_t start_time;
    uint32_t elapsed_time;

    uint8_t crc = 0x00;
    uint8_t cmd[6];

    if(acmd) {
        CMD_RESPONSE_ts dummy;
        err = sd_send_cmd(spi_instance, 55, 0, false, &dummy);
        if(err != ERR_OK) {
            LOG_ERROR(
                internal_state.subsys,
                LOG_LEVEL_ERROR,
                "sd_send_cmd: unknown error"
            );

            return err;
        }
    }

    if(index == 0x00) {
        crc = 0x95; // CMD0 requires valid CRC
    }
    else if(index == 0x08) {
        crc = 0x87; // CMD8 requires valid CRC
    }

    cmd[0] = 0x40 | index;
    cmd[1] = (arg >> 24) & 0xFF;
    cmd[2] = (arg >> 16) & 0xFF;
    cmd[3] = (arg >> 8) & 0xFF;
    cmd[4] = arg & 0xFF;
    cmd[5] = crc;

    spi_send(spi_instance, cmd, sizeof(cmd));

    // CMD12 requires discarding the first byte (stuff byte) before the R1 response
    if(index == 12) {
        uint8_t stuff_byte;
        spi_receive(spi_instance, &stuff_byte, 1);
    }

    uint8_t r1 = 0xFF;
    start_time = systick_get_ms();

    do {
        spi_receive(spi_instance, &r1, 1);
        elapsed_time = systick_get_ms() - start_time;
    } while ((r1 & 0x80) && elapsed_time <= CONFIG_SD_R1_RESP_TIMEOUT);

    cmd_response_o->r1 = r1;

    if(elapsed_time > CONFIG_SD_R1_RESP_TIMEOUT) {
        LOG_ERROR(
            internal_state.subsys,
            LOG_LEVEL_ERROR,
            "sd_send_cmd: timed out"
        );

        return ERR_TIMEOUT;
    }

    if(
        index == 0  || index == 1  || index == 41 || index == 9  ||
        index == 10 || index == 16 || index == 17 || index == 18 ||
        index == 23 || index == 24 || index == 25 || index == 55
    ) {
        // R1 only — zero unused fields
        memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
        memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));
    }
    else if(index == 58) {
        // R3 (OCR) — receive 4 trailing bytes
        memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));
        spi_receive(spi_instance, cmd_response_o->r3, 4);
    }
    else if(index == 8) {
        // R7 (interface condition) — receive 4 trailing bytes
        memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
        spi_receive(spi_instance, cmd_response_o->r7, 4);
    }
    else if(index == 12) {
        // R1B — poll until card releases MISO (busy signal ends)
        memset(cmd_response_o->r3, 0, sizeof(cmd_response_o->r3));
        memset(cmd_response_o->r7, 0, sizeof(cmd_response_o->r7));

        uint8_t rx = 0x00;
        start_time = systick_get_ms();

        do {
            spi_receive(spi_instance, &rx, 1);
            elapsed_time = systick_get_ms() - start_time;
        } while(rx == 0x00 && elapsed_time <= CONFIG_SD_R1B_RESP_TIMEOUT);

        if(elapsed_time > CONFIG_SD_R1B_RESP_TIMEOUT) {
            LOG_ERROR(
                internal_state.subsys,
                LOG_LEVEL_ERROR,
                "sd_send_cmd: timed out"
            );

            return ERR_TIMEOUT;
        }
    }

    return ERR_OK;
}

/**
 * @brief Raises CS, sends two dummy bytes, and disables SPI. Optionally deinitializes the handle.
 *
 * @details
 * Called after every completed or failed SPI transaction to return the bus
 * to an idle state. When @p deinit is true, temporarily sets the handle's
 * @c initialized flag to allow @ref sd_deinit_handle to run, then calls it
 * to release the pool slot.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 * @param[in] deinit    If true, deinitializes the handle after ending communication.
 *
 * @return
 * - ERR_OK always
 */
static ERR_te sd_cease_comms(SD_HANDLE_ts *sd_handle, bool deinit) {
    gpio_write(sd_handle->cs_gpio_port, sd_handle->cs_gpio_pin, HIGH);

    uint8_t dummy[2] = { 0xFF, 0xFF };
    spi_send(sd_handle->spi_instance, dummy, 2);

    spi_set_comm(sd_handle->spi_instance, DISABLE);

    if(deinit) {
        // Bypass init guard to allow sd_deinit_handle to proceed
        sd_handle->initialized = true;
        sd_deinit_handle(sd_handle);
    }

    return ERR_OK;
}

/**
 * @brief Issues CMD0 to reset the SD card into idle (SPI) mode.
 *
 * @details
 * Retries up to @ref CONFIG_SD_CMD_SEND_ERROR_RETRY_NUM times with
 * @ref CONFIG_SD_CMD_SEND_ERROR_RETRY_LATENCY ms between attempts.
 * Succeeds when R1 bit 0 (idle state) is set.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNSUCCESFUL_ACTION if the card does not enter idle state within the retry limit
 */
static ERR_te sd_go_idle_state(SD_HANDLE_ts *sd_handle) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };

    uint8_t retries = 0;
    do {
        retries++;
        err = sd_send_cmd(sd_handle->spi_instance, 0, 0, false, &cmd_response);
        if(err != ERR_OK || !(cmd_response.r1 & 0x01)) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_go_idle_state: command execution failure, retry %d/%d in %d ms",
                retries, CONFIG_SD_CMD_SEND_ERROR_RETRY_NUM, CONFIG_SD_CMD_SEND_ERROR_RETRY_LATENCY
            );
            DELAY(CONFIG_SD_CMD_SEND_ERROR_RETRY_LATENCY);
        }
    } while ((err != ERR_OK || !(cmd_response.r1 & 0x01)) && retries < CONFIG_SD_CMD_SEND_ERROR_RETRY_NUM);

    if(err != ERR_OK || !(cmd_response.r1 & 0x01)) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_go_idle_state: command execution failure"
        );

        return ERR_UNSUCCESFUL_ACTION;
    }

    return ERR_OK;
}

/**
 * @brief Issues CMD8 to determine whether the card is SD Ver.2 or older.
 *
 * @details
 * Sends CMD8 with the standard voltage argument 0x000001AA.
 * - If the R7 response echoes 0x000001AA, the card is SD Ver.2 and @p match_o is set.
 * - If CMD8 times out, the card is SD Ver.1 or MMC and @p no_resp_o is set.
 * - If the R7 does not match, the card is incompatible and an error is returned.
 *
 * @param[in]  sd_handle  Pointer to the SD card handle.
 * @param[out] match_o    Set to true if R7 echoes 0x000001AA (SD Ver.2).
 * @param[out] no_resp_o  Set to true if CMD8 produces no response (SD Ver.1 / MMC).
 *
 * @return
 * - ERR_OK on success (either match or no response)
 * - ERR_INITIALIZATION_FAILURE if R7 does not match the expected echo
 */
static ERR_te sd_send_if_cond(SD_HANDLE_ts *sd_handle, bool *match_o, bool *no_resp_o) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };

    err = sd_send_cmd(sd_handle->spi_instance, 8, 0x000001AA, false, &cmd_response);
    if(err != ERR_OK) {
        *no_resp_o = true;

        return ERR_OK;
    }

    uint8_t expected[] = { 0x00, 0x00, 0x01, 0xAA };
    for(uint8_t i = 0; i < 4; i++) {
        if(cmd_response.r7[i] != expected[i]) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_send_if_cond: initialization failure"
            );

            return ERR_INITIALIZATION_FAILURE;
        }
    }

    *match_o = true;

    return ERR_OK;
}

/**
 * @brief Issues ACMD41 to initiate SD card initialization (SD Ver.2 and Ver.1).
 *
 * @details
 * Retries up to @ref CONFIG_SD_INVALID_RESP_RETRY_NUM times while R1 equals
 * 0x01 (still initializing). Pass @p arg = 0x40000000 for SD Ver.2 (HCS bit
 * set) or @p arg = 0 for SD Ver.1.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 * @param[in] arg       ACMD41 argument. Use 0x40000000 for SDHC support,
 *                      0x00000000 for SDSC only.
 *
 * @return
 * - ERR_OK on success
 * - ERR_TIMEOUT if the R1 response is not received within timeout
 * - ERR_INITIALIZATION_FAILURE if the card does not complete initialization
 *   within the retry limit
 */
static ERR_te sd_app_send_op_cond(SD_HANDLE_ts *sd_handle, uint32_t arg) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };
    uint32_t retries = 0;

    do {
        retries++;
        err = sd_send_cmd(sd_handle->spi_instance, 41, arg, true, &cmd_response);
        if(err == ERR_TIMEOUT) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_app_send_op_cond: R1 response timeout"
            );

            return ERR_TIMEOUT;
        }

        if(cmd_response.r1 != 0x00) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_app_send_op_cond: invalid R1 response, retry %d/%d in %d ms",
                retries, CONFIG_SD_INVALID_RESP_RETRY_NUM, CONFIG_SD_INVALID_RESP_RETRY_TIMEOUT
            );
            DELAY(CONFIG_SD_INVALID_RESP_RETRY_TIMEOUT);
        }
    } while(cmd_response.r1 == 0x01 && retries < CONFIG_SD_INVALID_RESP_RETRY_NUM);

    if(cmd_response.r1 != 0x00) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_app_send_op_cond: initialization failure"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    return ERR_OK;
}

/**
 * @brief Reads the OCR register via CMD58 and stores power-up status, addressing mode, and voltage range.
 *
 * @details
 * Reconstructs the 32-bit OCR value from the 4-byte R3 response and
 * extracts: power-up completion flag (@ref SD_PWRUP_STATUS_te), Card
 * Capacity Status (block vs byte addressing), and the supported voltage
 * window (stored but not currently used for filtering).
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNSUCCESFUL_ACTION if CMD58 fails
 */
static ERR_te sd_read_ocr(SD_HANDLE_ts *sd_handle) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };

    err = sd_send_cmd(sd_handle->spi_instance, 58, 0, false, &cmd_response);
    if(err != ERR_OK) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_read_ocr: failed to read ocr"
        );

        return ERR_UNSUCCESFUL_ACTION;
    }

    uint32_t ocr =
        ((uint32_t)cmd_response.r3[0] << 24) |
        ((uint32_t)cmd_response.r3[1] << 16) |
        ((uint32_t)cmd_response.r3[2] << 8)  |
        ((uint32_t)cmd_response.r3[3]);

    sd_handle->pwrup_status = ((ocr >> OCR_PWRUP_STATUS) & 0x1) ?
        SD_PWRUP_STATUS_READY : SD_PWRUP_STATUS_BUSY;

    sd_handle->addr_mode = ((ocr >> OCR_CAPACITY_STATUS) & 0x1) ?
        SD_ADDR_MODE_BLOCK : SD_ADDR_MODE_BYTE;

    bool voltage_low_found = false;
    for(uint8_t ocr_counter = 4; ocr_counter < 24; ocr_counter++) {
        if((ocr >> ocr_counter) & 0x1 && !voltage_low_found) {
            sd_handle->min_operating_voltage = (SD_MIN_OPERATING_VOLTAGE_te)(23 - ocr_counter);
            voltage_low_found = true;
        }
        else if((ocr >> ocr_counter) & 0x1 && voltage_low_found) {
            sd_handle->max_operating_voltage = (SD_MAX_OPERATIING_VOLTAGE_te)(23 - ocr_counter);
        }
    }

    return ERR_OK;
}

/**
 * @brief Issues CMD1 to initiate MMC Ver.3 card initialization.
 *
 * @details
 * Used as a fallback when ACMD41 fails (card is MMC, not SD). Retries
 * while R1 equals 0x01 (still initializing).
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 * - ERR_TIMEOUT if the R1 response is not received within timeout
 * - ERR_INITIALIZATION_FAILURE if the card does not complete initialization
 */
static ERR_te sd_send_op_cond(SD_HANDLE_ts *sd_handle) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };
    uint32_t retries = 0;

    do {
        retries++;
        err = sd_send_cmd(sd_handle->spi_instance, 1, 0, false, &cmd_response);
        if(err == ERR_TIMEOUT) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_send_op_cond: R1 response timeout"
            );

            return ERR_TIMEOUT;
        }
    } while(cmd_response.r1 == 0x01 && retries < CONFIG_SD_INVALID_RESP_RETRY_NUM);

    if(cmd_response.r1 != 0x00) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_send_op_cond: initialization failure"
        );

        return ERR_INITIALIZATION_FAILURE;
    }

    return ERR_OK;
}

/**
 * @brief Issues CMD16 to set the block length to 512 bytes for byte-addressed cards.
 *
 * @details
 * Only called for SDSC (byte-addressed) cards. SDHC cards always use
 * a fixed 512-byte block length and do not require this command.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNSUCCESFUL_ACTION if CMD16 fails or the card returns a non-zero R1
 */
static ERR_te sd_set_blocklen(SD_HANDLE_ts *sd_handle) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };

    err = sd_send_cmd(sd_handle->spi_instance, 16, 0x00000200, false, &cmd_response);
    if(err != ERR_OK) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_set_blocklen: failed to set block length"
        );

        return ERR_UNSUCCESFUL_ACTION;
    }

    if(cmd_response.r1 != 0x00)
        return ERR_UNSUCCESFUL_ACTION;

    return ERR_OK;
}

/**
 * @brief Issues CMD9 to read the CSD register and stores capacity information in the handle.
 *
 * @details
 * Waits for the data token (0xFE), reads the 16-byte CSD register, and
 * dispatches to @ref decode_csd_v1 or @ref decode_csd_v2 based on the CSD
 * structure version field. CSD version 3 is treated as v2. The resulting
 * @ref sd_handle_s::capacity_mb, @ref sd_handle_s::block_count, and
 * @ref sd_handle_s::block_len fields are populated from the decoded data.
 *
 * @param[in] sd_handle Pointer to the SD card handle.
 *
 * @return
 * - ERR_OK on success
 * - ERR_UNSUCCESFUL_ACTION if CMD9 fails or the data token times out
 */
static ERR_te sd_read_csd(SD_HANDLE_ts *sd_handle) {
    ERR_te err;
    CMD_RESPONSE_ts cmd_response = { 0 };
    uint8_t csd_raw[16];
    SD_CSD_INFO_ts csd_info = { 0 };

    err = sd_send_cmd(sd_handle->spi_instance, 9, 0, false, &cmd_response);
    if (err != ERR_OK || cmd_response.r1 != 0x00) {
        LOG_ERROR(internal_state.subsys, internal_state.log_level,
                  "sd_read_csd: failed to send CMD9");
        return ERR_UNSUCCESFUL_ACTION;
    }

    uint8_t token = 0;
    uint32_t start_time = systick_get_ms();
    uint32_t elapsed_time = 0;

    do {
        spi_receive(sd_handle->spi_instance, &token, 1);
        elapsed_time = systick_get_ms() - start_time;
    } while (token != 0xFE && elapsed_time <= CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT);

    if (elapsed_time > CONFIG_SD_DATA_TOKEN_RECV_TIMEOUT || token != 0xFE) {
        LOG_ERROR(internal_state.subsys, internal_state.log_level,
                  "sd_read_csd: timeout waiting for data token");
        return ERR_UNSUCCESFUL_ACTION;
    }

    memset(csd_raw, 0, sizeof(csd_raw));
    spi_receive(sd_handle->spi_instance, csd_raw, 16);

    uint8_t crc[2];
    spi_receive(sd_handle->spi_instance, crc, 2);

    memset(&csd_info, 0, sizeof(SD_CSD_INFO_ts));

    uint8_t csd_structure = extract_bits(csd_raw, 126, 2);

    switch (csd_structure) {
        case 0:
            decode_csd_v1(csd_raw, &csd_info);
            break;
        case 1:
            decode_csd_v2(csd_raw, &csd_info);
            break;
        case 3:
            decode_csd_v2(csd_raw, &csd_info);
            // SDHC newer version — treated as v2
            break;
    }

    sd_handle->capacity_mb = csd_info.capacity_mb;
    sd_handle->block_count = csd_info.block_count;
    sd_handle->block_len   = csd_info.block_len;

    return ERR_OK;
}

/**
 * @brief Decodes a CSD v1.0 register (SDSC cards) into a @ref SD_CSD_INFO_ts structure.
 *
 * @details
 * Extracts all fields from the 16-byte big-endian CSD register using
 * @ref extract_bits and computes derived values:
 * - block_len = 2^READ_BL_LEN
 * - block_count = (C_SIZE + 1) × 2^(C_SIZE_MULT + 2)
 * - capacity_bytes = block_count × block_len
 * - capacity_mb = capacity_bytes / (1024 × 1024)
 *
 * @param[in]  csd_raw   Pointer to the 16-byte raw CSD register data.
 * @param[out] csd_info_o Pointer to the structure that will receive the decoded fields.
 *
 * @return
 * - ERR_OK always
 */
static ERR_te decode_csd_v1(const uint8_t *csd_raw, SD_CSD_INFO_ts *csd_info_o) {
    csd_info_o->csd_structure      = extract_bits(csd_raw, 126, 2);
    csd_info_o->taac               = extract_bits(csd_raw, 112, 8);
    csd_info_o->nsac               = extract_bits(csd_raw, 104, 8);
    csd_info_o->tran_speed         = extract_bits(csd_raw,  96, 8);
    csd_info_o->ccc                = extract_bits(csd_raw,  84, 12);
    csd_info_o->v1.read_bl_len     = extract_bits(csd_raw,  80, 4);
    csd_info_o->read_bl_partial    = extract_bits(csd_raw,  79, 1);
    csd_info_o->write_blk_misalign = extract_bits(csd_raw,  78, 1);
    csd_info_o->read_blk_misalign  = extract_bits(csd_raw,  77, 1);
    csd_info_o->dsr_imp            = extract_bits(csd_raw,  76, 1);
    csd_info_o->v1.c_size          = extract_bits(csd_raw,  62, 12);
    csd_info_o->v1.c_size_mult     = extract_bits(csd_raw,  47, 3);
    csd_info_o->erase_blk_en       = extract_bits(csd_raw,  46, 1);
    csd_info_o->sector_size        = extract_bits(csd_raw,  39, 7);
    csd_info_o->wp_grp_size        = extract_bits(csd_raw,  32, 7);
    csd_info_o->wp_grp_enable      = extract_bits(csd_raw,  31, 1);
    csd_info_o->r2w_factor         = extract_bits(csd_raw,  26, 3);
    csd_info_o->write_bl_len       = extract_bits(csd_raw,  22, 4);
    csd_info_o->write_bl_partial   = extract_bits(csd_raw,  21, 1);
    csd_info_o->file_format_grp    = extract_bits(csd_raw,  15, 1);
    csd_info_o->copy               = extract_bits(csd_raw,  14, 1);
    csd_info_o->perm_write_protect = extract_bits(csd_raw,  13, 1);
    csd_info_o->tmp_write_protect  = extract_bits(csd_raw,  12, 1);
    csd_info_o->file_format        = extract_bits(csd_raw,  10, 2);
    csd_info_o->wp_upc             = extract_bits(csd_raw,   9, 1);
    csd_info_o->crc                = extract_bits(csd_raw,   1, 7);

    csd_info_o->block_len    = 1 << csd_info_o->v1.read_bl_len;
    uint32_t mult            = 1 << (csd_info_o->v1.c_size_mult + 2);
    csd_info_o->block_count  = (csd_info_o->v1.c_size + 1) * mult;
    csd_info_o->capacity_bytes = csd_info_o->block_count * csd_info_o->block_len;
    csd_info_o->capacity_mb  = csd_info_o->capacity_bytes / (1024 * 1024);

    return ERR_OK;
}

/**
 * @brief Decodes a CSD v2.0 register (SDHC/SDXC cards) into a @ref SD_CSD_INFO_ts structure.
 *
 * @details
 * Extracts all fields from the 16-byte big-endian CSD register using
 * @ref extract_bits and computes derived values:
 * - block_len = 512 (fixed for SDHC)
 * - capacity_bytes = (C_SIZE + 1) × 512 × 1024
 * - capacity_mb = capacity_bytes / (1024 × 1024)
 * - block_count = capacity_bytes / 512
 *
 * @param[in]  csd_raw    Pointer to the 16-byte raw CSD register data.
 * @param[out] csd_info_o Pointer to the structure that will receive the decoded fields.
 *
 * @return
 * - ERR_OK always
 */
static ERR_te decode_csd_v2(const uint8_t *csd_raw, SD_CSD_INFO_ts *csd_info_o) {
    csd_info_o->csd_structure      = extract_bits(csd_raw, 126, 2);
    csd_info_o->taac               = extract_bits(csd_raw, 112, 8);
    csd_info_o->nsac               = extract_bits(csd_raw, 104, 8);
    csd_info_o->tran_speed         = extract_bits(csd_raw,  96, 8);
    csd_info_o->ccc                = extract_bits(csd_raw,  84, 12);
    csd_info_o->v1.read_bl_len     = extract_bits(csd_raw,  80, 4);
    csd_info_o->read_bl_partial    = extract_bits(csd_raw,  79, 1);
    csd_info_o->write_blk_misalign = extract_bits(csd_raw,  78, 1);
    csd_info_o->read_blk_misalign  = extract_bits(csd_raw,  77, 1);
    csd_info_o->dsr_imp            = extract_bits(csd_raw,  76, 1);
    csd_info_o->v2.c_size          = extract_bits(csd_raw,  48, 22);
    csd_info_o->erase_blk_en       = extract_bits(csd_raw,  46, 1);
    csd_info_o->sector_size        = extract_bits(csd_raw,  39, 7);
    csd_info_o->wp_grp_size        = extract_bits(csd_raw,  32, 7);
    csd_info_o->wp_grp_enable      = extract_bits(csd_raw,  31, 1);
    csd_info_o->r2w_factor         = extract_bits(csd_raw,  26, 3);
    csd_info_o->write_bl_len       = extract_bits(csd_raw,  22, 4);
    csd_info_o->write_bl_partial   = extract_bits(csd_raw,  21, 1);
    csd_info_o->file_format_grp    = extract_bits(csd_raw,  15, 1);
    csd_info_o->copy               = extract_bits(csd_raw,  14, 1);
    csd_info_o->perm_write_protect = extract_bits(csd_raw,  13, 1);
    csd_info_o->tmp_write_protect  = extract_bits(csd_raw,  12, 1);
    csd_info_o->file_format        = extract_bits(csd_raw,  10, 2);
    csd_info_o->wp_upc             = extract_bits(csd_raw,   9, 1);
    csd_info_o->crc                = extract_bits(csd_raw,   1, 7);

    csd_info_o->block_len      = 512;
    csd_info_o->capacity_bytes = (csd_info_o->v2.c_size + 1) * 512 * 1024;
    csd_info_o->capacity_mb    = csd_info_o->capacity_bytes / (1024 * 1024);
    csd_info_o->block_count    = csd_info_o->capacity_bytes / 512;

    return ERR_OK;
}

/** @} */

/**
 * @defgroup sd_command_handlers SD Command Handlers
 * @{
 */

/**
 * @brief CLI handler for the "list" command. Logs the names of all active SD card handles.
 *
 * @details
 * Expected invocation: `sd list`
 *
 * @param[in] argc Argument count. Must be exactly 2.
 * @param[in] argv Argument list: argv[0] = "sd", argv[1] = "list".
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 2
 */
static ERR_te sd_cmd_list_handler(uint32_t argc, char **argv) {
    if(argc != 2) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_cmd_list_handler: invalid arguments"
        );
        return ERR_INVALID_ARGUMENT;
    }

    for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
        if(internal_state.sds[i].in_use == true) {
            LOG_INFO(
                internal_state.subsys,
                internal_state.log_level,
                "%s",
                internal_state.sds[i].name
            );
        }
    }

    return ERR_OK;
}

/**
 * @brief CLI handler for the "info" command. Logs detailed information about a named SD card handle.
 *
 * @details
 * Expected invocation: `sd info <n>`
 *
 * Searches the pool for a handle whose name matches @c argv[2] and logs
 * its type (SDSC/SDHC), addressing mode (byte/block), capacity in MB,
 * block size in bytes, and block count.
 *
 * @param[in] argc Argument count. Must be exactly 3.
 * @param[in] argv Argument list: argv[0] = "sd", argv[1] = "info",
 *                 argv[2] = SD card handle name.
 *
 * @return
 * - ERR_OK on success
 * - ERR_INVALID_ARGUMENT if @p argc != 3 or no handle with the given name exists
 */
static ERR_te sd_cmd_info_handler(uint32_t argc, char **argv) {
    if(argc != 3) {
        LOG_ERROR(
            internal_state.subsys,
            internal_state.log_level,
            "sd_cmd_info_handler: invalid arguments"
        );
        return ERR_INVALID_ARGUMENT;
    }

    for(uint32_t i = 0; i < CONFIG_SD_MAX_OBJECTS; i++) {
        if(str_cmp(internal_state.sds[i].name, argv[2]) == true) {
            char type_str[10];
            char addr_mode[10];

            switch(internal_state.sds[i].type) {
                case SDSC: str_cpy(type_str, "SDSC", get_str_len("SDSC") + 1); break;
                case SDHC: str_cpy(type_str, "SDHC", get_str_len("SDHC") + 1); break;
            }

            switch(internal_state.sds[i].addr_mode) {
                case SD_ADDR_MODE_BLOCK: str_cpy(addr_mode, "block", get_str_len("block") + 1); break;
                case SD_ADDR_MODE_BYTE:  str_cpy(addr_mode, "byte",  get_str_len("byte")  + 1); break;
            }

            LOG_INFO(
                internal_state.subsys,
                internal_state.log_level,
                "type: %s, addr mode: %s, capacity: %d mb, block size: %d byte, block count: %d",
                type_str,
                addr_mode,
                internal_state.sds[i].capacity_mb,
                internal_state.sds[i].block_len,
                internal_state.sds[i].block_count
            );

            break;
        }

        if(i == CONFIG_SD_MAX_OBJECTS - 1) {
            LOG_ERROR(
                internal_state.subsys,
                internal_state.log_level,
                "sd_cmd_info_handler: no such handle"
            );

            return ERR_INVALID_ARGUMENT;
        }
    }

    return ERR_OK;
}

/** @} */