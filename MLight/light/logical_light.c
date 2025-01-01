#include PLATFORM_HEADER
#include "hal.h"
#include "ember.h"
#include "af.h"
#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "sl_zigbee_debug_print.h"
#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT

#include "app.h"
#include "hw_light.h"
#include "logical_light.h"

#define EP_RGB_LIGHT     1
#define EP_RED_CHANNEL   2
#define EP_GREEN_CHANNEL 3
#define EP_BLUE_CHANNEL  4

typedef struct {
    uint8_t endpoint;
    uint8_t *onoff;
    uint8_t *level;
    enum RGB_channel_name_t ch_name;
} _ch_state_t;

typedef struct {
    uint8_t on_off;
    uint8_t level;
} cluster_init_counter_t;

typedef struct {
    cluster_init_counter_t init_counters;
    bool inhibit_updates;
} Llight_state_t;

static Llight_state_t _state = {
    .init_counters = {
        .on_off = CLUSTERS_TO_INIT_ON_OFF,
        .level = CLUSTERS_TO_INIT_LEVEL
    },
    .inhibit_updates = true
};


// ******************************************
// Forward declarations for private functions
static void _sync_hardware_state(void);
static sl_status_t _sync_light_channel(uint8_t endpoint, enum RGB_channel_name_t ch_name);


// Callback implementations
void emberAfPluginOnOffClusterServerPostInitCallback(uint8_t endpoint)
{
    emberAfOnOffClusterPrintln("%d: Cluster post init - OnOff on %d ep, count: %d", TIMESTAMP_MS, endpoint, _state.init_counters.on_off);
    if ( _state.init_counters.on_off ) _state.init_counters.on_off--;
    _sync_hardware_state();
}

void emberAfPluginLevelControlClusterServerPostInitCallback(uint8_t endpoint)
{
    emberAfOnOffClusterPrintln("%d: Cluster post init - Level on %d ep, count: %d", TIMESTAMP_MS, endpoint, _state.init_counters.level);
    if ( _state.init_counters.level ) _state.init_counters.level--;
    _sync_hardware_state();
}

// *****************************
// Public method implementations
// -----------------------------

/**
 * @brief Sync color light state from individual channel lights
 *        this is required when operating on individual channel/lights
 */
sl_status_t llight_sync_channel_light_to_color(void)
{
    uint8_t r_onoff, g_onoff, b_onoff;
    uint8_t r_lvl, g_lvl, b_lvl;
    _ch_state_t init[] = {
        {
            .endpoint = EP_RED_CHANNEL,
            .onoff = &r_onoff,
            .level = &r_lvl,
            .ch_name = CH_RED,
        },
        {
            .endpoint = EP_GREEN_CHANNEL,
            .onoff = &g_onoff,
            .level = &g_lvl,
            .ch_name = CH_GREEN,
        },
        {
            .endpoint = EP_BLUE_CHANNEL,
            .onoff = &b_onoff,
            .level = &b_lvl,
            .ch_name = CH_BLUE,
        }
    };

    // Read level and on_off for each channel
    for (uint8_t i = 0; i < (sizeof(init)/sizeof(init[0])); i++) {
        if (emberAfReadServerAttribute(init[i].endpoint,
                                       ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                       ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                       (uint8_t *) (init[i].level),
                                       sizeof(uint8_t))
            != EMBER_ZCL_STATUS_SUCCESS) {
            return SL_STATUS_FAIL;
        }
        if (emberAfReadServerAttribute(init[i].endpoint,
                                       ZCL_ON_OFF_CLUSTER_ID,
                                       ZCL_ON_OFF_ATTRIBUTE_ID,
                                       (uint8_t *) (init[i].onoff),
                                       sizeof(uint8_t))
            != EMBER_ZCL_STATUS_SUCCESS) {
            return SL_STATUS_FAIL;
        }
        emberAfOnOffClusterPrintln("%d Endpoint: %d, channel: %d, on_off: %d, level: %d",
            TIMESTAMP_MS, init[i].endpoint, init[i].ch_name, (uint8_t) *(init[i].onoff), (uint8_t) *(init[i].level));
    }

    bool color_on_off = ( (r_onoff && r_lvl) || (g_onoff && g_lvl) || (b_onoff && b_lvl) );
    if ( EMBER_ZCL_STATUS_SUCCESS != emberAfWriteServerAttribute(
        EP_RGB_LIGHT,
        ZCL_ON_OFF_CLUSTER_ID,
        ZCL_ON_OFF_ATTRIBUTE_ID,
        (uint8_t *) &color_on_off,
        ZCL_BOOLEAN_ATTRIBUTE_TYPE
    )) return SL_STATUS_FAIL;

    emberAfOnOffClusterPrintln("%d Setting color on_off to %d", TIMESTAMP_MS, color_on_off);
    // ToDo
    // calculate color from RGB of individual channels

    return SL_STATUS_OK;
}

/**
 * @brief calculate and update individual channels status (on_off and level) from the
 *        color light
 */
sl_status_t llight_sync_color_light_to_channels(void)
{
    return SL_STATUS_OK;
}

// internal method implementations
/**
 * @brief helper to sync a specific light channel
 * @param[in] endpoint -- endpoint for initialization
 * @param[in] ch_name -- RGB channel name
 * @return    Status Code:
 *            - SL_STATUS_OK   Success
 *            - SL_STATUS_FAIL Error
 */
static sl_status_t _sync_light_channel(uint8_t endpoint, enum RGB_channel_name_t ch_name)
{
    uint8_t on_off, level;
    if (emberAfReadServerAttribute(endpoint,
                                   ZCL_LEVEL_CONTROL_CLUSTER_ID,
                                   ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
                                   (uint8_t *) &level,
                                   sizeof(level))
        != EMBER_ZCL_STATUS_SUCCESS) {
        sl_zigbee_app_debug_println("%d Couldn't sync level for endpoint %d", TIMESTAMP_MS, endpoint);
        return SL_STATUS_FAIL;
    }
    if (emberAfReadServerAttribute(endpoint,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (uint8_t *) &on_off,
                                   sizeof(on_off))
        != EMBER_ZCL_STATUS_SUCCESS) {
        sl_zigbee_app_debug_println("%d Couldn't sync on_off for endpoint %d", TIMESTAMP_MS, endpoint);
        return SL_STATUS_FAIL;
    }

    hw_light_set_level_ch( ch_name, level );
    if ( on_off ) {
        hw_light_turn_on_ch( ch_name );
    } else {
        hw_light_turn_off_ch( ch_name );
    }

    return SL_STATUS_OK;
}

/**
 * @brief if all the required (on_off and level) clusters were initialized, then syncrhonize the
 *        hardware state
 */
static void _sync_hardware_state(void)
{
    if ( _state.init_counters.on_off || _state.init_counters.level ) {
        sl_zigbee_app_debug_println("%d: Light is not yet fully initialized", TIMESTAMP_MS);
        return;
    }

    sl_zigbee_app_debug_println("%d: Light is initialized, sync hardware state", TIMESTAMP_MS);
    _sync_light_channel( EP_RED_CHANNEL, CH_RED );
    _sync_light_channel( EP_GREEN_CHANNEL, CH_GREEN );
    _sync_light_channel( EP_BLUE_CHANNEL, CH_BLUE );

    llight_sync_channel_light_to_color();
    _state.inhibit_updates = true;
}
