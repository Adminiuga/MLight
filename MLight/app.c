/***************************************************************************//**
 * @file app.c
 * @brief Callbacks implementation and application specific code.
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include PLATFORM_HEADER
#include "hal.h"
#include "ember.h"
#include "af.h"
#ifdef SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "sl_zigbee_debug_print.h"
#endif // SL_CATALOG_ZIGBEE_DEBUG_PRINT_PRESENT
#include "app/framework/util/af-main.h"
#if SL_POWER_MANAGER_DEBUG == 1
#include "sl_power_manager_debug.h"
#endif // SL_POWER_MANAGER_DEBUG == 1

#include "app/framework/plugin/reporting/reporting.h"

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif
#ifdef SL_CATALOG_ZIGBEE_BLE_EVENT_HANDLER_PRESENT
#include "sl_ble_event_handler.h"
#endif

#include "app.h"
#include "light/logical_light.h"
#include "mods/rz_button_press.h"

#include "sl_dmp_ui_stub.h"

#include "sl_simple_led_instances.h"
#if defined(SL_CATALOG_RZ_LED_BLINK_PRESENT)
#include <rz_led_blink.h>
#include "mods/device-nwk-join-control.h"
#endif // SL_CATALOG_RZ_LED_BLINK_PRESENT

#define SOURCE_ADDRESS_LEN 8
static EmberEUI64 SwitchEUI;
static bool identifying = false;

//---------------------
// Forward declarations

static void setDefaultReportEntry(void);

void emberAfMainTickCallback()
{
}

void dnjcButtonPressCb(uint8_t button, rz_button_press_status_t duration)
{
  sl_zigbee_app_debug_print("Button- %d, duration: ", button);
  switch ( duration ) {
    case RZ_BUTTON_PRESS_BUTTON_IS_RELEASED:
      sl_zigbee_app_debug_println("is released");
      break;

    case RZ_BUTTON_PRESS_RELEASED_SHORT:
      sl_zigbee_app_debug_println("is released after a short press");
      if ( BUTTON0 == button ) {
         emberAfOnOffClusterSetValueCallback(emberAfPrimaryEndpoint(),
                                             ZCL_TOGGLE_COMMAND_ID,
                                             0);
      }
      break;

    case RZ_BUTTON_PRESS_RELEASED_MEDIUM:
      sl_zigbee_app_debug_println("is released after a medium press");
      break;

    case RZ_BUTTON_PRESS_RELEASED_LONG:
      sl_zigbee_app_debug_println("is released after a long press");
      break;

    case RZ_BUTTON_PRESS_RELEASED_VERYLONG:
      sl_zigbee_app_debug_println("is released after a very long press");
      break;

    case RZ_BUTTON_PRESS_BUTTON_IS_PRESSED:
      sl_zigbee_app_debug_println("is pressed");
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_SHORT:
      sl_zigbee_app_debug_println("is still pressed for short duration");
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_MEDIUM:
      sl_zigbee_app_debug_println("is still pressed for long duration");
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_LONG:
      sl_zigbee_app_debug_println("is still pressed for long duration");
      break;

    case RZ_BUTTON_PRESS_STILL_PRESSED_VERYLONG:
      sl_zigbee_app_debug_println("is still pressed for very long duration");
      break;

    default:
      sl_zigbee_app_debug_println("unknown button press event");
      break;
  }
}

/**
 * @brief this is called by the Device Network Join Control plugin
 *        when the device successfully joins the network.
 */
void dnjcDeviceJoinedNwkCb(void)
{
  setDefaultReportEntry();
}

//----------------------
// Implemented Callbacks

/** @brief Init
 * Application init function
 */
void emberAfMainInitCallback(void)
{
  #if defined(SL_CATALOG_RZ_LED_BLINK_PRESENT)
  rz_led_blink_init();
  #if SL_SIMPLE_LED_COUNT >= 1
  rz_led_blink_blink_led_on( 500, 0 );
  #endif
  #endif // SL_CATALOG_RZ_LED_BLINK_PRESENT
  #if SL_POWER_MANAGER_DEBUG == 1
  sl_power_manager_debug_print_em_requirements();
  #endif // SL_POWER_MANAGER_DEBUG == 1
  dnjcInit();
  rz_button_press_init();
}

