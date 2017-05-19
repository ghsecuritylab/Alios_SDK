/*
 *  Copyright (C) 2015 YunOS Project. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "log.h"
#include "pal.h"
#include "emu_aes.h"

#if defined(TFS_OPENSSL)
#include "openssl/aes.h"
#endif

#define TAG_EMU_AES "TFS_EMU_AES"

int32_t emu_aes128_cbc_enc(const uint8_t *key,
                           const uint8_t *iv,
                           int32_t input_len,
                           const uint8_t *input,
                           uint8_t *output)
{
#if defined(TFS_OPENSSL)
    AES_KEY aeskey;
    char key128[16];

    LOGD(TAG_EMU_AES, "%s: enter\n", __func__);
    memcpy(key128, key, 16);

    if (AES_set_encrypt_key((unsigned char*)key128, 128, &aeskey) < 0) {
        LOGE(TAG_EMU_AES, "aes128_set key failed\n");
        return -1;
    }

    AES_cbc_encrypt((unsigned char*)input, (unsigned char*)output,
                    input_len, &aeskey, (unsigned char *)iv, AES_ENCRYPT);
    LOGD(TAG_EMU_AES, "aes128_cbc enc\n");

    return 0;
#else
    LOGE(TAG_EMU_AES, "aes128_cbc enc no implement!\n");
    return -1;
#endif
}

int32_t emu_aes128_cbc_dec(const uint8_t *key,
                           const uint8_t *iv,
                           int32_t input_len,
                           const uint8_t *input,
                           uint8_t *output)
{
#if defined(TFS_OPENSSL)
    AES_KEY aeskey;
    char key128[16];

    LOGD(TAG_EMU_AES, "%s: enter\n", __func__);
    memcpy(key128, key, 16);

    if (AES_set_decrypt_key((unsigned char*)key128, 128, &aeskey) < 0) {
        LOGE(TAG_EMU_AES, "aes128_set key failed\n");
        return -1;
    }

    AES_cbc_encrypt(input, output,
                    input_len, &aeskey, (unsigned char *)iv, AES_DECRYPT);

    LOGD(TAG_EMU_AES, "aes128_cbc dec\n");

    return 0;
#else
    LOGE(TAG_EMU_AES, "aes128_cbc dec no implement!\n");
    return -1;
#endif
}
