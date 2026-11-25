/* Fake 4S Li-ion pack: discharges at a constant 5 A from 16.8 V (full) to 14.0 V (empty),
 * then starts again from full. Small noise so the numbers visibly change. */
#include "battery_source.h"

#define CAPACITY_AH   5.0f
#define CURRENT_A     (-5.0f)
#define V_FULL        16.8f
#define V_EMPTY       14.0f
#define SAMPLE_S      0.1f
#define SPEEDUP       60.0f /* 1 real minute = 1 simulated hour, so a full cycle takes ~1 min */

static float charge_ah;
static uint32_t rng = 12345u;

static float noise(float amplitude)
{
  rng = rng * 1664525u + 1013904223u; /* LCG, good enough for fake data */
  return amplitude * ((float)(rng >> 8) / (float)(1u << 24) * 2.0f - 1.0f);
}

void battery_source_init(void)
{
  charge_ah = CAPACITY_AH;
}

void battery_source_read(battery_snapshot_t *out)
{
  charge_ah += CURRENT_A * (SAMPLE_S * SPEEDUP / 3600.0f);
  if (charge_ah <= 0.0f) charge_ah = CAPACITY_AH;

  float soc = charge_ah / CAPACITY_AH;
  float pack_v = V_EMPTY + (V_FULL - V_EMPTY) * soc;

  out->current = CURRENT_A + noise(0.05f);
  out->charge = charge_ah;
  out->capacity = CAPACITY_AH;
  out->percentage = soc;
  out->temperature = 25.0f + noise(0.2f);
  out->voltage = 0.0f;
  for (int i = 0; i < BMS_CELL_COUNT; i++) {
    out->cell_voltage[i] = pack_v / BMS_CELL_COUNT + noise(0.005f);
    out->voltage += out->cell_voltage[i];
  }
}