/** @brief Start feedback.
 *
 * This function is called by the Identify plugin when identification begins.
 * It informs the Identify Feedback plugin that it should begin providing its
 * implemented feedback functionality (e.g., LED blinking, buzzer sounding,
 * etc.) until the Identify plugin tells it to stop. The identify time is
 * purely a matter of informational convenience. This plugin does not need to
 * know how long it will identify (the Identify plugin will perform the
 * necessary timekeeping.)
 *
 * @param endpoint The identifying endpoint Ver.: always
 * @param identifyTime The identify time Ver.: always
 */
void emberAfPluginIdentifyStartFeedbackCallback(uint8_t endpoint,
                                                uint16_t identifyTime)
{
  if (identifyTime > 0) {
    identifying = true;
    emberAfAppPrintln("Start Identifying for %dS", identifyTime);
    emberAfSetDefaultPollControlCallback(EMBER_AF_SHORT_POLL);  // Use short poll while identifying.
  }
}

/** @brief Stop feedback.
 *
 * This function is called by the Identify plugin when identification is
 * finished. It tells the Identify Feedback plugin to stop providing its
 * implemented feedback functionality.
 *
 * @param endpoint The identifying endpoint Ver.: always
 */
void emberAfPluginIdentifyStopFeedbackCallback(uint8_t endpoint)
{
  if (identifying) {
    identifying = false;
    emberAfAppPrintln("Stop Identifying");
    emberAfSetDefaultPollControlCallback(EMBER_AF_LONG_POLL); // Revert to long poll when we stop identifying.
  }
}


/** @brief Post Attribute Change
 *
 * This function is called by the application framework after it changes an
 * attribute value. The value passed into this callback is the value to which
 * the attribute was set by the framework.
 */
void emberAfPostAttributeChangeCallback(uint8_t endpoint,
                                        EmberAfClusterId clusterId,
                                        EmberAfAttributeId attributeId,
                                        uint8_t mask,
                                        uint16_t manufacturerCode,
                                        uint8_t type,
                                        uint8_t size,
                                        uint8_t* value)
{
  sl_zigbee_app_debug_println("%d Post attribute change on %d ep, cluster: %d, attribute id: %d, value: %d",
      TIMESTAMP_MS, endpoint, clusterId, attributeId, (uint8_t) *(value)
  );
  if ( mask != CLUSTER_MASK_SERVER ) return; // we only process server attributes

#if !defined(SL_CATALOG_ZIGBEE_LEVEL_CONTROL_PRESENT)
  if (clusterId == ZCL_ON_OFF_CLUSTER_ID
      && attributeId == ZCL_ON_OFF_ATTRIBUTE_ID) {
    if (value[0] == 0x00) {
        llight_turnoff_light(endpoint);
#ifdef SL_CATALOG_ZIGBEE_BLE_EVENT_HANDLER_PRESENT
        zb_ble_dmp_notify_light(DMP_UI_LIGHT_OFF);
#endif
      } else {
        llight_turnon_light(endpoint);
#ifdef SL_CATALOG_ZIGBEE_BLE_EVENT_HANDLER_PRESENT
        zb_ble_dmp_notify_light(DMP_UI_LIGHT_ON);
#endif
      }
      if ((sl_dmp_ui_get_light_direction() == DMP_UI_DIRECTION_BLUETOOTH)
          || (sl_dmp_ui_get_light_direction() == DMP_UI_DIRECTION_SWITCH)) {
        sl_dmp_ui_update_direction(sl_dmp_ui_get_light_direction());
      } else {
        sl_dmp_ui_update_direction(DMP_UI_DIRECTION_ZIGBEE);
#ifdef SL_CATALOG_ZIGBEE_BLE_EVENT_HANDLER_PRESENT
        zb_ble_dmp_set_source_address(SwitchEUI);
#endif
      }
      sl_dmp_ui_set_light_direction(DMP_UI_DIRECTION_INVALID);
  }
#else
  if ( clusterId == ZCL_LEVEL_CONTROL_CLUSTER_ID
      && attributeId == ZCL_CURRENT_LEVEL_ATTRIBUTE_ID ) {
      llight_set_level( endpoint, value[0] );
  } else if ( ZCL_LEVEL_CONTROL_REMAINING_TIME_ATTRIBUTE_ID == attributeId ) {
    // Handle light state update depending on the remaining time attribute
    uint8_t onOff;
    if ( emberAfReadServerAttribute(endpoint,
                                    ZCL_ON_OFF_CLUSTER_ID,
                                    ZCL_ON_OFF_ATTRIBUTE_ID,
                                    (uint8_t *) &onOff,
                                    sizeof(onOff)) != EMBER_ZCL_STATUS_SUCCESS ) {
      emberAfAppPrintln("Couldn't read current 'on/off' state: %s, forcing light off");
      llight_turnoff_light(endpoint);
      return;
    }
    if ( onOff || ( (value[0] == 0) && (value[1] == 0) ) ) {
      emberAfOnOffClusterPrintln("Turning light %s on %d endpoint", (onOff ? "on" : "off"), endpoint);
      llight_turnonoff_light(endpoint, onOff);
    }
  }
#endif
}

