#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"

#define CLK_SRC_ADC_FREQ        20000000
#define CLK_ADC_FREQ            10000000
#define NUM_INPUTS              5

// Kanały PC09 - PC05
#define IADC_INPUT_0_PORT_PIN   iadcPosInputPortCPin9
#define IADC_INPUT_1_PORT_PIN   iadcPosInputPortCPin8
#define IADC_INPUT_2_PORT_PIN   iadcPosInputPortCPin7
#define IADC_INPUT_3_PORT_PIN   iadcPosInputPortCPin6
#define IADC_INPUT_4_PORT_PIN   iadcPosInputPortCPin5

#define GPIO_OUTPUT_0_PORT      gpioPortC
#define GPIO_OUTPUT_0_PIN       4

volatile double scanResult[NUM_INPUTS] = {0.0};
volatile bool newScanReady = false;

void initIADC(void)
{
   CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);
   CMU_ClockEnable(cmuClock_GPIO, true);
   CMU_ClockEnable(cmuClock_IADC0, true);

   GPIO_PinModeSet(GPIO_OUTPUT_0_PORT, GPIO_OUTPUT_0_PIN, gpioModePushPull, 0);

  IADC_Init_t init = IADC_INIT_DEFAULT;
  IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
  IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;
  IADC_ScanTable_t scanTable = IADC_SCANTABLE_DEFAULT;

  init.warmup = iadcWarmupNormal;
  init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);
  init.timerCycles = CMU_ClockFreqGet(cmuClock_IADCCLK) / 1000; // 1 kHz trigger

  initAllConfigs.configs[0].reference =  iadcCfgReferenceExt1V25;
  initAllConfigs.configs[0].vRef = 1250;
  initAllConfigs.configs[0].adcClkPrescale =
    IADC_calcAdcClkPrescale(IADC0, CLK_ADC_FREQ, 0, iadcCfgModeNormal, init.srcClkPrescale);

  initScan.triggerSelect = iadcTriggerSelImmediate;
  initScan.dataValidLevel = iadcFifoCfgDvl1;
  initScan.showId = true;

  // Tabela kanałów
  scanTable.entries[0].posInput = IADC_INPUT_0_PORT_PIN;
  scanTable.entries[1].posInput = IADC_INPUT_1_PORT_PIN;
  scanTable.entries[2].posInput = IADC_INPUT_2_PORT_PIN;
  scanTable.entries[3].posInput = IADC_INPUT_3_PORT_PIN;
  scanTable.entries[4].posInput = IADC_INPUT_4_PORT_PIN;

  for (int i = 0; i < NUM_INPUTS; i++) {
    scanTable.entries[i].negInput = iadcNegInputGnd;
    scanTable.entries[i].includeInScan = true;
  }

  // Alokacja pinów PC05–PC09 do ADC0
  GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD1_ADC0;  // PC09, PC05
  GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN1_ADC0; // PC08
  GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDODD0_ADC0;  // PC07
  GPIO->CDBUSALLOC |= GPIO_CDBUSALLOC_CDEVEN0_ADC0; // PC06

  IADC_init(IADC0, &init, &initAllConfigs);
  IADC_initScan(IADC0, &initScan, &scanTable);
  IADC_command(IADC0, iadcCmdStartScan);  // Ręczne uruchomienie

  //IADC_command(IADC0, iadcCmdEnableTimer);

  IADC_enableInt(IADC0, IADC_IEN_SCANFIFODVL);
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
}

void IADC_IRQHandler(void)
{
  IADC_Result_t sample;
  GPIO_PinOutToggle(GPIO_OUTPUT_0_PORT, GPIO_OUTPUT_0_PIN);

  while (IADC_getScanFifoCnt(IADC0)) {
    sample = IADC_pullScanFifoResult(IADC0);
    if (sample.id < NUM_INPUTS) {
      scanResult[sample.id] = sample.data * 1250 / 0xFFF; // przeliczenie na napięcie (V)
    }
  }

  IADC_clearInt(IADC0, IADC_IF_SCANFIFODVL);
  IADC_command(IADC0, iadcCmdStartScan);
}

