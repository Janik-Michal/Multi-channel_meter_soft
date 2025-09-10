/* read_temp_lut_only.c
 * LUT (-50..90 °C, krok 1°C) + interpolacja liniowa.
 * Wejście: scanResult[] w mV (jeśli masz w V, pomnóż *1000).
 */

#include "read_temp.h"
#include "em_iadc.h"
#include <math.h>
#include <stddef.h>
#include <stdbool.h>

extern volatile double scanResult[];

/* V->R conversion (tune if needed) */
#ifndef V_TO_R_OFFSET
#define V_TO_R_OFFSET 1859.3
#endif
#ifndef V_TO_R_SCALE
#define V_TO_R_SCALE 2.2637
#endif

/* LUT (−50..90 °C, 141 punktów) */
typedef struct { float tC; float Rohm; } PT_LUT_Entry;
#define PT_LUT_MIN_T  (-50)
#define PT_LUT_MAX_T  (90)
#define PT_LUT_SIZE   (PT_LUT_MAX_T - PT_LUT_MIN_T + 1)

static const PT_LUT_Entry pt1000_lut[PT_LUT_SIZE] = {
    {-50.0f,803.0628f},{-49.0f,807.0334f},{-48.0f,811.0026f},{-47.0f,814.9704f},
    {-46.0f,818.9368f},{-45.0f,822.9019f},{-44.0f,826.8656f},{-43.0f,830.8280f},
    {-42.0f,834.7890f},{-41.0f,838.7486f},{-40.0f,842.7069f},{-39.0f,846.6638f},
    {-38.0f,850.6193f},{-37.0f,854.5735f},{-36.0f,858.5262f},{-35.0f,862.4776f},
    {-34.0f,866.4276f},{-33.0f,870.3761f},{-32.0f,874.3233f},{-31.0f,878.2689f},
    {-30.0f,882.2133f},{-29.0f,886.1562f},{-28.0f,890.0977f},{-27.0f,894.0378f},
    {-26.0f,897.9765f},{-25.0f,901.9138f},{-24.0f,905.8496f},{-23.0f,909.7839f},
    {-22.0f,913.7168f},{-21.0f,917.6481f},{-20.0f,921.5779f},{-19.0f,925.5063f},
    {-18.0f,929.4331f},{-17.0f,933.3584f},{-16.0f,937.2821f},{-15.0f,941.2043f},
    {-14.0f,945.1249f},{-13.0f,949.0439f},{-12.0f,952.9613f},{-11.0f,956.8770f},
    {-10.0f,960.7911f},{ -9.0f,964.7035f},{ -8.0f,968.6143f},{ -7.0f,972.5233f},
    { -6.0f,976.4306f},{ -5.0f,980.3360f},{ -4.0f,984.2396f},{ -3.0f,988.1414f},
    { -2.0f,992.0412f},{ -1.0f,995.9391f},{  0.0f,1000.0000f},{  1.0f,1003.9029f},
    {  2.0f,1007.8057f},{  3.0f,1011.7084f},{  4.0f,1015.6109f},{  5.0f,1019.5132f},
    {  6.0f,1023.4154f},{  7.0f,1027.3173f},{  8.0f,1031.2191f},{  9.0f,1035.1207f},
    { 10.0f,1039.0221f},{ 11.0f,1042.9232f},{ 12.0f,1046.8241f},{ 13.0f,1050.7248f},
    { 14.0f,1054.6252f},{ 15.0f,1058.5253f},{ 16.0f,1062.4251f},{ 17.0f,1066.3246f},
    { 18.0f,1070.2237f},{ 19.0f,1074.1225f},{ 20.0f,1078.0209f},{ 21.0f,1081.9189f},
    { 22.0f,1085.8165f},{ 23.0f,1089.7137f},{ 24.0f,1093.6105f},{ 25.0f,1097.5069f},
    { 26.0f,1101.4028f},{ 27.0f,1105.2983f},{ 28.0f,1109.1933f},{ 29.0f,1113.0878f},
    { 30.0f,1116.9819f},{ 31.0f,1120.8754f},{ 32.0f,1124.7685f},{ 33.0f,1128.6610f},
    { 34.0f,1132.5531f},{ 35.0f,1136.4446f},{ 36.0f,1140.3356f},{ 37.0f,1144.2261f},
    { 38.0f,1148.1161f},{ 39.0f,1152.0054f},{ 40.0f,1155.8943f},{ 41.0f,1159.7825f},
    { 42.0f,1163.6702f},{ 43.0f,1167.5574f},{ 44.0f,1171.4440f},{ 45.0f,1175.3301f},
    { 46.0f,1179.2156f},{ 47.0f,1183.1005f},{ 48.0f,1186.9850f},{ 49.0f,1190.8690f},
    { 50.0f,1194.7523f},{ 51.0f,1198.6351f},{ 52.0f,1202.5173f},{ 53.0f,1206.3989f},
    { 54.0f,1210.2798f},{ 55.0f,1214.1601f},{ 56.0f,1218.0398f},{ 57.0f,1221.9188f},
    { 58.0f,1225.7972f},{ 59.0f,1229.6749f},{ 60.0f,1233.5519f},{ 61.0f,1237.4283f},
    { 62.0f,1241.3039f},{ 63.0f,1245.1789f},{ 64.0f,1249.0531f},{ 65.0f,1252.9267f},
    { 66.0f,1256.7995f},{ 67.0f,1260.6716f},{ 68.0f,1264.5430f},{ 69.0f,1268.4136f},
    { 70.0f,1272.2835f},{ 71.0f,1276.1526f},{ 72.0f,1280.0210f},{ 73.0f,1283.8886f},
    { 74.0f,1287.7554f},{ 75.0f,1291.6214f},{ 76.0f,1295.4866f},{ 77.0f,1299.3510f},
    { 78.0f,1303.2145f},{ 79.0f,1307.0772f},{ 80.0f,1310.9390f},{ 81.0f,1314.7999f},
    { 82.0f,1318.6599f},{ 83.0f,1322.5190f},{ 84.0f,1326.3771f},{ 85.0f,1330.2343f},
    { 86.0f,1334.0905f},{ 87.0f,1337.9456f},{ 88.0f,1341.7998f},{ 89.0f,1345.6530f},
    { 90.0f,1349.5051f}
};

