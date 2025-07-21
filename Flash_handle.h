#ifndef FLASH_HANDLE_H_
#define FLASH_HANDLE_H_

#include <stdint.h>

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

void store_flash_data_struct(void);
void retrieve_flash_data_struct(void);
Internal_data_struct * flash_data_struct_getter(void);

// Nowe funkcje do kalibracji
void set_adc_calibration(int16_t *offset_mv, uint16_t *gain);
void set_modbus_settings(uint16_t id, uint16_t baud);

#endif /* FLASH_HANDLE_H_ */
