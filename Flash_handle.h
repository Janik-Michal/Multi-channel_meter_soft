#ifndef FLASH_HANDLE_H_
#define FLASH_HANDLE_H_

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "em_device.h"      // Critical for EFM32PG23
#include "em_msc.h"         // MSC functions
#include "em_cmu.h"
#include "modbus_rtu_slave.h"

typedef struct
{
  // Kalibracja ADC
  uint16_t ADC_calib_gain[6];   // 0–5
  uint16_t ADC_calib_offset[6];

  // Konfiguracja Modbus
  uint16_t Modbus_ID;
  uint16_t Modbus_baud; // 0=115200, 1=38400, 2=9600, 3=2400

  // Identyfikatory
  uint16_t HWID;
  uint16_t SWID;

  uint16_t CRC;
} Internal_data_struct;

void rewrite_CRC_to_flash_data_struct(void);
void store_flash_data_struct(void);
void retrieve_flash_data_struct(void);
void read_bytes_from_flash(uint32_t address, uint8_t *data, uint32_t data_amount);
uint32_t read_bytes_from_user_flash(uint32_t address, uint8_t *data, uint32_t data_amount);
MSC_Status_TypeDef  write_user_flash(uint8_t *data, uint32_t bytes_amount);
Internal_data_struct * flash_data_struct_getter(void);

// Nowe funkcje do kalibracji
void set_adc_calibration(int16_t *offset_mv, uint16_t *gain);
void set_modbus_settings(uint16_t id, uint16_t baud);

#endif /* FLASH_HANDLE_H_ */
