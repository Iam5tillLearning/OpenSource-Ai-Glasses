#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "hal_driver.h"

extern int spi_file; // 声明外部变量

// 发送一帧数据
int spi_tx_frame(uint8_t* param) {
    struct spi_ioc_transfer transfer[1];
    memset(transfer, 0, sizeof(transfer));

    transfer[0].tx_buf = (unsigned long)param;
    transfer[0].len = sizeof(param);

    if (ioctl(spi_file, SPI_IOC_MESSAGE(1), transfer) < 0) {
        perror("Failed to perform SPI transfer");
        close(spi_file);
        return -1;
    }

    return 0;
}

// 接收一帧数据
int spi_rx_frame(uint8_t cmd, uint8_t* param, uint32_t len) {
    struct spi_ioc_transfer transfer[2];
    uint8_t buf[len + 1];
    memset(buf, 0, sizeof(buf));
    memset(transfer, 0, sizeof(transfer));

    buf[0] = cmd;
    transfer[0].tx_buf = (unsigned long)buf;
    transfer[0].len = 1;

    transfer[1].rx_buf = (unsigned long)(buf + 1);
    transfer[1].len = len;

    if (ioctl(spi_file, SPI_IOC_MESSAGE(2), transfer) < 0) {
        perror("Failed to perform SPI transfer");
        close(spi_file);
        return -1;
    }

    printf("接收字节%d个: ", len);
    for (uint32_t i = 1; i <= len; ++i) {
        printf("0x%02X, ", buf[i]);
    }
    printf("\n*******************\n");

    return 0;
}

// 读缓存数据,列地址col（0~639）,行地址row（0~479）,存储指针*pBuf,读取数据长度len（Max153600）
int spi_rd_buffer(uint16_t row, uint16_t col, uint32_t len) {
    uint32_t addr = ((row & 0x1ff) << 10) | (col & 0x3ff);
    struct spi_ioc_transfer transfer[2];
    uint8_t buf[len + 5];
    memset(buf, 0, sizeof(buf));
    memset(transfer, 0, sizeof(transfer));

    buf[0] = 0x03;
    buf[1] = (uint8_t)(addr >> 16);
    buf[2] = (uint8_t)(addr >> 8);
    buf[3] = (uint8_t)(addr);
    buf[4] = 0xFF;

    transfer[0].tx_buf = (unsigned long)buf;
    transfer[0].len = 5;

    transfer[1].rx_buf = (unsigned long)(buf + 5);
    transfer[1].len = len;

    if (ioctl(spi_file, SPI_IOC_MESSAGE(2), transfer) < 0) {
        perror("Failed to perform SPI transfer");
        close(spi_file);
        return -1;
    }

    printf("读缓冲区%d字节: \n", len);
    for (uint32_t i = 5; i <= len + 4; ++i) {
        printf("0x%02X, ", buf[i]);
    }
    printf("\n*******************\n");

    return 0;
}

// 写缓存数据,列地址col（0~639）,行地址row（0~479）,存储指针*pBuf,读取数据长度len（Max153600）
// 优化版本：支持大数据分块传输，避免栈溢出
int spi_wr_buffer(uint16_t col, uint16_t row, uint8_t* pBuf, uint32_t len) {
    const uint32_t MAX_CHUNK_SIZE = 4090;  // 每块最大数据大小
    uint32_t remaining = len;
    uint32_t offset = 0;
    uint16_t current_row = row;
    uint16_t current_col = col;
    
    while (remaining > 0) {
        // 计算当前块的大小
        uint32_t chunk_size = (remaining > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining;
        
        // 分配固定大小的缓冲区（避免栈溢出）
        uint8_t buf[4096];  // 4090 + 6字节的固定缓冲区
        struct spi_ioc_transfer transfer[1];
        
        memset(buf, 0, sizeof(buf));
        memset(transfer, 0, sizeof(transfer));
        
        // 计算当前块的地址
        uint32_t addr = ((current_row & 0x1ff) << 10) | (current_col & 0x3ff);

        buf[0] = 0x02;                             // 写缓存指令
        buf[1] = (uint8_t)(addr >> 16);            // 24位地址
        buf[2] = (uint8_t)(addr >> 8);
        buf[3] = (uint8_t)(addr);
        buf[4] = 0xFF;                             // Dummy
        
        // 复制当前块的数据
        memcpy(buf + 5, pBuf + offset, chunk_size);
        buf[chunk_size + 5] = 0x0F;               // Dummy

        transfer[0].tx_buf = (unsigned long)buf;
        transfer[0].len = chunk_size + 6;

        if (ioctl(spi_file, SPI_IOC_MESSAGE(1), transfer) < 0) {
            perror("Failed to perform SPI transfer");
            close(spi_file);
            return -1;
        }

        // 更新位置和剩余数据
        remaining -= chunk_size;
        offset += chunk_size;
        
        // 计算下一块的位置（考虑跨行）
        uint32_t pixels_sent = chunk_size * 2;  // 每字节2像素
        current_col += pixels_sent;
        
        // 处理跨行情况
        while (current_col >= 640) {
            current_col -= 640;
            current_row++;
        }
    }

    return 0;
}

// 读温度传感器数据，返回温度（℃）,温度传感器的ID sensorId（0~3）
float get_temperature_sensor_data(void) {
    uint8_t buf[2004];
    uint8_t maskBuf[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
    uint8_t tmpNum, isFlag;
    uint16_t tmpVal = 0, i;
    int bitCnt, rxBitCnt = -2;
    struct spi_ioc_transfer transfer[2];

    memset(buf, 0, sizeof(buf));
    memset(transfer, 0, sizeof(transfer));

    buf[0] = SPI_RD_TEMP_SENSOR;
    buf[1] = 0x02;                    // sensorId
    buf[2] = 0x00;                    // dummy data
    buf[3] = 0x00;                    // dummy data

    transfer[0].tx_buf = (unsigned long)buf;
    transfer[0].len = 4;

    transfer[1].rx_buf = (unsigned long)(buf + 4);
    transfer[1].len = 2000;

    if (ioctl(spi_file, SPI_IOC_MESSAGE(2), transfer) < 0) {
        perror("Failed to perform SPI transfer");
        close(spi_file);
        return -1;
    }

    //解析数据
    for (i = 4, isFlag = 0; i < 2004; i++) {
        if (buf[i] > 0) {
            tmpNum = buf[i];

            for (bitCnt = 7; bitCnt >= 0; bitCnt--) {
                if (rxBitCnt == -2) {
                    if (isFlag == 0 && ((tmpNum & maskBuf[bitCnt]) >= 1)) {
                        isFlag = 1;
                    }
                    else if (isFlag == 1 && ((tmpNum & maskBuf[bitCnt]) == 0)) {
                        isFlag = 2;
                    }
                    else if (isFlag == 2 && ((tmpNum & maskBuf[bitCnt]) == 0)) {
                        isFlag = 3;
                    }
                    else if (isFlag == 3 && ((tmpNum & maskBuf[bitCnt]) >= 1)) {
                        isFlag = 4;
                        rxBitCnt = 11;
                        continue;
                    }
                    else {
                        isFlag = 0;
                    }
                }
                if (rxBitCnt >= 0) {
                    tmpVal |= (((tmpNum & maskBuf[bitCnt]) >> bitCnt) << rxBitCnt);
                    rxBitCnt--;
                    if (rxBitCnt == -1)
                        break;
                }
            }
        }
        else {
            isFlag = 0;
        }
        if (rxBitCnt == -1)
            break;
    }

    return (float)((tmpVal - 1600.1) / 7.5817); //返回温度数据（℃）
}