/** @brief Pre Command Received
 *
 * This callback is the second in the Application Framework's message processing
 * chain. At this point in the processing of incoming over-the-air messages, the
 * application has determined that the incoming message is a ZCL command. It
 * parses enough of the message to populate an EmberAfClusterCommand struct. The
 * Application Framework defines this struct value in a local scope to the
 * command processing but also makes it available through a global pointer
 * called emberAfCurrentCommand, in app/framework/util/util.c. When command
 * processing is complete, this pointer is cleared.
 */
bool emberAfPreCommandReceivedCallback(EmberAfClusterCommand* cmd)
{
  if ((cmd->commandId == ZCL_ON_COMMAND_ID)
      || (cmd->commandId == ZCL_OFF_COMMAND_ID)
      || (cmd->commandId == ZCL_TOGGLE_COMMAND_ID)) {
    (void) memset(SwitchEUI, 0, SOURCE_ADDRESS_LEN);
    emberLookupEui64ByNodeId(cmd->source, SwitchEUI);
    emberAfCorePrintln(
      "SWITCH ZCL toggle/on/off EUI [%02X %02X %02X %02X %02X %02X %02X %02X]",
      SwitchEUI[7],
      SwitchEUI[6],
      SwitchEUI[5],
      SwitchEUI[4],
      SwitchEUI[3],
      SwitchEUI[2],
      SwitchEUI[1],
      SwitchEUI[0]);
  }
  return false;
}

/** @brief Trust Center Join
 *
 * This callback is called from within the application framework's
 * implementation of emberTrustCenterJoinHandler or ezspTrustCenterJoinHandler.
 * This callback provides the same arguments passed to the
 * TrustCenterJoinHandler. For more information about the TrustCenterJoinHandler
 * please see documentation included in stack/include/trust-center.h.
 */
void emberAfTrustCenterJoinCallback(EmberNodeId newNodeId,
                                    EmberEUI64 newNodeEui64,
                                    EmberNodeId parentOfNewNode,
                                    EmberDeviceUpdate status,
                                    EmberJoinDecision decision)
{
  if (status == EMBER_DEVICE_LEFT) {
    for (uint8_t i = 0; i < EMBER_BINDING_TABLE_SIZE; i++) {
      EmberBindingTableEntry entry;
      emberGetBinding(i, &entry);
      if ((entry.type == EMBER_UNICAST_BINDING)
          && (entry.clusterId == ZCL_ON_OFF_CLUSTER_ID)
          && ((MEMCOMPARE(entry.identifier, newNodeEui64, EUI64_SIZE)
               == 0))) {
        emberDeleteBinding(i);
        emberAfAppPrintln("deleted binding entry: %d", i);
        break;
      }
    }
  }
}

/** @brief
 *
 * Application framework equivalent of ::emberRadioNeedsCalibratingHandler
 */
void emberAfRadioNeedsCalibratingCallback(void)
{
  sl_mac_calibrate_current_channel();
}

//-----------------
// Static functions
static void setDefaultReportEntry(void)
{
  EmberAfPluginReportingEntry reportingEntry;
  emberAfClearReportTableCallback();
  reportingEntry.direction = EMBER_ZCL_REPORTING_DIRECTION_REPORTED;
  reportingEntry.endpoint = emberAfPrimaryEndpoint();
  reportingEntry.clusterId = ZCL_ON_OFF_CLUSTER_ID;
  reportingEntry.attributeId = ZCL_ON_OFF_ATTRIBUTE_ID;
  reportingEntry.mask = CLUSTER_MASK_SERVER;
  reportingEntry.manufacturerCode = EMBER_AF_NULL_MANUFACTURER_CODE;
  reportingEntry.data.reported.minInterval = 0x0001;
  reportingEntry.data.reported.maxInterval = 0x001E; // 30S report interval for SED.
  reportingEntry.data.reported.reportableChange = 0; // onoff is bool type so it is unused
  emberAfPluginReportingConfigureReportedAttribute(&reportingEntry);
}
