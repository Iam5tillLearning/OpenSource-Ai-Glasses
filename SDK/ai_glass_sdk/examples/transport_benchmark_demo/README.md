# 传输测速 Demo

本 demo 用于测试眼镜端把一段约 `200KB` 的内存数据传到手机端的耗时。

特点：

1. 不拍照、不落文件，眼镜端直接生成固定大小的内存 payload。
2. 保留两种数据通道：
   - 经典蓝牙 `SPP`
   - `Wi-Fi TCP`
3. Android 端负责触发测试、接收 payload、校验 `crc32`、显示耗时。
4. Wi-Fi 测试通过 SPP 发送控制命令，真正的 payload 数据只走 Wi-Fi。

## 目录

```text
transport_benchmark_demo/
  glasses/
    transport_benchmark_demo.c
    Makefile
  clients/
    android/
```

## 眼镜端

构建：

```bash
cd examples/transport_benchmark_demo/glasses
make
```

运行前确认：

```bash
ls -l /var/run/ai_spp.sock
```

运行：

```bash
../../build/transport_benchmark_demo
```

## Android 端

构建：

```bash
cd examples/transport_benchmark_demo/clients/android
bash build_android.sh
```

运行步骤：

1. 授予蓝牙权限。
2. 点击 `Connect SPP` 连接眼镜。
3. 点击 `Prepare Wi-Fi` 让手机端监听一个 TCP 端口。
4. 点击 `Run SPP Transfer` 或 `Run Wi-Fi Transfer`。
5. 查看页面上的：
   - payload 大小
   - `crc32`
   - 控制链路耗时
   - payload 接收耗时
   - 端到端耗时
6. 若 SPP 总耗时超过 `100ms`，优先看 `prep_ms`、`data_ms`、`ack_ms` 三段。

## 协议摘要

SPP 控制命令：

```text
SPP_SEND request=<request_id> size=<bytes>\n
WIFI_SEND request=<request_id> host=<ipv4> port=<port> size=<bytes>\n
```

数据头：

```text
OSAIG_BENCH_V1 channel=<spp|wifi> request=<request_id> size=<bytes> crc32=<hex>\n
<exactly size bytes payload>
```

ACK：

```text
OSAIG_BENCH_ACK request=<request_id> status=ok recv_ms=<ms> total_ms=<ms> bytes=<bytes> crc32=<hex>\n
```

结果回报：

```text
OSAIG_BENCH_DONE request=<request_id> channel=<spp|wifi> status=<ok|error> total_ms=<ms> prep_ms=<ms> connect_ms=<ms> data_ms=<ms> ack_ms=<ms> bytes=<bytes> crc32=<hex> reason=<token>\n
```
