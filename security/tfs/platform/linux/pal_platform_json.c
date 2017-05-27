#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "log.h"

#define TAG_PAL_JSON "PAL_JSON"

int pal_json_get_string_value(char *json_str, const char **tokens, int tokens_size, char *value) {
    cJSON *json;
    cJSON *root_json;
    cJSON *json_item;
    int i = 0;

    if (json_str == NULL || tokens == NULL || tokens_size <= 0 || value == NULL) {
        LOGE(TAG_PAL_JSON, "para error\n");
        return -1;
    }

	LOGD(TAG_PAL_JSON, "parse json.\n");
    json = cJSON_Parse(json_str);
    root_json = json;

	LOGD(TAG_PAL_JSON, "search value.\n");
	if(root_json == NULL) {
	    LOGD(TAG_PAL_JSON, "json is null");
		return -1;
	}

    for (; i < tokens_size; i ++) {
        json_item = cJSON_GetObjectItem(json, tokens[i]);
        if ((json_item == NULL) || ((i < tokens_size - 1) && (json_item->type != cJSON_Object))) {
            break;
        }
        json = json_item;
    }

	LOGD(TAG_PAL_JSON, "search value complete.\n");
    if (i != tokens_size) {
        LOGE(TAG_PAL_JSON, "get value error\n");
        cJSON_Delete(json);
        return -1;
    }

	LOGD(TAG_PAL_JSON, "check value type.\n");
    if (json_item->type != cJSON_String) {
        LOGE(TAG_PAL_JSON, "get value type error\n");
        cJSON_Delete(json);
        return -1;
    }

	LOGD(TAG_PAL_JSON, "copy value.\n");
    strncpy(value, json_item->valuestring, strlen(json_item->valuestring));
    cJSON_Delete(root_json);

    return 0;
}

int pal_json_get_number_value(char *json_str, const char **tokens, int tokens_size, int *value) {
    cJSON *json;
    cJSON *root_json;
    cJSON *json_item;
    int i = 0;

    if (json_str == NULL || tokens == NULL || tokens_size <= 0 || value == NULL) {
        LOGE(TAG_PAL_JSON, "para error\n");
        return -1;
    }

    json = cJSON_Parse(json_str);
    root_json = json;

	if(root_json == NULL) {
	    LOGD(TAG_PAL_JSON, "json is null");
		return -1;
	}

    for (; i < tokens_size; i ++) {
        json_item = cJSON_GetObjectItem(json, tokens[i]);
        if ((json_item == NULL) || ((i < tokens_size - 1) && (json_item->type != cJSON_Object))) {
            break;
        }
        json = json_item;
    }

    if (i != tokens_size) {
        LOGE(TAG_PAL_JSON, "get value error\n");
        cJSON_Delete(json);
        return -1;
    }

    if (json_item->type != cJSON_Number) {
        LOGE(TAG_PAL_JSON, "get value type error\n");
        cJSON_Delete(json);
        return -1;
    }

    *value = json_item->valueint;
    cJSON_Delete(root_json);

    return 0;
}
