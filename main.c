#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_timer.h"
#include "iadc_config.h"
#include "uart_comm.h"
#include "modbus_rtu_slave.h"
#include "read_temp.h"
#include "int_sensor.h"
#include "Flash_handle.h"

#include <stdio.h>

#define NUM_SCAN_CHANNELS 5
#define FIR_TAPS 20

float fir_coeffs[FIR_TAPS] = {
    0.0667f, 0.0648f, 0.0629f, 0.0610f, 0.0590f,
    0.0571f, 0.0552f, 0.0533f, 0.0514f, 0.0495f,
    0.0476f, 0.0457f, 0.0438f, 0.0419f, 0.0400f,
    0.0381f, 0.0362f, 0.0343f, 0.0324f, 0.0305f
};

float adc_buffer[NUM_SCAN_CHANNELS][FIR_TAPS];
float tmp_fir_buffer[FIR_TAPS] = {0};
int adc_index[NUM_SCAN_CHANNELS] = {0};
int tmp_fir_index = 0;

char buffer[64];
float temperatures_C[NUM_SCAN_CHANNELS];
extern int16_t holding_registers[];

// --- filtrowanie próbek --- //
float fir_filter(float *buffer, int *index, float new_sample) {
    buffer[*index] = new_sample;
    float result = 0.0f;
    int idx = *index;

    for (int i = 0; i < FIR_TAPS; i++) {
        result += fir_coeffs[i] * buffer[idx];
        if (--idx < 0) idx = FIR_TAPS - 1;
    }

    *index = (*index + 1) % FIR_TAPS;
    return result;
}

void systick_init(void)
{
    SysTick->LOAD = SystemCoreClock - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void)
{
    iadc_convert_all_to_temperature(temperatures_C, NUM_SCAN_CHANNELS);
    Internal_data_struct* calib = flash_data_struct_getter();

    for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
        float filtered = fir_filter(adc_buffer[i], &adc_index[i], temperatures_C[i]);
        // --- kalibracja ---
        float gain = calib->ADC_calib_gain[i] / 1000.0f;
        float offset = calib->ADC_calib_offset[i] / 1000.0f;
        float calibrated = filtered * gain + offset;

        holding_registers[i] = (int16_t)(calibrated * 100.0f); // np. 25.42°C -> 2542
    }

    // TMP1075
    float tmp_temp = tmp1075_read_temperature();
    float tmp_filtered = fir_filter(tmp_fir_buffer, &tmp_fir_index, tmp_temp);
    float gain = calib->ADC_calib_gain[5] / 1000.0f;
    float offset = calib->ADC_calib_offset[5] / 1000.0f;
    float tmp_calibrated = tmp_filtered * gain + offset;

    holding_registers[NUM_SCAN_CHANNELS] = (int16_t)(tmp_calibrated * 100.0f);
}

void timer0_init(void)
{
    CMU_ClockEnable(cmuClock_TIMER0, true);

    TIMER_Init_TypeDef timerInit = {
        .enable     = true,
        .debugRun   = false,
        .prescale   = timerPrescale1024,
        .clkSel     = timerClkSelHFPerClk,
        .fallAction = timerInputActionNone,
        .riseAction = timerInputActionNone,
        .mode       = timerModeUp,
        .dmaClrAct  = false,
        .quadModeX4 = false,
        .oneShot    = false,
        .sync       = false
    };

    TIMER_Init(TIMER0, &timerInit);
    TIMER_TopSet(TIMER0, 185);
    TIMER_IntEnable(TIMER0, TIMER_IEN_OF);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void)
{
    TIMER_IntClear(TIMER0, TIMER_IF_OF);
    modbus_on_frame_timeout();
}

int main(void)
{
    CHIP_Init();
    retrieve_flash_data_struct();
    uart_init();
    i2c_init();
    initIADC();
    systick_init();
    timer0_init();

    NVIC_EnableIRQ(USART0_RX_IRQn);

    while (1)
    {
        modbus_poll();
        __WFI();
    }
}
