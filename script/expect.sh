#!/bin/sh
LOG_FILE=/tmp/glass_boot.log

echo "[glassinit] 👓 Glass boot init START" >> "$LOG_FILE"

hciconfig hci0 up
hciconfig hci0 name 'xiaoai'

# 启动 dbus
mkdir -p /var/run/dbus
export DBUS_SYSTEM_BUS_ADDRESS='unix:path=/var/run/dbus/system_bus_socket'
dbus-daemon --fork --config-file=/usr/share/dbus-1/system.conf >> "$LOG_FILE" 2>&1

# 等待 dbus 启动
sleep 1

# 启动 Bluetooth 服务（不建议加 &，或用更安全方式等待）
bluetoothd -n >> "$LOG_FILE" 2>&1 &
bluealsa -p a2dp-sink >> "$LOG_FILE" 2>&1 &
bluealsa-aplay >> "$LOG_FILE" 2>&1 &

sleep 1


# 开启 LE 广播
hciconfig hci0 leadv

# 启动自定义 BLE 服务（如果你确定是后台服务，可以加 &）
btgatt-server >> "$LOG_FILE" 2>&1 &

# 启动 expect 脚本并确保完整日志记录
{
/usr/bin/expect << 'EOF'
#!/usr/bin/expect
set timeout -1

# 记录启动信息
puts "=== Expect 蓝牙控制脚本启动 ==="
puts "开始 spawn bluetoothctl..."

spawn bluetoothctl
expect "#"  # 等待 bluetoothctl 提示符

puts "设置可发现模式..."
send "discoverable on\r"
expect "Changing discoverable on succeeded"

puts "开始监听蓝牙事件..."
# 持续监听事件
expect {
    -ex "Confirm passkey" {
        puts "检测到配对确认请求"
        send "yes\r"
        puts "✅ Auto-confirmed passkey"
        exp_continue
    }

    -ex "Authorize service" {
        puts "检测到服务授权请求"
        send "yes\r"
        puts "✅ Auto-authorized service"
        exp_continue
    }

    "Connected: yes" {
        puts "🎉 Device connected!"
        exp_continue
    }

    "Device *" {
        puts "发现设备: $expect_out(0,string)"
        exp_continue
    }

    eof {
        puts "🔚 Session ended"
        exit 0
    }

    timeout {
        puts "等待超时，继续监听..."
        exp_continue
    }

    default {
        puts "收到未知消息: $expect_out(buffer)"
        exp_continue
    }
}
EOF
} >> "$LOG_FILE" 2>&1 &

echo "蓝牙服务已启动，查看详细日志: $LOG_FILE"
echo "[glassinit] ✅ All services launched" >> "$LOG_FILE"
