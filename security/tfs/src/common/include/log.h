/*
 *  Copyright (C) 2015 YunOS Project. All rights reserved.
 */

#ifndef _TFS_LOG_H
#define _TFS_LOG_H

#include <stdio.h>

#ifndef LOG

#define LOG printf
#define LOGI(tag, _fmt, _arg ...) LOG("I:%s: "_fmt, tag, ##_arg)

#ifdef TFS_DEBUG
#define LOGD(tag, _fmt, _arg ...) LOG("D:%s: "_fmt, tag, ##_arg)
#else
#define LOGD(tag, _fmt, _arg ...)
#endif

#define LOGE(tag, _fmt, _arg ...) LOG("E:%s: "_fmt, tag, ##_arg)
#endif

#endif // _TFS_LOG_H
