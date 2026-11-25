#ifndef BMS_DATA_H
#define BMS_DATA_H

#include <stdint.h>

#define BMS_CELL_COUNT 4

/* One reading of the pack. Units match sensor_msgs/BatteryState (V, A, Ah, degC, 0..1). */
typedef struct {
  float voltage;
  float current;            /* negative = discharging (ROS convention) */
  float charge;             /* Ah remaining */
  float capacity;           /* Ah when full */
  float percentage;         /* 0..1 */
  float temperature;
  float cell_voltage[BMS_CELL_COUNT];
} battery_snapshot_t;

/* Latest snapshot shared between battery_task (writer) and ros_task (reader). */
void bms_data_init(void);
void bms_data_update(const battery_snapshot_t *in);
void bms_data_get(battery_snapshot_t *out);

#endif
