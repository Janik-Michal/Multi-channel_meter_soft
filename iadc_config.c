#include "em_device.h"
#include "em_chip.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_iadc.h"

#define CLK_SRC_ADC_FREQ 20000000
#define CLK_ADC_FREQ     10000000
#define NUM_INPUTS       10

volatile double scanResult[NUM_INPUTS] = {0.0};

static const IADC_PosInput_t iadc_inputs[NUM_INPUTS] = {
    iadcPosInputPortCPin9, iadcPosInputPortCPin8,
    iadcPosInputPortCPin7, iadcPosInputPortCPin6,
    iadcPosInputPortCPin5, iadcPosInputPortCPin4,
    iadcPosInputPortCPin3, iadcPosInputPortCPin2,
    iadcPosInputPortCPin1, iadcPosInputPortCPin0
};

void initIADC(void)
{
    CMU_ClockSelectSet(cmuClock_IADCCLK, cmuSelect_FSRCO);
    CMU_ClockEnable(cmuClock_GPIO, true);
    CMU_ClockEnable(cmuClock_IADC0, true);

    for (int p = 0; p < 10; p++) {
        GPIO_PinModeSet(gpioPortC, p, gpioModeDisabled, 0);
    }

    IADC_Init_t init = IADC_INIT_DEFAULT;
    IADC_AllConfigs_t initAllConfigs = IADC_ALLCONFIGS_DEFAULT;
    IADC_InitScan_t initScan = IADC_INITSCAN_DEFAULT;
    IADC_ScanTable_t scanTable = IADC_SCANTABLE_DEFAULT;

    init.warmup = iadcWarmupNormal;
    init.srcClkPrescale = IADC_calcSrcClkPrescale(IADC0, CLK_SRC_ADC_FREQ, 0);

    initAllConfigs.configs[0].reference      = iadcCfgReferenceExt1V25; // ext 1,25V
    initAllConfigs.configs[0].vRef           = 1250;
    initAllConfigs.configs[0].osrHighSpeed = iadcCfgOsrHighSpeed64x;
    initAllConfigs.configs[0].adcClkPrescale =
        IADC_calcAdcClkPrescale(IADC0, CLK_ADC_FREQ, 0, iadcCfgModeNormal, init.srcClkPrescale);

    initScan.dataValidLevel = iadcFifoCfgDvl1;
    initScan.showId         = true;

    for (int i = 0; i < NUM_INPUTS; i++) {
        scanTable.entries[i].posInput = iadc_inputs[i];
        scanTable.entries[i].negInput = iadcNegInputGnd;
        scanTable.entries[i].includeInScan = true;
    }

    GPIO->CDBUSALLOC =
          GPIO_CDBUSALLOC_CDEVEN0_ADC0
        | GPIO_CDBUSALLOC_CDEVEN1_ADC0
        | GPIO_CDBUSALLOC_CDODD0_ADC0
        | GPIO_CDBUSALLOC_CDODD1_ADC0;

    IADC_init(IADC0, &init, &initAllConfigs);
    IADC_initScan(IADC0, &initScan, &scanTable);

    // IRQ to read FIFO only
    IADC_enableInt(IADC0, IADC_IEN_SCANFIFODVL);
    NVIC_ClearPendingIRQ(IADC_IRQn);
    NVIC_EnableIRQ(IADC_IRQn);
}

void IADC_IRQHandler(void)
{
    while (IADC_getScanFifoCnt(IADC0)) {
        IADC_Result_t s = IADC_pullScanFifoResult(IADC0);
        if (s.id < NUM_INPUTS) {
            scanResult[s.id] = (double)s.data * 1250.0 / 0xFFF; // ext 1,25V
        }
    }
    IADC_clearInt(IADC0, IADC_IF_SCANFIFODVL);
}
