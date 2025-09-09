#!/bin/sh

# 设置日志文件路径（与参考代码保持一致）
LOG_FILE=/tmp/glass_boot.log
export LD_LIBRARY_PATH=/oem/usr/lib:$LD_LIBRARY_PATH
./oem/usr/bin/rkaiq_3A_server  >> "$LOG_FILE" 2>&1 & #启动相机服务

sleep 3
display &

launch &
./rtkhci -n -s 115200 /dev/ttyS2 rtk_h5 &
