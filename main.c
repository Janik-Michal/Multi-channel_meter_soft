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
#include "em_iadc.h"

#include <stdio.h>
#include <stdbool.h>

#define NUM_SCAN_CHANNELS 10
#define FIR_TAPS          20

// ---------------- FIR coefficients ----------------
static float fir_coeffs[FIR_TAPS] = {
    0.069f, 0.068f, 0.066f, 0.064f, 0.062f,
    0.060f, 0.058f, 0.056f, 0.053f, 0.051f,
    0.049f, 0.047f, 0.045f, 0.042f, 0.040f,
    0.038f, 0.036f, 0.034f, 0.032f, 0.030f
};

// ---------------- Buffers ----------------
static float adc_buffer[NUM_SCAN_CHANNELS][FIR_TAPS];
static int   adc_index[NUM_SCAN_CHANNELS] = {0};

static float tmp_fir_buffer[FIR_TAPS] = {0};
static int   tmp_fir_index = 0;

static float temperatures_C[NUM_SCAN_CHANNELS];
extern int16_t holding_registers[];

// ---------------- Flags ----------------
volatile bool g_do_measurement  = false;  // ustawiane w SysTick
volatile bool g_modbus_timeout  = false; // ustawiane w Timer0

// ---------------- FIR filter ----------------
static float fir_filter(float *buffer, int *index, float new_sample)
{
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

// ---------------- SysTick ----------------
static void systick_init(void)
{
    SysTick->LOAD = (SystemCoreClock) - 1; // 1 Hz tick
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk    |
                    SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void)
{
    g_do_measurement = true;  // tylko sygnał
}

// ---------------- Timer0 ----------------
static void timer0_init(void)
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
    TIMER_TopSet(TIMER0, 4);
    TIMER_IntEnable(TIMER0, TIMER_IEN_OF);
    NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler(void)
{
    TIMER_IntClear(TIMER0, TIMER_IF_OF);
    g_modbus_timeout = true; // tylko flaga
}

// ---------------- Measurement task ----------------
static void do_measurement_task(void)
{
    iadc_convert_all_to_temperature(temperatures_C, NUM_SCAN_CHANNELS);
    Internal_data_struct* calib = flash_data_struct_getter();

    for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
        float filtered   = fir_filter(adc_buffer[i], &adc_index[i], temperatures_C[i]);
        float gain       = calib->ADC_calib_gain[i]   / 1000.0f;
        float offset     = calib->ADC_calib_offset[i] / 1000.0f;
        float calibrated = filtered * gain + offset - 0.5f;

        holding_registers[i] = (int16_t)(calibrated * 100.0f);
    }

    // TMP1075
    float tmp_temp     = tmp1075_read_temperature();
    float tmp_filtered = fir_filter(tmp_fir_buffer, &tmp_fir_index, tmp_temp);
    holding_registers[10] = (int16_t)(tmp_filtered * 100.0f);

    // Example: raw ADC channel value
    float v_ch1 = (float)scanResult[0];
    holding_registers[33] = (int16_t)(v_ch1);
}

// ---------------- Main ----------------
int main(void)
{
    CHIP_Init();
    MSC_Init();

    retrieve_flash_data_struct();

    // Init peripherals
    uart_init();

    i2c_init();
    initIADC();
    systick_init();
    timer0_init();

    NVIC_EnableIRQ(USART0_RX_IRQn);

    while (1)
    {
        // zadania okresowe
        if (g_do_measurement) {
            g_do_measurement = false;
            IADC_command(IADC0, iadcCmdStartScan);
            do_measurement_task();
        }

        // obsługa timeoutu Modbus
        if (g_modbus_timeout) {
            g_modbus_timeout = false;
            modbus_on_frame_timeout();
        }

        // obsługa protokołu
        modbus_poll();

        __WFI(); // oszczędzanie energii
    }
}
