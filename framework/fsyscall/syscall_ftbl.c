/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <k_api.h>
#include <aos/aos.h>
#include <aos/alink.h>
#ifdef CONFIG_YWSS
#include <enrollee.h>
#endif
#ifdef CONFIG_ALINK
#include <accs.h>
#include <gateway_service.h>
#include <json_parser.h>
#include <config.h>
#endif
#include <netmgr.h>


extern const char *gateway_get_uuid(void);
extern bool gateway_is_connected(void);


/* for syscall_ftbl.h, on the last */
#include <syscall_ftbl.h>

