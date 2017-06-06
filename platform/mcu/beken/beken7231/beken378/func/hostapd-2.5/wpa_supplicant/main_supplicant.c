/*
 * WPA Supplicant / main() function for UNIX like OSes and MinGW
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "fst/fst.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

#include "main_none.h"
#include "ps.h"

#include "FreeRTOS.h"
#include "task.h"
#include "rtos_pub.h"
#include "error.h"
#include "uart_pub.h"

static struct wpa_global *wpa_global_ptr;
static xTaskHandle wpas_thread_handle;
uint32_t wpas_stack_size = 4000;
beken_semaphore_t wpas_sema = NULL;
struct wpa_ssid_value *wpas_connect_ssid = 0;
struct wpa_interface *wpas_ifaces = 0;

extern void wpas_thread_start(void);

int supplicant_main_exit(void)
{
    //wpas_thread_stop();

    if(wpa_global_ptr)
    {
        wpa_supplicant_deinit(wpa_global_ptr);
    }

	if(wpas_ifaces)
	{
		os_free(wpas_ifaces);
		wpas_ifaces = 0;
	}
	
	if(wpas_connect_ssid)
	{
		os_free(wpas_connect_ssid);
		wpas_connect_ssid = 0;
	}
	return 0;
}

int supplicant_main_entry(char *oob_ssid)
{
    int i;
    int iface_count, exitcode = -1;
    struct wpa_params params;
    struct wpa_supplicant *wpa_s;
    struct wpa_interface *iface;

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = MSG_INFO;

	if(0 == wpas_ifaces)
	{
		wpas_ifaces = os_zalloc(sizeof(struct wpa_interface));		
	    if (wpas_ifaces == NULL)
	        return -1;
	}
	
    iface = wpas_ifaces;
    iface_count = 1;
    iface->ifname = bss_iface;
    exitcode = 0;

    wpa_global_ptr = wpa_supplicant_init(&params);
    if (wpa_global_ptr == NULL)
    {
        wpa_printf(MSG_ERROR, "Failed to initialize wpa_supplicant");
        exitcode = -1;
        goto out;
    }
    else
    {
        wpa_printf(MSG_INFO, "Successfully initialized wpa_supplicant");
    }

    for (i = 0; exitcode == 0 && i < iface_count; i++)
    {
        if (wpas_ifaces[i].ctrl_interface == NULL &&
                wpas_ifaces[i].ifname == NULL)
        {
            if (iface_count == 1 
				&& (params.ctrl_interface 
					|| params.dbus_ctrl_interface))
            {
                break;
            }
			
            exitcode = -1;
            break;
        }

        wpa_s = wpa_supplicant_add_iface(wpa_global_ptr, &wpas_ifaces[i], NULL);
        if (wpa_s == NULL)
        {
            exitcode = -1;
            break;
        }
		
		
        if(oob_ssid)
        {
            int len;
            int oob_ssid_len;

            ASSERT(0 == wpa_s->ssids_from_scan_req);
            oob_ssid_len = os_strlen(oob_ssid);

			if(0 == wpas_connect_ssid)
			{
	            wpas_connect_ssid = (struct wpa_ssid_value *)os_malloc(sizeof(struct wpa_ssid_value));
	            ASSERT(wpas_connect_ssid);
			}

            len = MIN(SSID_MAX_LEN, oob_ssid_len);

            wpas_connect_ssid->ssid_len = len;
            os_memcpy(wpas_connect_ssid->ssid, oob_ssid, len);

            wpa_s->num_ssids_from_scan_req = 1;
            wpa_s->ssids_from_scan_req = wpas_connect_ssid;
            wpa_s->scan_req = MANUAL_SCAN_REQ;
            os_printf("MANUAL_SCAN_REQ\r\n");
        }
    }

    if (exitcode)
    {
        wpa_supplicant_deinit(wpa_global_ptr);
    }
    else
	{
		dhcp_stop_timeout_check();
    	wpas_thread_start();
		return 0;
	}

out:
    os_free(wpas_ifaces);
    os_free(params.pid_file);

    return exitcode;
}

static void wpas_thread_main( void *arg )
{
    OSStatus ret;

    while(1)
    {
        ret = rtos_get_semaphore(&wpas_sema, BEKEN_WAIT_FOREVER);
        ASSERT(kNoErr == ret);

        if(wpa_supplicant_run(wpa_global_ptr) < 0)
        {
            os_printf("Failed to start eloop\r\n");
        }
    }
}

void wpas_thread_start(void)
{
    OSStatus ret;

    if(NULL == wpas_sema)
    {
	    ret = rtos_init_semaphore(&wpas_sema, 1);
	    ASSERT(kNoErr == ret);
	}

    if(NULL == wpas_thread_handle)
    {
	    ret = rtos_create_thread(&wpas_thread_handle,
	                             THD_WPAS_PRIORITY,
	                             "wpas_thread",
	                             (beken_thread_function_t)wpas_thread_main,
	                             (unsigned short)wpas_stack_size,
	                             (beken_thread_arg_t)NULLPTR);
	    ASSERT(kNoErr == ret);
	}
}

void wpas_thread_stop(void)
{
    OSStatus ret;

    if(wpas_thread_handle)
    {
        ret = rtos_delete_thread(&wpas_thread_handle);
        ASSERT(kNoErr == ret);
		wpas_thread_handle = NULL;
    }

    if(wpas_sema)
    {
        ret = rtos_deinit_semaphore(&wpas_sema);
        ASSERT(kNoErr == ret);
		wpas_sema = NULL;
    }
}

void wpa_supplicant_poll(void *param)
{
    OSStatus ret;

	if(wpas_sema)
	{
    	ret = rtos_set_semaphore(&wpas_sema);
	}
}

// eof

