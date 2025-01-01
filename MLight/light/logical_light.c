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
    bool external_updates_disabled;
} Llight_state_t;

static Llight_state_t _state = {
    .init_counters = {
        .on_off = CLUSTERS_TO_INIT_ON_OFF,
        .level = CLUSTERS_TO_INIT_LEVEL
    },
    .external_updates_disabled = true
};


// ******************************************
// Forward declarations for private functions
static void _sync_hardware_state(void);
static sl_status_t _sync_light_channel(uint8_t endpoint, enum RGB_channel_name_t ch_name);
static sl_status_t _sync_channel_light_to_color(void);
static sl_status_t _sync_color_light_to_channels(void);
static sl_status_t _turn_onoff_light(uint8_t endpoint, bool turn_on);


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
 * @brief ignore update to the light state based on post attribute changes, this allows the
 *        other method to update the "linked state", e.g. red/green/blue channel lights to RGB
 *        light
 */
void llight_disable_external_updates(void)
{
    _state.external_updates_disabled = true;
}

/**
 * @brief enabled update to the light state based on post attribute changes, this allows the
 *        other method to update the "linked state", e.g. red/green/blue channel lights to RGB
 *        light
 */
void llight_enable_external_updates(void)
{
    _state.external_updates_disabled = false;
}

/**
 * @brief turn on the light RGB or individual channel based on the endpoint. The method
 *        would take care updating the linked state lights.
 */
sl_status_t llight_turnon_light(uint8_t endpoint)
{
    if ( _state.external_updates_disabled ) return SL_STATUS_OK;
    llight_disable_external_updates();

    sl_status_t status = _turn_onoff_light(endpoint, true);

    llight_enable_external_updates();
    return status;
}

/**
 * @brief turn off the light RGB or individual channel based on the endpoint. The method
 *        would take care updating the linked state lights.
 */
sl_status_t llight_turnoff_light(uint8_t endpoint)
{
    if ( _state.external_updates_disabled ) return SL_STATUS_OK;
    llight_disable_external_updates();

    sl_status_t status = _turn_onoff_light(endpoint, false);

    llight_enable_external_updates();
    return status;
}

// *****************************
// internal method implementations
// -----------------------------
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

    _sync_channel_light_to_color();
    _state.external_updates_disabled = false;
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
 * @brief calculate and update individual channels status (on_off and level) from the
 *        color light
 */
static sl_status_t _sync_color_light_to_channels(void)
{
    uint8_t onoff;
    // Get RGB light on_off
    if (emberAfReadServerAttribute(EP_RGB_LIGHT,
                                   ZCL_ON_OFF_CLUSTER_ID,
                                   ZCL_ON_OFF_ATTRIBUTE_ID,
                                   (uint8_t *) &onoff,
                                   sizeof(uint8_t))
        != EMBER_ZCL_STATUS_SUCCESS) {
        return SL_STATUS_FAIL;
    }

    uint8_t ch_endpoints[] = {EP_RED_CHANNEL, EP_GREEN_CHANNEL, EP_BLUE_CHANNEL};

    for (uint8_t i = 0; i < (sizeof(ch_endpoints)/sizeof(ch_endpoints[0])); i++) {
        if ( EMBER_ZCL_STATUS_SUCCESS != emberAfWriteServerAttribute(
            ch_endpoints[i],
            ZCL_ON_OFF_CLUSTER_ID,
            ZCL_ON_OFF_ATTRIBUTE_ID,
            (uint8_t *) &onoff,
            ZCL_BOOLEAN_ATTRIBUTE_TYPE
        )) return SL_STATUS_FAIL;
        emberAfOnOffClusterPrintln("%d Setting CH %d on_off to %d", TIMESTAMP_MS, ch_endpoints[i], onoff);
    }

    // ToDo sync color to channel levels
    return SL_STATUS_OK;
}

/**
 * @brief Sync color light state from individual channel lights
 *        this is required when operating on individual channel/lights
 */
static sl_status_t _sync_channel_light_to_color(void)
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
 * @brief handle light turn on/off
 * @param[in] endpoint
 * @param[in] turn_on -- true to turn on the light, false to turn off
 */
sl_status_t _turn_onoff_light(uint8_t endpoint, bool turn_on)
{
    sl_status_t state = SL_STATUS_OK;
    switch ( endpoint ) {
        case EP_RED_CHANNEL:
            state = hw_light_turn_ch_onoff( CH_RED, turn_on );
            _sync_channel_light_to_color();
            break; 

        case EP_GREEN_CHANNEL:
            state = hw_light_turn_ch_onoff( CH_GREEN, turn_on );
            _sync_channel_light_to_color();
            break; 

        case EP_BLUE_CHANNEL:
            state = hw_light_turn_ch_onoff( CH_BLUE, turn_on );
            _sync_channel_light_to_color();
            break; 

        case EP_RGB_LIGHT:
            if ( turn_on ) {
                hw_light_turnon();
            } else {
                hw_light_turnoff();
            }
            _sync_color_light_to_channels();
            break; 

        default:
            state = SL_STATUS_FAIL;
            break;
    }

    return state;
}
