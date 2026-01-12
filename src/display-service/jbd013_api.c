#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "jbd013_api.h"
#include "string.h"

// 发送JBD013VGA面板的SPI指令给面板
void send_cmd(uint8_t cmd) {
    uint8_t pBuf[1];

    pBuf[0] = cmd;
    spi_tx_frame(pBuf);
}

// 读取面板ID，返回面板ID
void read_id(void) {
    uint8_t pBuf[3];

    printf("读取面板ID:\n");
    spi_rx_frame(SPI_RD_ID, pBuf, 3);
}

// 读面板唯一ID，存储在pBuf指针指向的内存空间中，指针对应的缓存空间应大于等于15字节
void read_uid(void) {
    uint8_t pBuf[15];

    printf("读面板唯一ID:\n");
    spi_rx_frame(SPI_RD_UID, pBuf, 15);
}

// 写状态寄存器，寄存器地址regAddr，写入数据data
void wr_status_reg(uint8_t regAddr, uint8_t data) {
    uint8_t pBuf[2];

    pBuf[0] = regAddr;
    pBuf[1] = data;

    spi_tx_frame(pBuf);
}

// 读状态寄存器，寄存器地址regAddr，返回寄存器数据
void rd_status_reg(uint8_t regAddr) {
    uint8_t pBuf[1];

    printf("读状态寄存器: 0x%02X\n", regAddr);
    spi_rx_frame(regAddr, pBuf, 1);
}

// 写偏移寄存器，行偏移地址row（0~31），列偏移地址col（0~31）
void wr_offset_reg(uint8_t row, uint8_t col) {
    uint8_t pBuf[3];

    pBuf[0] = 0xc0;
    pBuf[1] = row;
    pBuf[2] = col;

    spi_tx_frame(pBuf);
    send_cmd(SPI_SYNC); //发送命令，数据同步
    usleep(1 * 1000);   //1ms (8MHz) 或 0.5ms (16MHz)
}

// 读偏移寄存器，返回寄存器数据
void rd_offset_reg(void) {
    uint8_t pBuf[2];

    printf("读偏移寄存器: \n");
    spi_rx_frame(SPI_RD_OFFSET_REG, pBuf, 2);
}

// 写电流寄存器，要写入的数据（范围为0~63）param
void wr_cur_reg(uint8_t param) {
    uint8_t pBuf[2];

    pBuf[0] = 0x46;
    pBuf[1] = param;
    spi_tx_frame(pBuf);
}

// 读电流寄存器，返回寄存器数据
void rd_cur_reg(void) {
    uint8_t pBuf[1];

    printf("读电流寄存器: \n");
    spi_rx_frame(SPI_RD_CURRENT_REG, pBuf, 1);
}

// 写亮度寄存器
// [self refresh frequency，param],[25Hz,(0~21331)],[50Hz,(0~10664)],[75Hz,(0~7109)]
// [100Hz,(0~5331)],[125Hz,(0~4264)],[150Hz,(0~3366)],[175Hz,(0~2907)],[200Hz,(0~2558)]
void wr_lum_reg(uint16_t param) {
    uint8_t pBuf[3];

    pBuf[0] = 0x36;
    pBuf[1] = param >> 8;
    pBuf[2] = param;
    spi_tx_frame(pBuf);
}

// 读亮度寄存器,返回寄存器数据
void rd_lum_reg(void) {
    uint8_t pBuf[2];

    printf("读亮度寄存器: \n");
    spi_rx_frame(SPI_RD_LUM_REG, pBuf, 2);
}

// 设置镜像模式，[param = 0：正常显示], [param = 1：仅左右镜像]
// [param = 2：只镜像上下], [param = 3：同时镜像上、下、左、右]
void set_mirror_mode(uint8_t param) {
    send_cmd(SPI_DISPLAY_DEFAULT_MODE);
    if (param == 1 || param == 3) {
        send_cmd(SPI_DISPLAY_RL);
    }
    if (param == 2 || param == 3) {
        send_cmd(SPI_DISPLAY_UD);
    }
    send_cmd(SPI_SYNC);
    usleep(1 * 1000);
}

// 清空缓存
void clr_cache(void) {
    uint8_t pBuf[10];
    uint32_t pBufLen = sizeof(pBuf);
    uint8_t addrStep = pBufLen * 2;
    uint16_t rowCnt, colCnt;

    memset(pBuf, 0, pBufLen);
    spi_wr_buffer(0,0,pBuf,pBufLen);
     for (rowCnt = 0; rowCnt < 480; rowCnt++) {
         for (colCnt = 0; colCnt < 640; colCnt += addrStep) {
             spi_wr_buffer(colCnt, rowCnt, pBuf, pBufLen);
         }
        if (640 % addrStep != 0) {
             spi_wr_buffer((640 - 640 % addrStep), rowCnt, pBuf, 640 % addrStep);
         }
     }
}

// 显示图像，指向图像数据的指针pBuf，图片数据的长度len（0~153600）
void display_image(uint16_t row, uint16_t col, uint8_t *pBuf, uint32_t len) {
    spi_wr_buffer(col, row, pBuf, len);
    send_cmd(SPI_SYNC);            //同步缓存数据
    usleep(1 * 1000);              //1ms (8MHz) 或 0.5ms (16MHz)
}

/**
 * @brief 显示图像数据（优化版）
 * @param row 起始行
 * @param col 起始列
 * @param pBuf 图像数据缓冲区
 * @param len 数据长度
 * @param sync 是否立即同步
 */
void display_image_sync(uint16_t row, uint16_t col, uint8_t *pBuf, uint32_t len, uint8_t sync) {
    spi_wr_buffer(col, row, pBuf, len);
    if (sync) {
        send_cmd(SPI_SYNC);            // 同步缓存数据
        usleep(1 * 1000);             // 1ms (8MHz) 或 0.5ms (16MHz)
    }
}

// 复位面板
void panel_rst(void) {
    send_cmd(SPI_RST_EN);
    send_cmd(SPI_RST);
    usleep(50 * 1000);
}

// 初始化面板
void panel_init(void) {
    panel_rst();                            //复位面板
    send_cmd(SPI_WR_ENABLE);                //写入使能
    wr_cur_reg(30);                          //设置电流寄存器
    wr_status_reg(SPI_WR_STATUS_REG1, 0x10);//写状态寄存器1，关闭demura
    wr_lum_reg(3000);                       //写亮度寄存器
    wr_status_reg(SPI_WR_STATUS_REG2, 0x05);//写状态寄存器2
    clr_cache();                            //清除缓存
    wr_offset_reg(0, 0);                    //设置左上角偏移量
    wr_offset_reg(0, 20);                   //设置右上角的偏移量
    wr_offset_reg(24, 0);                   //设置左下角的偏移量
    wr_offset_reg(24, 20);                  //设置右下角的偏移量
    wr_offset_reg(12, 10);                  //设置实际偏移量，屏幕居中
    wr_lum_reg(3000);                       //写亮度寄存器原本1000
    wr_cur_reg(30);                          //设置电流寄存器
    set_mirror_mode(1);                     //默认镜像模式
    send_cmd(SPI_DISPLAY_ENABLE);           //设置显示启用
    send_cmd(SPI_SYNC);                     //同步设置
    usleep(1 * 1000);
}