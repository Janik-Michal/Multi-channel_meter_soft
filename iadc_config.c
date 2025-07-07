#include "em_iadc.h"
#include "em_cmu.h"
#include "em_gpio.h"

#define CLK_SRC_ADC_FREQ    20000000  // źródło dla ADC: 20 MHz
#define CLK_ADC_FREQ        10000000  // docelowa częstotliwość ADC: 10 MHz
#define VREF_MV             1250      // zewnętrzne napięcie referencyjne w mV

void initIADC_PC09(void)
{
  // Włącz zegary
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Ustaw źródło zegara dla IADC
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);  // 20 MHz

  // Skonfiguruj pin PC09 jako analogowe wejście
  GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD1_ADC0;  // PC09 = CDODD1

  // Struktury konfiguracyjne
  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitSingle_t singleInit = IADC_INITSINGLE_DEFAULT;
  IADC_SingleInput_t singleInput = IADC_SINGLEINPUT_DEFAULT;

  // Ustawienia ogólne
  init.warmup = iadcWarmupKeepWarm;
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

  // Ustawienia dla konfiguracji 0
  allConfigs.configs[0].reference = iadcCfgReferenceExt1V25;  // zewnętrzne Vref+ na pinie
  allConfigs.configs[0].vRef = VREF_MV;
  allConfigs.configs[0].adcClkPrescale = IADC_calcAdcClkPrescale(IADC0,
                                                CLK_ADC_FREQ,
                                                0,
                                                iadcCfgModeNormal,
                                                init.srcClkPrescale);
  allConfigs.configs[0].twosComplement = false;  // tryb unsigned, single-ended

  // Ustawienia wejścia — single-ended: tylko dodatnie napięcia
  singleInput.posInput = iadcPosInputPortCPin9;  // PC09
  singleInput.negInput = iadcNegInputGnd;        // GND

  // Inicjalizacja IADC
  IADC_init(IADC0, &init, &allConfigs);
  IADC_initSingle(IADC0, &singleInit, &singleInput);
}
