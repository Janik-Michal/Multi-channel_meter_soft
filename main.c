#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "em_timer.h"
#include "iadc_config.h"
#include "uart_comm.h"
#include "modbus_rtu_slave.h"
#include "read_temp.h"
#include <stdio.h>

#define NUM_SCAN_CHANNELS 5

char buffer[64];
float temperatures_C[NUM_SCAN_CHANNELS];
extern volatile double scanResult[NUM_SCAN_CHANNELS];
extern int16_t holding_registers[]; // dostęp do rejestrów Modbus

void systick_init(void)
{
    SysTick->LOAD = SystemCoreClock - 1;  // 1 Hz
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_ENABLE_Msk |
                    SysTick_CTRL_TICKINT_Msk;
}

void SysTick_Handler(void)
{
    iadc_convert_all_to_temperature(temperatures_C, NUM_SCAN_CHANNELS);

    // Update holding_registers
    for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
            holding_registers[i] = (int16_t)(temperatures_C[i] * 100.0f);
    }
                      // ---  USB --- //
   /* uart_send_string("Temperatury:\r\n");
    for (int i = 0; i < NUM_SCAN_CHANNELS; i++) {
        snprintf(buffer, sizeof(buffer), "CH[%d]: %.2f °C\r\n", i, temperatures_C[i]);
        uart_send_string(buffer);
    }*/
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
    TIMER_TopSet(TIMER0, 185);  // 1 ms
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
    uart_init();               // USART0: TX/RX
    initIADC();                // ADC - 5 kanałów
    systick_init();            // 1 Hz update
    timer0_init();             // Timer dla Modbus RTU

    NVIC_EnableIRQ(USART0_RX_IRQn); // RX interrupt

    while (1) {

        modbus_poll();  // Obsługa Modbus
        __WFI();        // Czekaj na przerwanie
    }
}
