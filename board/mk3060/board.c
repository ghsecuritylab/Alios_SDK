#include "hal/soc/soc.h"
#include <yos/kernel.h>
#include <yos/framework.h>

/* Logic partition on flash devices */
const hal_logic_partition_t hal_partitions[] =
{
	[HAL_PARTITION_BOOTLOADER] =
	{
	    .partition_owner            = HAL_FLASH_EMBEDDED,
	    .partition_description      = "Bootloader",
	    .partition_start_addr       = 0x0,
	    .partition_length           = 0x10000,    //64k bytes
	    .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_DIS,
	},
	[HAL_PARTITION_PARAMETER_1] =
    {
        .partition_owner            = HAL_FLASH_EMBEDDED,
        .partition_description      = "PARAMETER1",
        .partition_start_addr       = 0x10000,
        .partition_length           = 0x1000, // 4k bytes
        .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
    },
    [HAL_PARTITION_PARAMETER_2] =
    {
        .partition_owner            = HAL_FLASH_EMBEDDED,
        .partition_description      = "PARAMETER2",
        .partition_start_addr       = 0x11000,
        .partition_length           = 0x1000, //4k bytes
        .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
    },
	[HAL_PARTITION_APPLICATION] =
	{
	    .partition_owner            = HAL_FLASH_EMBEDDED,
	    .partition_description      = "Application",
	    .partition_start_addr       = 0x13000,
	    .partition_length           = 0x8E000, //568k bytes
	    .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
	},
    [HAL_PARTITION_OTA_TEMP] =
    {
        .partition_owner           = HAL_FLASH_EMBEDDED,
        .partition_description     = "OTA Storage",
        .partition_start_addr      = 0xA1000,
        .partition_length          = 0x5F000, //380k bytes
        .partition_options         = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
    },
    [HAL_PARTITION_PARAMETER_3] =
    {
        .partition_owner            = HAL_FLASH_EMBEDDED,
        .partition_description      = "PARAMETER3",
        .partition_start_addr       = 0x12000,
        .partition_length           = 0x1000, //4k bytes
        .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
    },
    [HAL_PARTITION_PARAMETER_4] =
    {
        .partition_owner            = HAL_FLASH_EMBEDDED,
        .partition_description      = "PARAMETER4",
        .partition_start_addr       = 0xD000,
        .partition_length           = 0x1000, //4k bytes
        .partition_options          = PAR_OPT_READ_EN | PAR_OPT_WRITE_EN,
    },
};

#define KEY_STATUS 1
#define KEY_ELINK  2
#define KEY_BOOT   7

static uint64_t   elink_time = 0;
static gpio_dev_t gpio_key_boot;

static void key_poll_func(void *arg)
{
    uint32_t level;
    uint64_t diff;

    hal_gpio_input_get(&gpio_key_boot, &level);

    if (level == 0) {
        yos_post_delayed_action(10, key_poll_func, NULL);
    } else {
        diff = yos_now_ms() - elink_time;
        if (diff > 6000) { /*long long press */
            elink_time = 0;
            yos_post_event(EV_KEY, CODE_BOOT, VALUE_KEY_LLTCLICK);
        } else if (diff > 2000) { /* long press */
            elink_time = 0;
            yos_post_event(EV_KEY, CODE_BOOT, VALUE_KEY_LTCLICK);
        } else if (diff > 40) { /* short press */
            elink_time = 0;
            yos_post_event(EV_KEY, CODE_BOOT, VALUE_KEY_CLICK);
        } else {
            yos_post_delayed_action(10, key_poll_func, NULL);
        }
    }
}

static void key_proc_work(void *arg)
{
    yos_schedule_call(key_poll_func, NULL);
}

static void handle_elink_key(void *arg)
{
    uint32_t gpio_value;

    hal_gpio_input_get(&gpio_key_boot, &gpio_value);
    if (gpio_value == 0 && elink_time == 0) {
        elink_time = yos_now_ms();
        yos_loop_schedule_work(0, key_proc_work, NULL, NULL, NULL);
    }
}

void board_init(void)
{
    gpio_key_boot.port = KEY_BOOT;

    hal_gpio_clear_irq(&gpio_key_boot);
    hal_gpio_enable_irq(&gpio_key_boot, IRQ_TRIGGER_FALLING_EDGE, handle_elink_key, NULL);
}
