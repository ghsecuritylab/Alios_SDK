/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef UMESH_TYPES_H
#define UMESH_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "hal/base.h"
#include "yos/yos.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MESH_VERSION_1 = 1,
};

typedef enum ur_error_s {
    UR_ERROR_NONE          = 0,
    UR_ERROR_FAIL          = 1,
    UR_ERROR_BUSY          = 2,
    UR_ERROR_DROP          = 3,
    UR_ERROR_MEM           = 4,
    UR_ERROR_ROUTE         = 5,
    UR_ERROR_PARSE         = 6,
    UR_ERROR_ADDRESS_QUERY = 7,
}
ur_error_t;

typedef enum media_type_s {
    MEDIA_TYPE_WIFI = 0,
    MEDIA_TYPE_BLE  = 1,
    MEDIA_TYPE_15_4 = 2,
} media_type_t;

#ifndef NULL
#define NULL (void *)0
#endif

enum {
    UR_IP6_ADDR_SIZE   = 16,

    IP6_UCAST_ADDR_NUM = 2,
    IP6_MCAST_ADDR_NUM = 1,

    MESH_IP4_ADDR_SIZE = 4,
};

enum {
    SHORT_ADDR_SIZE = 2,
    EXT_ADDR_SIZE   = 8,
    EXT_NETID_SIZE = 6,
};

typedef struct ur_ip6_addr_s {
    union {
        uint8_t  m8[UR_IP6_ADDR_SIZE];
        uint16_t m16[UR_IP6_ADDR_SIZE / sizeof(uint16_t)];
        uint32_t m32[UR_IP6_ADDR_SIZE / sizeof(uint32_t)];
    };
} __attribute__((packed)) ur_ip6_addr_t;

typedef struct mesh_ip4_addr_s {
    union {
        uint8_t  m8[MESH_IP4_ADDR_SIZE];
        uint16_t m16[MESH_IP4_ADDR_SIZE / sizeof(uint16_t)];
        uint32_t m32;
    };
} __attribute__((packed)) mesh_ip4_addr_t;

typedef struct ur_ip6_prefix_s {
    ur_ip6_addr_t prefix;
    uint8_t       length;
} __attribute__((packed)) ur_ip6_prefix_t;

enum {
    UR_IP6_HLEN      = 40,
    MESH_IP4_HLEN = 20,
    UR_UDP_HLEN      = 8,
};

typedef struct ur_netif_ip6_address_s {
    union {
        ur_ip6_addr_t ip6_addr;
        mesh_ip4_addr_t ip4_addr;
    } addr;
    uint8_t                       prefix_length;
    struct ur_netif_ip6_address_s *next;
} ur_netif_ip6_address_t;

typedef struct mac_address_s {
    union {
        uint64_t value;
        uint16_t short_addr;
        uint8_t  addr[EXT_ADDR_SIZE];
    };
    uint8_t len;
} mac_address_t;

typedef struct ur_addr_s {
    mac_address_t addr;
    uint16_t netid;
} ur_addr_t;

typedef struct umesh_extnetid_s {
    uint8_t netid[EXT_NETID_SIZE];
    uint8_t len;
} umesh_extnetid_t;

enum {
    KEY_SIZE = 16,  // bytes

    INVALID_KEY_INDEX = 0xff,
    ONE_TIME_KEY_INDEX  = 0,
    GROUP_KEY1_INDEX  = 1,
};

typedef struct mesh_key_s {
    uint8_t len;
    uint8_t key[KEY_SIZE];
} mesh_key_t;

typedef struct frame_s {
    uint8_t  *data;
    uint16_t len;
    uint8_t key_index;
} frame_t;

typedef struct frame_info_s {
    mac_address_t peer;
    uint8_t       channel;
    int8_t        rssi;
    int8_t        key_index;
} frame_info_t;

typedef struct channel_s {
    uint16_t channel;
    uint16_t wifi_channel;
    uint16_t hal_ucast_channel;
    uint16_t hal_bcast_channel;
} channel_t;

typedef ur_error_t (* umesh_raw_data_received)(ur_addr_t *src,
                                               uint8_t *payload,
                                               uint8_t length);

typedef struct frame_stats_s {
    uint32_t in_frames;
    uint32_t out_frames;
} frame_stats_t;

typedef struct ur_link_stats_s {
    uint32_t in_frames;
    uint32_t in_command;
    uint32_t in_data;
    uint32_t in_filterings;
    uint32_t in_drops;

    uint32_t out_frames;
    uint32_t out_command;
    uint32_t out_data;
    uint32_t out_errors;

    uint16_t send_queue_size;
    uint16_t recv_queue_size;

    bool     sending;
    uint16_t sending_timeouts;
} ur_link_stats_t;

enum {
    UMESH_1 = 0,  // 0
    MESH_FORWARDER_1,  // 1
    MESH_FORWARDER_2,  // 2
    MESH_FORWARDER_3,  // 3
    MESH_MGMT_1,  // 4
    MESH_MGMT_2,  // 5
    MESH_MGMT_3,  // 6
    MESH_MGMT_4,  // 7
    MESH_MGMT_5,  // 8
    MESH_MGMT_6,  // 9
    MESH_MGMT_7,  // 10
    ADDRESS_MGMT_1,  // 11
    ADDRESS_MGMT_2,  // 12
    ADDRESS_MGMT_3,  // 13
    ADDRESS_MGMT_4,  // 14
    NETWORK_MGMT_1,  // 15
    NETWORK_MGMT_2,  // 16
    LOWPAN6_2,  // 18
    LINK_MGMT_1,  // 19
    LINK_MGMT_2,  // 20
    LINK_MGMT_3,  // 21
    ROUTER_MGR_1,  // 22
    DIAGS_1,  // 23
    DIAGS_2,  // 24
    UT_MSG,  // 25
    UMESH_2,
    MSG_DEBUG_INFO_SIZE,
};

typedef struct ur_message_stats_s {
    int16_t num;
    int16_t queue_fulls;
    int16_t mem_fails;
    int16_t pbuf_fails;
    int16_t size;

    int16_t debug_info[MSG_DEBUG_INFO_SIZE];
} ur_message_stats_t;

typedef struct ur_mem_stats_s {
    int32_t num;
} ur_mem_stats_t;

enum {
    WHITELIST_ENTRY_NUM = 16,
};

typedef enum node_mode_s {
    MODE_NONE   = 0x00,  // this is for testing that not joining net
    MODE_MOBILE = 0x01,
    MODE_LOW_MASK = 0x0f,
    MODE_RX_ON  = 0x10,
    MODE_SUPER  = 0x20,
    MODE_LEADER = 0x40,
    MODE_HI_MASK = 0xf0,
} node_mode_t;

typedef struct whitelist_entry_s {
    mac_address_t address;
    int8_t        rssi;
    bool          valid;
    bool          constant_rssi;
} whitelist_entry_t;

#ifdef __cplusplus
}
#endif

#endif  /* UMESH_TYPES_H */
