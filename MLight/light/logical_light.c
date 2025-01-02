#include <math.h>
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
static sl_status_t _sync_color_brightness_to_channels( uint8_t level );
static EmberAfStatus _rgb_from_xy_and_brightness(uint8_t *red, uint8_t *green, uint8_t *blue);
static sl_status_t _turn_onoff_light(uint8_t endpoint, bool turn_on);
static sl_status_t _update_xy_color_from_rgb(uint8_t red, uint8_t green, uint8_t blue);


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

/** @brief Compute Pwm from HSV
 *
 * This function is called from the color server when it is time for the PWMs to
 * be driven with a new value from the HSV values.
 *
 * @param endpoint The identifying endpoint Ver.: always
 */
void emberAfPluginColorControlServerComputePwmFromHsvCallback(uint8_t endpoint)
{
    if ( _state.external_updates_disabled ) return;
    llight_disable_external_updates();
    emberAfColorControlClusterPrintln("%d Updating temperature from HSV", TIMESTAMP_MS);


    llight_enable_external_updates();
}

/** @brief Compute Pwm from HSV
 *
 * This function is called from the color server when it is time for the PWMs to
 * be driven with a new value from the color X and color Y values.
 *
 * @param endpoint The identifying endpoint Ver.: always
 */
void emberAfPluginColorControlServerComputePwmFromXyCallback(uint8_t endpoint)
{
    if ( _state.external_updates_disabled || (EP_RGB_LIGHT != endpoint) ) return;
    llight_disable_external_updates();

    emberAfColorControlClusterPrintln("%d Updating RGB from XY", TIMESTAMP_MS);
    _sync_color_brightness_to_channels(endpoint);

    llight_enable_external_updates();
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


/**
 * @brief set level for the light
 */
sl_status_t llight_set_level(uint8_t endpoint, uint8_t level)
{
    if ( _state.external_updates_disabled ) return SL_STATUS_OK;
    llight_disable_external_updates();

    sl_status_t status = SL_STATUS_FAIL;

    switch ( endpoint ) {
        case EP_RED_CHANNEL:
            status = hw_light_set_level_ch( CH_RED, level );
            _sync_channel_light_to_color();
            break;

        case EP_GREEN_CHANNEL:
            status = hw_light_set_level_ch( CH_GREEN, level );
            _sync_channel_light_to_color();
            break;

        case EP_BLUE_CHANNEL:
            status = hw_light_set_level_ch( CH_BLUE, level );
            _sync_channel_light_to_color();
            break;

        case EP_RGB_LIGHT:
            status = _sync_color_brightness_to_channels( level );
            break;

        default:
            break;
    }

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

    _update_xy_color_from_rgb( r_lvl, g_lvl, b_lvl );

    bool color_on_off = r_onoff || g_onoff || b_onoff;
    if ( EMBER_ZCL_STATUS_SUCCESS != emberAfWriteServerAttribute(
        EP_RGB_LIGHT,
        ZCL_ON_OFF_CLUSTER_ID,
        ZCL_ON_OFF_ATTRIBUTE_ID,
        (uint8_t *) &color_on_off,
        ZCL_BOOLEAN_ATTRIBUTE_TYPE
    )) return SL_STATUS_FAIL;

    emberAfOnOffClusterPrintln("%d Setting color on_off to %d", TIMESTAMP_MS, color_on_off);

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

/**
 * @brief recalculate RGB from XY, normalize to brighntess and update channels
 */
static sl_status_t _sync_color_brightness_to_channels(uint8_t endpoint)
{
    struct {
        uint8_t ep;
        enum RGB_channel_name_t chname;
        uint8_t level;
    } levels[] = {
        {.ep = EP_RED_CHANNEL, .chname = CH_RED, },
        {.ep = EP_GREEN_CHANNEL, .chname = CH_GREEN, },
        {.ep = EP_BLUE_CHANNEL, .chname = CH_BLUE }
    };

    sl_status_t status;
    status = _rgb_from_xy_and_brightness( &(levels[0].level), &(levels[1].level), &(levels[2].level) );
    if ( SL_STATUS_OK != status ) return status;

    for ( uint8_t i = 0; i < (sizeof( levels )/sizeof( levels[0] )); i++) {
        status |= hw_light_set_level_ch( levels[i].chname, levels[i].level );
        if ( EMBER_ZCL_STATUS_SUCCESS != emberAfWriteServerAttribute(
            levels[i].ep, ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
            &(levels[i].level),
            ZCL_INT8U_ATTRIBUTE_TYPE
        )) status |= SL_STATUS_FAIL;
    }

    return status;
}

/**
 * @brief calclulate rgb from X & Y color, normilized to current brightness
 */
static EmberAfStatus _rgb_from_xy_and_brightness(uint8_t *red, uint8_t *green, uint8_t *blue) {
    uint16_t color_x, color_y;
    uint8_t level;

    if ( EMBER_ZCL_STATUS_SUCCESS != emberAfReadServerAttribute(
            EP_RGB_LIGHT, ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID,
            (uint8_t *) &color_x, sizeof(color_x)
    )) return SL_STATUS_FAIL;
    if ( EMBER_ZCL_STATUS_SUCCESS != emberAfReadServerAttribute(
            EP_RGB_LIGHT, ZCL_COLOR_CONTROL_CLUSTER_ID, ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID,
            (uint8_t *) &color_y, sizeof(color_y)
    )) return SL_STATUS_FAIL;
    if ( EMBER_ZCL_STATUS_SUCCESS != emberAfReadServerAttribute(
            EP_RGB_LIGHT, ZCL_LEVEL_CONTROL_CLUSTER_ID, ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
            &level, sizeof(level)
    )) return SL_STATUS_FAIL;

    sl_zigbee_app_debug_println("Current x,y is (0x%x, 0x%x), level: %d (int)", color_x, color_y, level);

    float x = color_x / 65535.0f;
    float y = color_y / 65535.0f;
    float z = 1.0f - x - y;

    float Y = ( 1.0f * level) / 255.0f ; // The reference white point luminance
    sl_zigbee_app_debug_println("Current x,y is (%.5f, %.5f), luminance: %f (float)", x, y, Y);
    float X = (Y / y) * x;
    float Z = (Y / y) * z;

    float r_linear = X * 1.656492f - Y * 0.354851f - Z * 0.255038f;
    float g_linear = -X * 0.707196f + Y * 1.655397f + Z * 0.036152f;
    float b_linear = X * 0.051713f - Y * 0.121364f + Z * 1.011530f;
    // Convert to linear RGB
    // r_linear = X * 3.2406f - Y * 1.5372f - Z * 0.4986f;
    // g_linear = -X * 0.9689f + Y * 1.8758f + Z * 0.0415f;
    // b_linear = X * 0.0557f - Y * 0.2040f + Z * 1.0570f;


    // Apply gamma correction to convert from linear to sRGB color space
    float gamma = 2.4f;
    float a = 0.055f;
    float r = (r_linear <= 0.0031308f) ? 12.92f*r_linear : (1.0f+a)*pow(r_linear, 1.0/gamma) - a;
    float g = (g_linear <= 0.0031308f) ? 12.92f*g_linear : (1.0f+a)*pow(g_linear, 1.0/gamma) - a;
    float b = (b_linear <= 0.0031308f) ? 12.92f*b_linear : (1.0f+a)*pow(b_linear, 1.0/gamma) - a;

    if (r < 0) {
        r = 0;
    } else if (r > 1) {
        r = 1;
    }
    if (g < 0) {
        g = 0;
    } else if (g > 1) {
        g = 1;
    }
    if (b < 0) {
        b = 0;
    } else if (b > 1) {
        b = 1;
    }

    *red = (uint8_t) round( 255.0f * r * Y );
    *green = (uint8_t) round( 255.0f * g * Y );
    *blue = (uint8_t) round( 255.0f * b * Y );
    sl_zigbee_app_debug_println("Calculated RGB: %d/%d/%d)",
        (uint8_t) *red, (uint8_t) *blue, (uint8_t) *blue);
    return SL_STATUS_OK;
}

/**
 * @brief calculate and update XY color & level on the Color light endpoint, based
 *        on rgb values (levels for EP 2, 3 and 4)
 */
sl_status_t _update_xy_color_from_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    // Normalize the RGB values
    float r = red / 255.0f;
    float g = green / 255.0f;
    float b = blue / 255.0f;

    // Apply gamma correction
    r = (r > 0.04045) ? powf((r + 0.055) / 1.055, 2.4) : (r / 12.92);
    g = (g > 0.04045) ? powf((g + 0.055) / 1.055, 2.4) : (g / 12.92);
    b = (b > 0.04045) ? powf((b + 0.055) / 1.055, 2.4) : (b / 12.92);

    // Convert to XYZ color space
    float X = r * 0.4124 + g * 0.3576 + b * 0.1805;
    float Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    float Z = r * 0.0193 + g * 0.1192 + b * 0.9505;

    // Convert to xy color space
    float x = X / (X + Y + Z);
    float y = Y / (X + Y + Z);

    // Calculate brightness
    uint8_t brightness = round(Y * 255.0f);
    uint16_t color_x = round( x * 65535.0f );
    uint16_t color_y = round( y * 65535.0f );

    emberAfWriteServerAttribute(
        EP_RGB_LIGHT,
        ZCL_LEVEL_CONTROL_CLUSTER_ID,
        ZCL_CURRENT_LEVEL_ATTRIBUTE_ID,
        &brightness,
        ZCL_INT8U_ATTRIBUTE_TYPE
    );
    emberAfWriteServerAttribute(
        EP_RGB_LIGHT,
        ZCL_COLOR_CONTROL_CLUSTER_ID,
        ZCL_COLOR_CONTROL_CURRENT_X_ATTRIBUTE_ID,
        (uint8_t *) &color_x,
        ZCL_INT16U_ATTRIBUTE_TYPE
    );
    emberAfWriteServerAttribute(
        EP_RGB_LIGHT,
        ZCL_COLOR_CONTROL_CLUSTER_ID,
        ZCL_COLOR_CONTROL_CURRENT_Y_ATTRIBUTE_ID,
        (uint8_t *) &color_y,
        ZCL_INT16U_ATTRIBUTE_TYPE
    );

    sl_zigbee_app_debug_println("Updated RGB EP color_x: %d, color_y: %d, level: %d from RGB %d/%d/%d",
        color_x, color_y, brightness,
        red, green, blue
    );

    return SL_STATUS_OK;
}
