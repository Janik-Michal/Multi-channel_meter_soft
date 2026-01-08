#include "Flash_handle.h"
#include "em_msc.h" // Flash access
#include "modbus_rtu_slave.h"
#include <string.h>

#define USER_PAGE_START_ADDR  0x0807E000UL
#define USER_PAGE_SIZE        8192UL

static Internal_data_struct mInternal_data_struct = {
    .ADC_calib_gain = {1000,1000,1000,1000,1000,1000,1000,1000,1000,1000},   // 1.0000 gain
    .ADC_calib_offset = {0,0,0,0,0,0,0,0,0,0},              // 0.000 offset
    .Modbus_ID = 2,
    .Modbus_baud = 4,
    .HWID = 0,
    .SWID = 0,
    ._padding = {0,0},
    .CRC = 0
};
void read_bytes_from_flash(uint32_t address, uint8_t *data, uint32_t bytes_amount)
{
    memcpy(data, (const void *)(0x08000000+address), bytes_amount); //aligned to flash start address
}

uint32_t read_bytes_from_user_flash(uint32_t address, uint8_t *data, uint32_t bytes_amount)
{
  if(address > (8192 - 1)) return 1; //out of range, the user data is 8192bytes
  if(bytes_amount > (8192)) return 1; //out of range, the user data is 8192bytes
  read_bytes_from_flash(address + 0x7E000, data, bytes_amount);//address aligned to the end of flash - user area
  return 0;
}

MSC_Status_TypeDef write_user_flash(uint8_t *data, uint32_t bytes_amount)
{
  if(bytes_amount % 4) return mscReturnUnaligned; //amount must be aligned to four

    MSC_Init();

    if (MSC_ErasePage((uint32_t *)(0x7E000 + 0x08000000)) != mscReturnOk) {
        MSC_Deinit();
        return false;
    }

    MSC_Status_TypeDef status = MSC_WriteWord((uint32_t *)(0x7E000 + 0x08000000), (void const *)data, bytes_amount);

    if (status != mscReturnOk) {
        MSC_Deinit();
        return status;
    }
    MSC_Deinit();
    return mscReturnOk;
}

void rewrite_CRC_to_flash_data_struct(void)
{
  mInternal_data_struct.CRC = modbus_crc16((uint8_t *)(&mInternal_data_struct), sizeof(Internal_data_struct) - 2);
}

void store_flash_data_struct(void)
{
    mInternal_data_struct.CRC = modbus_crc16((uint8_t *)&mInternal_data_struct, CRC_OFFSET);
    write_user_flash((uint8_t *)(&mInternal_data_struct), sizeof(Internal_data_struct));
}

  void retrieve_flash_data_struct(void)
  {
    Internal_data_struct temp;
    read_bytes_from_user_flash(0, (uint8_t *)&temp, sizeof(temp));

    uint16_t crc_calc = modbus_crc16((uint8_t *)&temp, CRC_OFFSET);

    if (temp.CRC == crc_calc) {
        memcpy(&mInternal_data_struct, &temp, sizeof(temp));
    } else {
        rewrite_CRC_to_flash_data_struct();
        store_flash_data_struct();
    }
  }

Internal_data_struct * flash_data_struct_getter(void)
{
  return &mInternal_data_struct;
}

void set_adc_calibration(int16_t *offset_mv, uint16_t *gain)
{
  memcpy(mInternal_data_struct.ADC_calib_offset, offset_mv, sizeof(mInternal_data_struct.ADC_calib_offset));
  memcpy(mInternal_data_struct.ADC_calib_gain, gain, sizeof(mInternal_data_struct.ADC_calib_gain));
  rewrite_CRC_to_flash_data_struct();
  store_flash_data_struct();
}
void set_modbus_settings(uint16_t id, uint16_t baud)
{
  mInternal_data_struct.Modbus_ID = id;
  mInternal_data_struct.Modbus_baud = baud;
  rewrite_CRC_to_flash_data_struct();
  store_flash_data_struct();
}
