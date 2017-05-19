#ifndef _key_value_h_
#define _key_value_h

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C"
{
#endif

#define HASH_TABLE_MAX_SIZE 1024
#define MAX_KV_LEN 256

#ifndef CONFIG_YOS_KVFILE
#define KVFILE_NAME "/tmp/KVfile"
#else
#define KVFILE_NAME CONFIG_YOS_KVFILE
#endif

#ifndef CONFIG_YOS_KVFILE_BACKUP
#define KVFILE_NAME_BACKUP "/tmp/KVfile_backup"
#else
#define KVFILE_NAME_BACKUP CONFIG_YOS_KVFILE_BACKUP
#endif

/**
 * @brief init the kv module.
 *
 * @param[in] none.
 *
 * @note: the default KV size is @HASH_TABLE_MAX_SIZE, the path to store
 *        the kv file is @KVFILE_PATH.
 * @retval  0 on success, otherwise -1 will be returned
 */
int yos_kv_init();

/**
 * @brief deinit the kv module.
 *
 * @param[in] none.
 *
 * @note: all the KV in RAM will be released.
 * @retval none. 
 */
void yos_kv_deinit();

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif
