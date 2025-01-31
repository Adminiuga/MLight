#include "sl_component_catalog.h"

#ifdef SL_CATALOG_SL_BATTERY_MONITOR_PRESENT
#include <af.h>

#include "app.h"

void emberAfPowerConfigurationClusterBatteryUpdated(uint8_t endpoint,
                                                    uint8_t battery_double_percent,
                                                    uint16_t battery_milliV)
{
    sl_zigbee_app_debug_println("%d Battery voltage %dmV, %d%% remaining",
    TIMESTAMP_MS, battery_milliV, battery_double_percent>>1);
}
#endif // SL_CATALOG_SL_BATTERY_MONITOR_PRESENT