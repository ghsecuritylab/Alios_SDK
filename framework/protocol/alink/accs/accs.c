#include <stdlib.h>
#include "accs.h"
#include "alink_protocol.h"
#include "connectivity_manager.h"
#include "yos/list.h"
#include "yos/framework.h"
#include "yos/log.h"
#include "service_manager.h"
#include "wsf.h"
#include "yos/kernel.h"
#include "alink_export_internal.h"
#include "os.h"
#include "service.h"

#define ACCS_DEBUG 0
#if(ACCS_DEBUG==1)
    #define accs_debug LOGD
#else
    #define accs_debug ;
#endif

static dlist_t g_accs_listener_list;
const device_t *main_device = 0;
const connectivity_t *remote_conn = 0;
uint8_t *uplink_buff = 0;
uint8_t *downlink_buff = 0;
void *link_buff_mutex;

static int accs_set_state(int);
static int accs_get_state(void);
static int accs_conn_listener(int, void*, int, void*, int*);
static int accs_broadcast(int, void*, int, void*, int*);
static int accs_notify_event(service_cb, int);
static int accs_broadcast_event(int);
static int accs_notify_data(service_cb, void*, int, void*, int*);
static int accs_broadcast_data(void*, int, void*, int*);
static void accs_handshake(void *arg);
static int accs_event_handler(int type, void *data, int dlen, void *result, int *rlen);

#define MODULE_NAME_ACCS "accs"
SERVICE_DEFINE(accs);

void start_accs_work()
{
    yos_schedule_work(0,accs_handshake,NULL,NULL,NULL);
}

/*
 * SERVICE_STATE_INIT: cloud thread is working
 * SERVICE_STATE_PREPARE: cloud is connected
 * SERVICE_STATE_READY: accs ready
 * SERVICE_STATE_STOP: accs stoped
 */
int accs_prepare() {
    if(accs_get_state() == SERVICE_STATE_STOP) {
        accs_set_state(SERVICE_STATE_INIT);
        main_device = get_main_dev();
        remote_conn = cm_get_conn("wsf");
        cm_bind_conn("wsf", &accs_conn_listener);
        link_buff_mutex = os_mutex_init();
        remote_conn->init_buff(&uplink_buff, &downlink_buff);
        remote_conn->connect();//FIXME: what happen if connection failed
    }
    return SERVICE_RESULT_OK;
}

int accs_start() {
    start_accs_work(NULL);
    accs_add_listener(&accs_event_handler);
    return SERVICE_RESULT_OK;
}

int accs_stop() {
    if(accs_get_state() != SERVICE_STATE_STOP) {
        LOG("accs will stop current session.\n");
        cm_get_conn("wsf")->disconnect();//wsf_disconnect
        cm_release_conn("wsf", &accs_conn_listener);
        accs_set_state(SERVICE_STATE_STOP);
        os_mutex_destroy(link_buff_mutex);
        accs_del_listener(&accs_event_handler);
    }
    return SERVICE_RESULT_OK;
}

int accs_put(void *in, int len) {
    int ret;
    if(accs_get_state() != SERVICE_STATE_READY) {
        LOGW(MODULE_NAME_ACCS,"accs not ready!");
        return (0-accs_get_state());
    }
    /* handle transaction */
    alink_data_t *pack = (alink_data_t*)in;
    ret = __alink_post(pack->method, pack->data);
    return (ret==ALINK_CODE_SUCCESS)? SERVICE_RESULT_OK : ret;
}

int accs_get(void *in, int inlen, void *out, int outlen) {
    int ret;
    if(accs_get_state() != SERVICE_STATE_READY) {
        LOGW(MODULE_NAME_ACCS,"accs not ready!");
        return (0-accs_get_state());
    }
    if(!out || outlen <=0) {
        LOGE(MODULE_NAME_ACCS,"accs_get fail, illegal output buffer !");
        return SERVICE_RESULT_ERR;
    }

    /* handle transaction */
    alink_data_t *p = (alink_data_t*)in;
    ret = alink_get(p->method, p->data, out, outlen);
    return (ret==ALINK_CODE_SUCCESS)? SERVICE_RESULT_OK : ret;
}

int accs_add_listener(service_cb func) {
    service_listener_t *listener = (service_listener_t *)malloc(sizeof(service_listener_t));
    listener->listen = func;
    dlist_add(&listener->list_head, &g_accs_listener_list);
    LOGD(MODULE_NAME_ACCS,"accs add listerner: %p\n", func);
    accs_notify_event(func, SERVICE_ATTACH);
    return SERVICE_RESULT_OK;
}

int accs_del_listener(service_cb func) {
    service_listener_t *pos;
    dlist_t *tmp = NULL;
    dlist_for_each_entry_safe(&g_accs_listener_list, tmp, pos, service_listener_t, list_head){
        if(pos->listen == func) {
            LOGD(MODULE_NAME_ACCS,"accs del listerner: %p\n", func);
            accs_notify_event(func, SERVICE_DETACH);
            dlist_del(&pos->list_head);
            free(pos);
        }
    }
    return SERVICE_RESULT_OK;
}

static int accs_set_state(int st) {
    accs_debug(MODULE_NAME_ACCS,"%s -> %s", sm_code2string(accs.state), sm_code2string(st));
    accs.state = st;
    accs_broadcast_event(accs.state);
    return SERVICE_RESULT_OK;
}

static int accs_get_state() {
    return accs.state;
}

