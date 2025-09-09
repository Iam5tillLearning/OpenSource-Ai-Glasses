cmake_minimum_required(VERSION 3.10)
project(FFlaunch)

# 设置ARM交叉编译器
set(CMAKE_C_COMPILER /home/xjy/1106b/ttys2+blooth_but_erofs/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-gcc)
set(CMAKE_CXX_COMPILER /home/xjy/1106b/ttys2+blooth_but_erofs/rv1106b_rv1103b_linux_ipc_v1.0.0_20241016/tools/linux/toolchain/arm-rockchip831-linux-uclibcgnueabihf/bin/arm-rockchip831-linux-uclibcgnueabihf-g++)

add_executable(FFlaunch launch.cpp)

