/**
 * HTTP客户端使用示例
 *
 * 编译:
 * gcc -o http_example http_example.c ../src/http_client.c ../third_party/mongoose/mongoose.c \
 *     -I../include -I../third_party/mongoose -lpthread
 */

#include "http_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 示例1: 简单的HTTP GET请求
void example_http_get() {
    printf("\n=== 示例1: HTTP GET请求 ===\n");

    http_response_t resp;
    int ret = http_get("http://httpbin.org/get", &resp);

    if (ret == 0) {
        printf("✅ 请求成功\n");
        printf("状态码: %d\n", resp.status_code);
        printf("响应体:\n%.*s\n", (int)resp.body_len, resp.body);
        http_response_free(&resp);
    } else {
        printf("❌ 请求失败\n");
    }
}

// 示例2: HTTP POST JSON请求
void example_http_post_json() {
    printf("\n=== 示例2: HTTP POST JSON请求 ===\n");

    const char *json = "{\"text\":\"你好世界\",\"voice\":\"xiaoyun\"}";
    http_response_t resp;

    int ret = http_post_json("http://httpbin.org/post", json, &resp);

    if (ret == 0) {
        printf("✅ 请求成功\n");
        printf("状态码: %d\n", resp.status_code);
        printf("响应体:\n%.*s\n", (int)resp.body_len, resp.body);
        http_response_free(&resp);
    } else {
        printf("❌ 请求失败\n");
    }
}

// 示例3: 自定义HTTP请求（带自定义头）
void example_http_custom() {
    printf("\n=== 示例3: 自定义HTTP请求 ===\n");

    http_request_t req = {
        .url = "http://httpbin.org/headers",
        .method = HTTP_GET,
        .headers = "User-Agent: AI-Core/1.0\r\nX-Custom-Header: test\r\n",
        .timeout_ms = 10000  // 10秒超时
    };

    http_response_t resp;
    int ret = http_request(&req, &resp);

    if (ret == 0) {
        printf("✅ 请求成功\n");
        printf("状态码: %d\n", resp.status_code);
        printf("响应体:\n%.*s\n", (int)resp.body_len, resp.body);
        http_response_free(&resp);
    } else {
        printf("❌ 请求失败\n");
    }
}

void print_usage(const char *program) {
    printf("用法: %s [URL]\n", program);
    printf("\n");
    printf("参数:\n");
    printf("  URL    要请求的HTTP地址（可选）\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s http://httpbin.org/get\n", program);
    printf("  %s https://www.baidu.com\n", program);
    printf("  %s http://api.example.com/status\n", program);
    printf("  %s                              # 运行内置示例\n", program);
    printf("\n");
}

int main(int argc, char *argv[]) {
    printf("HTTP客户端使用示例\n");
    printf("===================\n");

    // 如果提供了命令行参数，请求指定的URL
    if (argc >= 2) {
        const char *url = argv[1];

        // 检查是否是帮助命令
        if (strcmp(url, "-h") == 0 || strcmp(url, "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }

        printf("\n正在请求: %s\n", url);
        printf("====================\n");

        http_response_t resp;
        int ret = http_get(url, &resp);

        if (ret == 0) {
            printf("\n✅ 请求成功\n");
            printf("状态码: %d\n", resp.status_code);
            printf("响应长度: %zu bytes\n", resp.body_len);
            printf("\n响应体:\n");
            printf("----------------------------------------\n");
            printf("%.*s\n", (int)resp.body_len, resp.body);
            printf("----------------------------------------\n");
            http_response_free(&resp);
        } else {
            printf("\n❌ 请求失败\n");
            printf("请检查:\n");
            printf("  - URL格式是否正确\n");
            printf("  - 服务器是否可访问\n");
            printf("  - 网络连接是否正常\n");
            return 1;
        }
    } else {
        // 没有参数，运行默认示例
        printf("\n提示: 可以使用 %s <URL> 来请求任意地址\n", argv[0]);
        printf("      或使用 %s --help 查看帮助\n\n", argv[0]);

        // 运行内置示例
        example_http_get();
        example_http_post_json();
        example_http_custom();
    }

    printf("\n✅ 程序执行完成\n");
    return 0;
}
