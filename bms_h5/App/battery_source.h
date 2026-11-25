#ifndef BATTERY_SOURCE_H
#define BATTERY_SOURCE_H

#include "bms_data.h"

/* Where battery readings come from. Today: battery_source_fake.c.
 * Later: a MAX17205 driver implementing these same two functions. */
void battery_source_init(void);
void battery_source_read(battery_snapshot_t *out); /* called every 100 ms */

#endif
