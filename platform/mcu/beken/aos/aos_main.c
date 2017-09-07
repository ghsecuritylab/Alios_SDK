/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <aos_main.h>
#include <k_api.h>
#include <yos/kernel.h>

#define YOS_START_STACK 2048

ktask_t *g_yos_init;

extern void board_init(void);

extern int application_start(int argc, char **argv);

void trace_start(void);

#ifdef YOS_BINS
extern void *g_syscall_tbl[];
extern char  app_info_addr;

extern k_mm_head  *g_kmm_head;

struct app_info_t *app_info = (struct app_info_t *)&app_info_addr;

static void application_init(void)
{
    memcpy((void *)(app_info->data_ram_start), (void *)(app_info->data_flash_begin),
           app_info->data_ram_end - app_info->data_ram_start);
    memset((void *)(app_info->bss_start), 0, app_info->bss_end - app_info->bss_start);

    krhino_add_mm_region(g_kmm_head, (void *)(app_info->heap_start),
                        app_info->heap_end - app_info->heap_start);

    krhino_mm_leak_region_init((void *)(app_info->data_ram_start), (void *)(app_info->data_ram_end));
    krhino_mm_leak_region_init((void *)(app_info->bss_start), (void *)(app_info->bss_end));
}
#endif

static void yos_init(void)
{
    int i = 0;

    soc_system_init();

#ifdef BOOTLOADER
    main();
#else

    board_init();

    vfs_init();
    vfs_device_init();

    for (i = 0; i < 10; i++) {
        vflash_register_partition(i);
    }

    yos_cli_init();
    yos_kv_init();
    yos_loop_init();

    trace_start();
    
#ifdef YOS_BINS
    application_init();

    if (app_info->app_entry) {
        app_info->app_entry((void *)g_syscall_tbl, 0, NULL);
    }
#else
    yos_framework_init();
    application_start(0, NULL);
#endif

#endif
}

void yos_start(void)
{
    krhino_init();

    soc_driver_init();

    krhino_task_dyn_create(&g_yos_init, "yos-init", 0, YOS_DEFAULT_APP_PRI, 0, YOS_START_STACK, yos_init, 1);

    krhino_start();
}

