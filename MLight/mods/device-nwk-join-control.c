#include "af.h"
#include "app.h"
#include "app_button_press.h"
#include "device-nwk-join-control.h"
#include "network-steering.h"
#include "sl_zigbee_debug_print.h"

#if defined(SL_CATALOG_RZ_LED_BLINK_PRESENT)
#include "rz_led_blink.h"
#define COMMISSIONING_STATUS_LED 0
#else // !SL_CATALOG_RZ_LED_BLINK_PRESENT
#define rz_led_blink_blink_led_on(time, led)
#define rz_led_blink_blink_led_off(time, led)
#define rz_led_blink_counted(count, time, led)
#define rz_led_blink_pattern(count, length, pattern, ledIndex)
#endif // SL_CATALOG_RZ_LED_BLINK_PRESENT

#define LED_BLINK_SHORT_MS         100
#define LED_BLINK_LONG_MS          750
#define LED_BLINK_IDENTIFY_MS      500
#define LED_BLINK_NETWORK_UP_COUNT 3


typedef struct {
  bool leavingNwk;
  uint8_t  joinAttempt;         // ReJoin Attempt Number
  bool     isCurrentlySteering; // true if currently is trying to steer the network
  bool     haveNetworkToken;    // is there network token (join or rejoin)
  uint32_t currentChannel;      // current channel
} DeviceNwkJoinControl_State_t;

static DeviceNwkJoinControl_State_t dnjcState = {
    .leavingNwk = false,
    .joinAttempt = 0,
    .isCurrentlySteering = false,
    .haveNetworkToken = false,
    .currentChannel = 0,
};

//----------------
// Forward declarations
static bool writeIdentifyTime(uint16_t identifyTime);
static void startIdentifying(void);
static void stopIdentifying(void);


/**
 * @brief Stack Status callback
 */
SL_WEAK void dnjcStackStatusCb(EmberStatus status)
{
  (void) status;
}


/**
 * @brief Button press callback
 * @param button -- indicates the button that was pressed
 * @param duration -- indicates the duration of the button press as
 *        defined by the app_button_press component
 */
SL_WEAK void dnjcButtonPressCb(uint8_t button, uint8_t duration)
{
  (void) duration;
}

/**
 * @brief Callback when devices successfuly joins the network
 */
SL_WEAK void dnjcDeviceJoinedNwkCb(void)
{
}

/**
 * @brief Callback after device leaves the network
 */
SL_WEAK void dnjcDeviceLeftNwkCb(void)
{
}

//----------------------
// Implementation

/** @brief Stack Status
 *
 * This function is called by the application framework from the stack status
 * handler.  This callbacks provides applications an opportunity to be notified
 * of changes to the stack status and take appropriate action. The framework
 * will always process the stack status after the callback returns.
 */
void emberAfStackStatusCallback(EmberStatus status)
{
  EmberNetworkStatus nwkState = emberAfNetworkState();
  emberAfCorePrintln("Stack status=0x%X, nwkState=%d", status, nwkState);

  switch (nwkState) {
    case EMBER_NO_NETWORK:
      dnjcState.leavingNwk = false; // leave has completed.
      break;
    default:
      break;
  }

  dnjcStackStatusCb(status);
}


/**
 * @brief app_button_press callback
 */
void app_button_press_cb(uint8_t button, uint8_t duration)
{
  bool longPress = ( APP_BUTTON_PRESS_DURATION_LONG == duration )
                   || ( APP_BUTTON_PRESS_DURATION_VERYLONG == duration );

  if ( BUTTON0 == button
       && !(dnjcState.leavingNwk)) // Ignore button events while leaving.
  {
    EmberNetworkStatus state = dnjcIndicateNetworkState();
    if (state == EMBER_NO_NETWORK) {
      // no network, short or long press -> start network steering
      emberAfPluginNetworkSteeringStart();
    } else {
      // there's network, short or long press
      if (longPress) {
        // network & long press -> leave network
        dnjcState.leavingNwk = true;
        sl_zigbee_app_debug_println("Button- Leave Nwk");
        emberLeaveNetwork();
        emberClearBindingTable();
      } else {
        // has network, short press 
        if (state == EMBER_JOINED_NETWORK) {
          sl_zigbee_app_debug_println("Button- Identify start");
          startIdentifying();
        }
      }
    }
  }

  dnjcButtonPressCb(button, duration);
}


/** @brief Complete network steering.
 *
 * This callback is fired when the Network Steering plugin is complete.
 *
 * @param status On success this will be set to EMBER_SUCCESS to indicate a
 * network was joined successfully. On failure this will be the status code of
 * the last join or scan attempt. Ver.: always
 *
 * @param totalBeacons The total number of 802.15.4 beacons that were heard,
 * including beacons from different devices with the same PAN ID. Ver.: always
 * @param joinAttempts The number of join attempts that were made to get onto
 * an open Zigbee network. Ver.: always
 *
 * @param finalState The finishing state of the network steering process. From
 * this, one is able to tell on which channel mask and with which key the
 * process was complete. Ver.: always
 */
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                  uint8_t totalBeacons,
                                                  uint8_t joinAttempts,
                                                  uint8_t finalState)
{
  sl_zigbee_app_debug_println(
    "Network Steering Complete: status=0x%X, totalBeacons=%d, joinAttempts=%d, finalState=%d",
    status, totalBeacons, joinAttempts, finalState);
  dnjcIndicateNetworkState();
  if (status == EMBER_SUCCESS) {
    startIdentifying();
  } else {
    stopIdentifying();
  }
}

/**
 * @brief Indicate network status. Short 3 blinks is on network.
 *        Short and Long blink -- no parent. Long blink -- no network.
 * @return current network status
 */
EmberNetworkStatus dnjcIndicateNetworkState(void)
{
  EmberNetworkStatus nwkState = emberAfNetworkState();
  sl_zigbee_app_debug_println("Indicating Current Network State: %d", nwkState);

  switch (nwkState) {
    case EMBER_JOINED_NETWORK:
      rz_led_blink_counted(
        LED_BLINK_NETWORK_UP_COUNT,
        LED_BLINK_SHORT_MS,
        COMMISSIONING_STATUS_LED
      );
      break;

    case EMBER_JOINED_NETWORK_NO_PARENT:
      uint16_t pattern[] = {
        LED_BLINK_SHORT_MS, // LED On blink
        LED_BLINK_LONG_MS,  // interblink pause
        LED_BLINK_LONG_MS,  // LED On Blink
      };
      rz_led_blink_pattern(1, sizeof(pattern), pattern, COMMISSIONING_STATUS_LED);
      break;

    case EMBER_NO_NETWORK:
      rz_led_blink_blink_led_on(LED_BLINK_LONG_MS, COMMISSIONING_STATUS_LED);
      break;

    default:
      break;
  }

  return nwkState;
}

//*****************
// Static functions
//-----------------
static bool writeIdentifyTime(uint16_t identifyTime)
{
  EmberAfStatus status =
    emberAfWriteServerAttribute(emberAfPrimaryEndpoint(),
                                ZCL_IDENTIFY_CLUSTER_ID,
                                ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                                (uint8_t *)&identifyTime,
                                sizeof(identifyTime));

  return (status == EMBER_ZCL_STATUS_SUCCESS);
}

static void startIdentifying(void)
{
  writeIdentifyTime(180);
}

static void stopIdentifying(void)
{
  writeIdentifyTime(0);
}