/* Binary search: znajdź najbliższy indeks */
static int lut_find_index_by_R(double R)
{
    int lo = 0, hi = PT_LUT_SIZE - 1;
    if (R <= pt1000_lut[0].Rohm) return 0;
    if (R >= pt1000_lut[hi].Rohm) return hi - 1;
    while (hi - lo > 1) {
        int mid = (lo + hi) >> 1;
        if (pt1000_lut[mid].Rohm <= R) lo = mid;
        else hi = mid;
    }
    return lo;
}

/* Liniowa interpolacja R -> t */
static double lut_interpolate_R_to_t(double R)
{
    int idx = lut_find_index_by_R(R);
    double R1 = pt1000_lut[idx].Rohm;
    double R2 = pt1000_lut[idx+1].Rohm;
    double T1 = pt1000_lut[idx].tC;
    double T2 = pt1000_lut[idx+1].tC;
    if (R2 == R1) return T1;
    double frac = (R - R1) / (R2 - R1);
    return T1 + frac * (T2 - T1);
}

/* Główna funkcja */
void iadc_convert_all_to_temperature(float *temperatures_C, int NUM_SCAN_CHANNELS)
{
    for (int i = 0; i < NUM_SCAN_CHANNELS; ++i) {
        double V_mV = scanResult[i]; /* jeśli w Voltach, daj *1000.0 */
        double R = (V_mV + V_TO_R_OFFSET) / V_TO_R_SCALE;

        double t_est = lut_interpolate_R_to_t(R);

        temperatures_C[i] = (float)t_est;
    }
}
