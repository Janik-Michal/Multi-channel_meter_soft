#include "read_temp.h"
#include "em_iadc.h"
#include <math.h>
#include <stddef.h>
#include <stdbool.h>

extern volatile double scanResult[];

/* Conversion: V -> R */
#ifndef V_TO_R_OFFSET
#define V_TO_R_OFFSET 1835
#endif
#ifndef V_TO_R_SCALE
#define V_TO_R_SCALE 2.235
#endif

/* Const Callendar–Van Dusen for PT1000 IEC60751 */
#define PT_R0   1000.0
#define PT_A    3.9083e-3
#define PT_B   -5.775e-7
#define PT_C   -4.183e-12   /* T < 0 */

/* ========================================================================== */
/*  EMPIRICAL ERROR COMPENSATION                                              */
/*  ΔT = c - a*(T - t0)^2                                                     */
/*  T_comp = T + ΔT                                                           */
/* ========================================================================== */
static const double COMP_A  = 0.00019;
static const double COMP_T0 = 25.8;
static const double COMP_C  = 0.07;

static double apply_empirical_comp(double T_cvd)
{
    double dx = T_cvd - COMP_T0;
    double delta = COMP_C - COMP_A * dx * dx;
    return T_cvd - delta;
}

/* ========================================================================== */
/*  Inverse function Callendar–Van Dusen: R → T                               */
/* ========================================================================== */
static double PT1000_CVD_to_temperature(double R)
{
    double r = R / PT_R0;

    /* 1)  T >= 0 */
    double D = PT_A * PT_A - 4.0 * PT_B * (1.0 - r);

    if (D >= 0) {
        double T1 = (-PT_A + sqrt(D)) / (2.0 * PT_B);
        double T2 = (-PT_A - sqrt(D)) / (2.0 * PT_B);

        double T = (T1 >= 0 ? T1 : T2);

        if (T >= 0 && T < 850) {
            return T;
        }
    }

    /* 2) Newton for T < 0 */
    double T = -50.0;
    for (int i = 0; i < 15; i++) {
        double f = PT_R0 * (1.0 + PT_A*T + PT_B*T*T + PT_C*(T - 100.0)*T*T*T) - R;
        double df = PT_R0 * (PT_A + 2.0*PT_B*T + PT_C*(4.0*T*T*T - 300.0*T*T + 30000.0*T));

        T = T - f / df;
    }

    return T;
}

/* ========================================================================== */
/*  Main function: voltage → resistance → temperature + compensation     */
/* ========================================================================== */
void iadc_convert_all_to_temperature(float *temperatures_C, int NUM_SCAN_CHANNELS)
{
    for (int i = 0; i < NUM_SCAN_CHANNELS; ++i) {

        double V_mV = scanResult[i];

        /* Converting voltage to resistance */
        double R = (V_mV + V_TO_R_OFFSET) / V_TO_R_SCALE;

        /* Callendar–Van Dusen */
        double T = PT1000_CVD_to_temperature(R);

        /* Empirical compensation */
        double T_corr = apply_empirical_comp(T);

        temperatures_C[i] = (float)T_corr;
    }
}
