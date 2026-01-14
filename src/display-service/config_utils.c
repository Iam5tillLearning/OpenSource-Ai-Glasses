#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "cJSON.h"
#include "config_utils.h"

// 读取整个文件内容
static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = (char *)malloc(length + 1);
    if (content) {
        fread(content, 1, length, f);
        content[length] = '\0';
    }
    fclose(f);
    return content;
}

// 将内容写入文件
static int write_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    fprintf(f, "%s", content);
    fclose(f);
    return 0;
}

int config_get_int(const char *key, int default_value) {
    char *content = read_file(CONFIG_FILE_PATH);
    if (!content) return default_value;

    int ret = default_value;
    cJSON *json = cJSON_Parse(content);
    if (json) {
        cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
        if (cJSON_IsNumber(item)) {
            ret = item->valueint;
        }
        cJSON_Delete(json);
    }

    free(content);
    return ret;
}

void config_set_int(const char *key, int value) {
    char *content = read_file(CONFIG_FILE_PATH);
    cJSON *json = NULL;

    if (content) {
        json = cJSON_Parse(content);
        free(content);
    }

    // 如果解析失败或文件不存在，创建新对象
    if (!json) {
        json = cJSON_CreateObject();
    }

    // 更新或添加字段
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (item) {
        cJSON_SetNumberValue(item, value);
    } else {
        cJSON_AddNumberToObject(json, key, value);
    }

    // 生成 JSON 字符串并写入
    char *new_content = cJSON_Print(json);
    if (new_content) {
        write_file(CONFIG_FILE_PATH, new_content);
        free(new_content); // cJSON_Print returns mallocated string
    }

    cJSON_Delete(json);
}
