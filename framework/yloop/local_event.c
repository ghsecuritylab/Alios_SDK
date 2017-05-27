/*
* Copyright (C) 2016 YunOS Project. All rights reserved.
*/
/**
 * Copyright (C) 2016 YunOS Project. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <yos/kernel.h>
#include <yos/framework.h>
#include <yos/list.h>

#include "event_device.h"
#include "yloop.h"

typedef struct {
    dlist_t       node;
    yos_event_cb  cb;
    void         *priv;
    uint16_t      type_filter;
} event_list_node_t;

static struct {
    void *handle;
    int   fd;
} local_event = {
    .fd = -1,
};

static dlist_t g_local_event_list = YOS_DLIST_INIT(g_local_event_list);

static void handle_events(input_event_t *event);
static int  input_add_event(int fd, input_event_t *event);
static void event_read_cb(int fd, void *param);

/* Handle events
 * just dispatch
 */
void handle_events(input_event_t *event)
{
    if (event->type == EV_RPC) {
        yos_call_t handler = (yos_call_t)event->value;
        void *arg = (void *)event->extra;
        handler(arg);

        return;
    }

    event_list_node_t *event_node = NULL;
    dlist_for_each_entry(&g_local_event_list, event_node, event_list_node_t, node) {
        if (event_node->type_filter != EV_ALL
            && event_node->type_filter != event->type)
            continue;
        (event_node->cb)(event, event_node->priv);
    }
}

static int input_add_event(int fd, input_event_t *event)
{
    bool urgent = event->type & EV_FLAG_URGENT;
    event->type &= ~EV_FLAG_URGENT;
    int cmd;

    if (urgent)
        cmd = MK_CMD(IOCTL_WRITE_URGENT, sizeof(*event));
    else
        cmd = MK_CMD(IOCTL_WRITE_NORMAL, sizeof(*event));

    return yos_ioctl(fd, cmd, (unsigned long)event);
}

void event_read_cb(int fd, void *param)
{
    input_event_t event;
    int ret = yos_read(fd, &event, sizeof(event));
    if (ret == sizeof(event)) {
        handle_events(&event);
    }
}

int yos_event_service_init(void)
{
    int fd = yos_open("/dev/event", 0);

    if (local_event.fd < 0)
        local_event.fd = fd;
    yos_poll_read_fd(fd, event_read_cb, NULL);
    yos_loop_set_eventfd(fd);

    return 0;
}

void yos_event_service_deinit(int fd)
{
    yos_cancel_poll_read_fd(fd, event_read_cb, NULL);
}

int yos_post_event(uint16_t type, uint16_t code, unsigned long value)
{
    input_event_t event = {
        .type  = type,
        .code  = code,
        .value = value,
    };

    return input_add_event(local_event.fd, &event);
}

int yos_register_event_filter(uint16_t type, yos_event_cb cb, void *priv)
{
    event_list_node_t* event_node = malloc(sizeof(event_list_node_t));
    if(NULL == event_node){
        return -1;
    }

    event_node->cb           = cb;
    event_node->type_filter  = type;
    event_node->priv         = priv;

    dlist_add_tail(&event_node->node, &g_local_event_list);

    return 0;
}

int yos_unregister_event_filter(uint16_t type, yos_event_cb cb, void *priv)
{
    event_list_node_t* event_node = NULL;
    dlist_for_each_entry(&g_local_event_list, event_node, event_list_node_t, node) {
        if (event_node->type_filter != type)
            continue;

        if (event_node->cb != cb)
            continue;

        if (event_node->priv != priv)
            continue;

        dlist_del(&event_node->node);
        free(event_node);
        return 0;
    }

    return -1;
}

/*
 * schedule a callback in yos loop main thread
 */
static int _schedule_call(yos_loop_t *loop, yos_call_t fun, void *arg, bool urgent)
{
    input_event_t event = {
        .type = EV_RPC,
        .value = (unsigned long)fun,
        .extra = (unsigned long)arg,
    };
    int fd = yos_loop_get_eventfd(loop);
    if (fd < 0)
        fd = local_event.fd;

    if (urgent)
        event.type |= EV_FLAG_URGENT;
    return input_add_event(fd, &event);
}

int yos_loop_schedule_urgent_call(yos_loop_t *loop, yos_call_t fun, void *arg)
{
    return _schedule_call(loop, fun, arg, true);
}

int yos_loop_schedule_call(yos_loop_t *loop, yos_call_t fun, void *arg)
{
    return _schedule_call(loop, fun, arg, false);
}

int yos_schedule_call(yos_call_t fun, void *arg)
{
    return _schedule_call(NULL, fun, arg, false);
}

typedef struct work_para {
    yos_work_t *work;
    yos_loop_t loop;
    yos_call_t action;
    void *arg1;
    yos_call_t fini_cb;
    void *arg2;
} work_par_t;

static void run_my_work(void *arg)
{
    work_par_t *wpar = arg;

    wpar->action(wpar->arg1);

    if (wpar->fini_cb)
        yos_loop_schedule_call(wpar->loop, wpar->fini_cb, wpar->arg2);

    free(wpar->work);
    free(wpar);
}

void yos_cancel_work(void *work, yos_call_t action, void *arg)
{
    int ret = yos_work_cancel(work);
    if (ret != 0)
        return;

    work_par_t *wpar = arg;
    if (wpar->work != work)
        return;

    free(wpar->work);
    free(wpar);
}

void *yos_schedule_work(int ms, yos_call_t action, void *arg1, yos_call_t fini_cb, void *arg2)
{
    int ret;

    yos_work_t *work = malloc(sizeof(*work));
    work_par_t *wpar = malloc(sizeof(*wpar));

    if (!work || !wpar)
        goto err_out;

    wpar->work = work;
    wpar->loop = yos_current_loop();
    wpar->action = action;
    wpar->arg1 = arg1;
    wpar->fini_cb = fini_cb;
    wpar->arg2 = arg2;

    ret = yos_work_init(work, run_my_work, wpar, ms);
    if (ret != 0)
        goto err_out;
    ret = yos_work_sched(work);
    if (ret != 0)
        goto err_out;

    return work;
err_out:
    free(work);
    free(wpar);
    return NULL;
}

