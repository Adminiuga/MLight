#include PLATFORM_HEADER
#include "hal.h"
#include "ember.h"
#include "af.h"
#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "sl_zigbee_debug_print.h"
#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

#include "app.h"
#include "hw_light.h"


typedef struct {
    uint8_t on_off;
    uint8_t level;
} cluster_init_counter_t;

cluster_init_counter_t _post_init_counter = {
    .on_off = CLUSTERS_TO_INIT_ON_OFF,
    .level = CLUSTERS_TO_INIT_LEVEL
};


// ******************************************
// Forward declarations for private functions
static void _sync_hardware_state(void);


// Callback implementations
void emberAfPluginOnOffClusterServerPostInitCallback(uint8_t endpoint)
{
    emberAfOnOffClusterPrintln("%d: Cluster post init - OnOff on %d ep, count: %d", TIMESTAMP_MS, endpoint, _post_init_counter.on_off);
    if ( _post_init_counter.on_off ) _post_init_counter.on_off--;
    _sync_hardware_state();
}

void emberAfPluginLevelControlClusterServerPostInitCallback(uint8_t endpoint)
{
    emberAfOnOffClusterPrintln("%d: Cluster post init - Level on %d ep, count: %d", TIMESTAMP_MS, endpoint, _post_init_counter.level);
    if ( _post_init_counter.level ) _post_init_counter.level--;
    _sync_hardware_state();
}

// internal method implementations
static void _sync_hardware_state(void)
{
    if ( _post_init_counter.on_off || _post_init_counter.level ) {
        sl_zigbee_app_debug_println("%d: Light is not yet fully initialized", TIMESTAMP_MS);
        return;
    }
    sl_zigbee_app_debug_println("%d: Light is initialized, sync hardware state", TIMESTAMP_MS);
}