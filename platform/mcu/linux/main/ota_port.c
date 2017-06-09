#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 

#include <errno.h>
#include "hal/ota.h"
#include "yos/log.h"

static int linuxhost_ota_init(hal_ota_module_t *m, void *something)
{
    return 0;
}

static FILE* ota_fd = NULL;
#define OTA_IMAGE_TMP_FILE "/tmp/aos_firmware_temp"
#define OTA_IMAGE_FILE     "alinkapp@linuxhost.elf"

int linuxhost_ota_write(hal_ota_module_t *m, volatile uint32_t* off_set, uint8_t* in_buf ,uint32_t in_buf_len)
{
    int ret = 0;
    
     
    if(ota_fd == NULL) 
    {
        if((access(OTA_IMAGE_TMP_FILE, 0 )) != -1 )
        {
            printf( "File tmp exists \n " );
            remove(OTA_IMAGE_TMP_FILE);
        }
	ota_fd = fopen(OTA_IMAGE_TMP_FILE, "w");
    }

    ret = fwrite(in_buf, in_buf_len, 1, ota_fd);
    if(ret != 1) {
        printf("write error: %d, %d ,%s\n", ret, in_buf_len ,strerror(errno));
	return -1;
    }
    return 0;
}

static int linuxhost_ota_read(hal_ota_module_t *m,  volatile uint32_t* off_set, uint8_t* out_buf, uint32_t out_buf_len)
{
    return 0;
}

static int linuxhost_ota_set_boot(hal_ota_module_t *m, void *something)
{
   if(ota_fd != NULL) { 
      fflush(ota_fd);
      fclose(ota_fd);
   }
   char cmd1[256] = {0};
   sprintf(cmd1, "mv %s ./%s", OTA_IMAGE_TMP_FILE, OTA_IMAGE_FILE);
   system(cmd1);
   
   memset(cmd1,0,sizeof cmd1);
   sprintf(cmd1, "chmod +x ./%s", OTA_IMAGE_FILE);
   system(cmd1);
   printf("Rebooting and updating FLASH now....\n");
   return 0;
}


struct hal_ota_module_s linuxhost_ota_module = {
    .init = linuxhost_ota_init,
    .ota_write = linuxhost_ota_write,
    .ota_read = linuxhost_ota_read,
    .ota_set_boot = linuxhost_ota_set_boot,
};
