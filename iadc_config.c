#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"
#include "iadc_config.h"

#define VREF_MV 1250 // Zewnętrzne napięcie odniesienia 1.25V

void initIADC_PC09(void)
{
  // Włącz zegary
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_HFRCOEM23, true);
  CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_HFRCOEM23);

  // PC09 jako wejście analogowe
  GPIO_PinModeSet(gpioPortC, 9, gpioModeDisabled, 0);

  // Inicjalizacja IADC
  IADC_Init_t init = IADC_INIT_DEFAULT;
  init.warmup = iadcWarmupKeepWarm;

  // Źródło zegara IADC: FSRCO (~20 MHz), preskaler: 20 → 1 MHz
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, 1000000, 0);

  // Konfiguracja IADC
  IADC_AllConfigs_t allConfigs = IADC_ALLCONFIGS_DEFAULT;
  allConfigs.configs[0].reference = iadcCfgReferenceExt1V25;
  allConfigs.configs[0].vRef = 1250;
  allConfigs.configs[0].analogGain = iadcCfgAnalogGain1x;

  // Wejście: PC09
  IADC_SingleInput_t input = IADC_SINGLEINPUT_DEFAULT;
  input.posInput = iadcPosInputPortCPin9;
  input.negInput = iadcNegInputGnd;
  input.configId = 0;

  IADC_InitSingle_t initSingle = IADC_INITSINGLE_DEFAULT;
  initSingle.triggerAction = iadcTriggerActionOnce;
  initSingle.dataValidLevel = iadcFifoCfgDvl1;
  initSingle.start = false;

  IADC_init(IADC0, &init, &allConfigs);
  IADC_initSingle(IADC0, &initSingle, &input);
}

float iadc_read_mv(void)
{
  IADC_command(IADC0, iadcCmdStartSingle);

  // Czekaj aż wynik będzie gotowy
  while (!(IADC0->STATUS & _IADC_STATUS_SINGLEFIFODV_MASK));

  IADC_Result_t result = IADC_pullSingleFifoResult(IADC0);

  // UWAGA: dane są 12-bitowe (0–4095) tylko jeśli rozdzielczość jest taka ustawiona
  // Jeśli masz 16-bitową rozdzielczość → dziel przez 65535.0

  return ((float)result.data * VREF_MV) / 4095.0f;
}

