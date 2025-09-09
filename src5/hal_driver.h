#ifndef HAL_DRIVER_H_
#define HAL_DRIVER_H_

#include "jbd013_api.h"

int spi_tx_frame(uint8_t* param);
int spi_rx_frame(uint8_t cmd, uint8_t* param, uint32_t len);
int spi_rd_buffer(uint16_t row, uint16_t col, uint32_t len);
int spi_wr_buffer(uint16_t col, uint16_t row, uint8_t* pBuf, uint32_t len);
float get_temperature_sensor_data(void);
#endif