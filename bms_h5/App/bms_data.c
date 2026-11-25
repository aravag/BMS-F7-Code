#include "bms_data.h"
#include "cmsis_os2.h"

static battery_snapshot_t latest;
static osMutexId_t lock;

void bms_data_init(void)
{
  lock = osMutexNew(NULL);
}

void bms_data_update(const battery_snapshot_t *in)
{
  osMutexAcquire(lock, osWaitForever);
  latest = *in;
  osMutexRelease(lock);
}

void bms_data_get(battery_snapshot_t *out)
{
  osMutexAcquire(lock, osWaitForever);
  *out = latest;
  osMutexRelease(lock);
}