#define ALINK_RESPONSE_OK   "{\"id\":%d,\"result\":{\"msg\":\"success\",\"code\":1000}}"
static int accs_conn_listener(int type, void *data, int dlen, void *result, int *rlen) {
     if(type == CONNECT_EVENT) {
        int st = *((int*)data);
        accs_debug(MODULE_NAME_ACCS,"ACCS recv %s, %s", cm_code2string(type), cm_code2string(st));
        if(st == CONNECT_STATE_OPEN) {
            ; //ignore connect open event
        }else if(st == CONNECT_STATE_READY) {
            accs_set_state(SERVICE_STATE_PREPARE);
            start_accs_work();
        }else if(st == CONNECT_STATE_CLOSE) {
            void *cb = alink_cb_func[_ALINK_CLOUD_DISCONNECTED];
            if (cb) {
                void (*func)(void) = cb;
                func();
            }
            accs_set_state(SERVICE_STATE_INIT);
        }
    } else if(type == CONNECT_DATA) {
        alink_data_t pack;
        if(alink_parse_data(data, dlen, &pack) != ALINK_CODE_SUCCESS) {
            LOGE(MODULE_NAME_ACCS,"ACCS recv malformed pack");
        } else {
            char *str = result;
            accs_broadcast_data(&pack, dlen, result, rlen);
            /* if result is empty, fill it with response OK */
            if (str && str[0] == '\0')
                *rlen = sprintf(str, ALINK_RESPONSE_OK, last_state.id);
        }
    }
    return 0;
}

static int accs_broadcast(int type, void *data, int dlen, void *result, int *rlen) {
    service_listener_t *pos;
    dlist_for_each_entry_reverse(pos, &g_accs_listener_list, list_head, service_listener_t){
        service_cb func = *pos->listen;
        if(func(type, data, dlen, result, rlen) == EVENT_CONSUMED) {
            accs_debug(MODULE_NAME_ACCS,"ACCS broadcast consumed by listener:%p", func);
            break;
        }
    }
    return SERVICE_RESULT_OK;
}

static int accs_notify_event(service_cb func, int evt) {
    if(func) {
        func(SERVICE_EVENT, &evt, sizeof(evt), 0, 0);
    }
    return SERVICE_RESULT_OK;
}

static int accs_broadcast_event(int evt) {
    return accs_broadcast(SERVICE_EVENT, &evt, sizeof(evt), 0, 0);
}

static int accs_notify_data(service_cb func, void *data, int dlen, void *result, int *rlen) {
    if(func) {
        func(SERVICE_DATA, data, dlen, result, rlen);
    }
    return SERVICE_RESULT_OK;
}

static int accs_broadcast_data(void *data, int dlen, void *result, int *rlen) {
    return accs_broadcast(SERVICE_DATA, data, dlen, result, rlen);
}

static void accs_handshake(void *arg)
{
    /*//tz  TODO: this code block will be removed if everything runs well for long time.
    char ssid[PLATFORM_MAX_SSID_LEN];
    memset(ssid, 0, PLATFORM_MAX_SSID_LEN);
    os_wifi_get_APInfo(ssid, NULL, NULL);
    if (strcmp(ssid, DEFAULT_SSID) == 0){
        return SERVICE_RESULT_OK;
    }
    */
    if (accs_get_state() == SERVICE_STATE_PREPARE) {
        if (alink_handshake() == SERVICE_RESULT_OK) {
            void *cb = alink_cb_func[_ALINK_CLOUD_CONNECTED];
            accs_set_state(SERVICE_STATE_READY);
            if (cb) {
                void (*func)(void) = cb;
                func();
            }
        } else {
            accs_set_state(SERVICE_STATE_PREPARE);
        }
    }
}

static int accs_event_handler(int type, void *data, int dlen, void *result, int *rlen)
{
    if (type == SERVICE_EVENT) {
        int st = *((int*)data);
        accs_debug(MODULE_NAME_ACCS,"app recv: %s, %s", sm_code2string(type), sm_code2string(st));
        return EVENT_IGNORE;
    } else if (type == SERVICE_DATA) {
        alink_data_t *p = (alink_data_t *)data;
        void *cb = NULL;
        int ret = EVENT_IGNORE;

        if (!strcmp(p->method, "getDeviceStatus")) {
            cb = alink_cb_func[_ALINK_GET_DEVICE_STATUS];
            ret = EVENT_IGNORE;//ATTR handler depend on it
            if (cb) {
                int (*func)(char *) = cb;
                func(p->data);
            }
        } else if (!strcmp(p->method, "setDeviceStatus")) {
            cb = alink_cb_func[_ALINK_SET_DEVICE_STATUS];
            ret = EVENT_IGNORE;//ATTR handler depend on it
            if (cb) {
                int (*func)(char *) = cb;
                func(p->data);
            }
        } else if (!strcmp(p->method, "getDeviceStatusByRawData")) {
            cb = alink_cb_func[_ALINK_GET_DEVICE_RAWDATA];
            ret = EVENT_CONSUMED;
            if (cb) {
                int (*func)(char *) = cb;
                func(p->data);
            }
        } else if (!strcmp(p->method, "setDeviceStatusByRawData")) {
            cb = alink_cb_func[_ALINK_SET_DEVICE_RAWDATA];
            ret = EVENT_CONSUMED;
            if (cb) {
                int (*func)(char *) = cb;
                func(p->data);
            }
        } else if (!strcmp(p->method, "setDeviceStatusArray")) {
            ret = EVENT_IGNORE;
            //TODO: setDeviceStatusArray not support
        } else
            ret = EVENT_IGNORE;

        return ret;
    }
}

int cloud_is_connected(void)
{
    return accs.state == SERVICE_STATE_READY;
}
