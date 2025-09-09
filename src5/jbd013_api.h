#ifndef JBD013_API_H_
#define JBD013_API_H_

#include "hal_driver.h"

//***************** JBD013VGA ָ�� *****************//
#define SPI_RD_ID 0x9f
#define SPI_RD_UID 0xab
#define SPI_DEEP_POWER_DOWN 0xb9
#define SPI_RST_EN 0x66
#define SPI_RST 0x99
#define SPI_SYNC 0x97
#define SPI_DISPLAY_ENABLE 0xa3
#define SPI_DISPLAY_DISABLE 0xa9
#define SPI_DISPLAY_DEFAULT_MODE 0x71
#define SPI_DISPLAY_UD 0x72
#define SPI_DISPLAY_RL 0x73
#define SPI_WR_LUM_REG 0x36
#define SPI_RD_LUM_REG 0x37
#define SPI_WR_CURRENT_REG 0x46
#define SPI_RD_CURRENT_REG 0x47
#define SPI_WR_OFFSET_REG 0xc0
#define SPI_RD_OFFSET_REG 0xc1
#define SPI_WR_CACHE 0x02
#define SPI_RD_CACHE 0x03
#define SPI_WR_CACHE_QSPI 0x62
#define SPI_RD_CACHE_QSPI 0x63
#define SPI_WR_CACHE_1BIT_QSPI 0x52
#define SPI_RD_CACHE_1BIT_QSPI 0x53
#define SPI_WR_CACHE_FAST_1BIT_QSPI 0x54
#define SPI_WR_ENABLE 0x06
#define SPI_WR_DISABLE 0x04
#define SPI_WR_STATUS_REG1 0x01
#define SPI_RD_STATUS_REG1 0x05
#define SPI_WR_STATUS_REG2 0x31
#define SPI_RD_STATUS_REG2 0x35
#define SPI_WR_STATUS_REG3 0x57
#define SPI_RD_STATUS_REG3 0x59
#define SPI_RD_CHECK_SUM_REG 0x42
#define SPI_RD_OTP 0x81
#define SPI_WR_OTP 0x82
#define SPI_SELF_TEST_ALL_OFF 0x13
#define SPI_SELF_TEST_ALL_ON 0x14
#define SPI_SELF_TEST_CHK_I 0x15
#define SPI_SELF_TEST_CHK_II 0x16
#define SPI_RD_TEMP_SENSOR 0x26

//***************** JBD013VGA api *****************//
void send_cmd(uint8_t cmd);
void read_id(void);
void read_uid(void);
void wr_status_reg(uint8_t regAddr, uint8_t data);
void rd_status_reg(uint8_t regAddr);
void wr_offset_reg(uint8_t row, uint8_t col);
void rd_offset_reg(void);
void wr_cur_reg(uint8_t param);
void rd_cur_reg(void);
void wr_lum_reg(uint16_t param);
void rd_lum_reg(void);
void set_mirror_mode(uint8_t param);
void clr_cache(void);
void display_image(uint16_t row, uint16_t col, uint8_t* pBuf, uint32_t len);
void panel_rst(void);
void panel_init(void);
void pixel_test(void);
void display_image_sync(uint16_t row, uint16_t col, uint8_t *pBuf, uint32_t len, uint8_t sync) ;
#endif