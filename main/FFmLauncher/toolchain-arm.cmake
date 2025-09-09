# toolchain-arm.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER /home/xjy/1106b/ttys2+blooth_but_erofs/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-cc)

# 如果需要 sysroot
# set(CMAKE_SYSROOT /opt/sysroots/arm-rootfs)

# 如果需要 C++，加上
set(CMAKE_CXX_COMPILER /home/xjy/1106b/ttys2+blooth_but_erofs/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-g++)