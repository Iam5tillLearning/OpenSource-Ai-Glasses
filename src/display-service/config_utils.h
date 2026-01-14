#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

// 默认配置文件路径
#define CONFIG_FILE_PATH "/etc/ai_glasses_system.conf"

// 获取整数配置
// key: 配置项名称
// default_value: 如果未找到或出错，返回此默认值
int config_get_int(const char *key, int default_value);

// 设置整数配置（会自动保存到文件）
// key: 配置项名称
// value: 要设置的值
void config_set_int(const char *key, int value);

#endif // CONFIG_UTILS_H
