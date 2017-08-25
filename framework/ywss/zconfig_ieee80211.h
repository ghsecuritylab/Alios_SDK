/*
 * Copyright (C) 2017 YunOS Project. All rights reserved.
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

#ifndef __IEEE80211_H
#define __IEEE80211_H

#include "zconfig_utils.h"
#include "zconfig_protocol.h"

#ifndef ETH_ALEN
#define ETH_ALEN    6
#endif

#define __le16  unsigned short

enum ALINK_TYPE {
    ALINK_INVALID = 0,
    ALINK_BROADCAST = 1,
    ALINK_ROUTER = 2,
    ALINK_ACTION = 3,
    ALINK_WPS = 4,
    ALINK_DEFAULT_SSID = 5,
    ALINK_ZERO_CONFIG = 6
};

/* 80211 frame parser result */
struct parser_res {
    union _alink_type_ {
        /* for broadcast data frame */
        struct broadcast_info {
            u8 encry_type;/* none/wep/tkip/aes */
            u16 data_len;/* framelen - 80211 hdr - fcs(4) */
            u16 sn;
        } br;
        /* for alink ie frame */
        struct ie_info {
            u8 *alink_ie;
            u16 alink_ie_len;
        } ie;
        /* for p2p action frame */
        struct action_info {
            u8 *data;
            u16 data_len;
        } action;
        /* for p2p wps frame */
        struct wps_info {
            u8 *data;
            u16 data_len;
        } wps;
    } u;

    u8 *src; /* src mac of sender */
    u8 *dst; /* ff:ff:ff:ff:ff:ff */
    u8 *bssid; /* mac of AP */

    u8 tods; /* fromDs or toDs */
    u8 channel; /* 1 - 13 */
};

/* ap list */
#define MAX_APLIST_NUM              (100)
extern struct ap_info *zconfig_aplist;
extern u8 zconfig_aplist_num;

int ieee80211_data_extract(u8 *in, int len, int link_type,
                           struct parser_res *res);

struct ap_info *zconfig_get_apinfo(u8 *mac);
struct ap_info *zconfig_get_apinfo_by_ssid(u8 *ssid);
struct ap_info *zconfig_get_apinfo_by_ssid_prefix(u8 *ssid_prefix);
struct ap_info *zconfig_get_apinfo_by_ssid_suffix(u8 *ssid_suffix);

/* add channel to scanning channel list */
int zconfig_add_active_channel(int channel);

#endif /* __IEEE80211_H */
