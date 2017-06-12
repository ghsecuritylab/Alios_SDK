#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 

#include <errno.h>
#include "hal/ota.h"
#include "yos/log.h"
#include "hal/soc/soc.h"

typedef struct
{
    uint32_t ota_len;
    uint32_t ota_crc;
} ota_reboot_info_t;

typedef struct  _boot_table_t {
  uint32_t start_address; // the address of the bin saved on flash.
  uint32_t length; // file real length
  uint8_t version[8];
  uint8_t type; // B:bootloader, P:boot_table, A:application, D: 8782 driver
  uint8_t upgrade_type; //u:upgrade, 
  uint16_t crc;
  uint8_t reserved[4];
}boot_table_t;

int hal_ota_switch_to_new_fw( int ota_data_len, uint16_t ota_data_crc )
{
    uint32_t offset;
    boot_table_t boot_tbl,boot_tbl_rb;
    hal_logic_partition_t* ota_partition;
    
    ota_partition = hal_flash_get_info( HAL_PARTITION_OTA_TEMP );

    memset( &boot_tbl, 0, sizeof(boot_table_t) );
    boot_tbl.length = ota_data_len;
    boot_tbl.start_address = ota_partition->partition_start_addr;
    boot_tbl.type = 'A';
    boot_tbl.upgrade_type = 'U';
    boot_tbl.crc = ota_data_crc;

    offset = 0x00;
    hal_flash_erase( HAL_PARTITION_PARAMETER_1, offset, sizeof(boot_table_t) );

    offset = 0x00;
    hal_flash_write( HAL_PARTITION_PARAMETER_1, &offset, (const void *)&boot_tbl, sizeof(boot_table_t));

    offset = 0x00;
    memset(&boot_tbl_rb, 0, sizeof(boot_table_t));
    hal_flash_read( HAL_PARTITION_PARAMETER_1, &offset, &boot_tbl_rb, sizeof(boot_table_t));

    if(memcmp(&boot_tbl, &boot_tbl_rb, sizeof(boot_table_t)) != 0)
    {
        return -1;
    }

    /* reboot */
    ((void(*)(void))0)();

    return 0;
}

static int moc108_ota_init(hal_ota_module_t *m, void *something)
{
#if 0
    hal_logic_partition_t *partition_info;
    
    partition_info = hal_flash_get_info( HAL_PARTITION_OTA_TEMP );
    hal_flash_erase(HAL_PARTITION_OTA_TEMP, 0 ,partition_info.partition_size);
#endif
    
    return 0;
}

static int moc108_ota_write(hal_ota_module_t *m, volatile uint32_t* off_set, uint8_t* in_buf ,uint32_t in_buf_len)
{
    hal_flash_write(HAL_PARTITION_OTA_TEMP, off_set, in_buf, in_buf_len);
    return 0;
}

static int moc108_ota_read(hal_ota_module_t *m,  volatile uint32_t* off_set, uint8_t* out_buf, uint32_t out_buf_len)
{
    hal_flash_read(HAL_PARTITION_OTA_TEMP, off_set, out_buf, out_buf_len);
    return 0;
}

static int moc108_ota_set_boot(hal_ota_module_t *m, void *something)
{
    ota_reboot_info_t *p_info = (ota_reboot_info_t *)something;
    hal_ota_switch_to_new_fw(p_info->ota_len, p_info->ota_crc);
    return 0;
}

struct hal_ota_module_s moc108_ota_module = {
    .init = moc108_ota_init,
    .ota_write = moc108_ota_write,
    .ota_read = moc108_ota_read,
    .ota_set_boot = moc108_ota_set_boot,
};
