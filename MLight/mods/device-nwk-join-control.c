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
#define LED_BLINK_NETWORK_UP_COUNT 5

enum uint8_t {
  DNJC_SM_IDLE = 0,
  DNJC_SM_INDICATE_STARTUP_NWK,
  DNJC_SM_INDICATE_LEAVING_NWK,
  DNJC_SM_INDICATE_START_STEERING
};

typedef struct {
  bool isInitialized;
  bool leavingNwk;
  uint8_t  joinAttempt;         // ReJoin Attempt Number
  bool     isCurrentlySteering; // true if currently is trying to steer the network
  bool     haveNetworkToken;    // is there network token (join or rejoin)
  uint32_t currentChannel;      // current channel
  sl_zigbee_event_t dnjcEvent;  // event for the Device Network Join Control, used to indicate status on power on
  uint8_t smState;              // state machine state
  void (*smPostTransition) (void); // step to execute for state machine transition
} DeviceNwkJoinControl_State_t;

static DeviceNwkJoinControl_State_t dnjcState = {
    .isInitialized = false,
    .leavingNwk = false,
    .joinAttempt = 0,
    .isCurrentlySteering = false,
    .haveNetworkToken = false,
    .currentChannel = 0,
    .smState = DNJC_SM_IDLE,
    .smPostTransition = NULL,
};

//----------------
// Forward declarations
static bool writeIdentifyTime(uint16_t identifyTime);
static void startIdentifying(void);
static void stopIdentifying(void);
static void _event_handler(sl_zigbee_event_t *event);
static void _event_state_indicate_startup_nwk(void);
static void _indicate_leaving_nwk(void);
static void _post_indicate_leaving_nwk(void);
static void _indicate_start_steering_nwk(void);
static void _post_indicate_start_steering_nwk(void);


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
  sl_zigbee_app_debug_println("%d (dnjc) Stack status=0x%X, nwkState=%d", TIMESTAMP_MS, status, nwkState);

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
      dnjcState.isCurrentlySteering = true;
      emberAfPluginNetworkSteeringStart();
    } else {
      // there's network, short or long press
      if (longPress) {
        // network & long press -> leave network
        sl_zigbee_app_debug_println("Button- Leave Nwk");
        _indicate_leaving_nwk();
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
  dnjcState.isCurrentlySteering = false;
  if (status == EMBER_SUCCESS) {
    startIdentifying();
  } else {
    stopIdentifying();
  }
}

/**
 * @brief Initialize the Device Network Join Control plugin
 *        Sets the delay to indicate the network status on startup.
 *        If the network is not up after the DNJC_STARTUP_STATUS_DELAY_MS,
 *        then initiate network steering.
 */
EmberNetworkStatus dnjcInit(void)
{
  if ( ! dnjcState.isInitialized ) {
    dnjcState.isInitialized = true;
    sl_zigbee_event_init(&dnjcState.dnjcEvent, _event_handler);
    sl_zigbee_event_set_delay_ms(&dnjcState.dnjcEvent, DNJC_STARTUP_STATUS_DELAY_MS);
    dnjcState.smState = DNJC_SM_INDICATE_STARTUP_NWK;
    dnjcState.smPostTransition = _event_state_indicate_startup_nwk;
  }
  return SL_STATUS_OK;
}

/**
 * @brief Indicate network status. Short 3 blinks is on network.
 *        Short and Long blink -- no parent. Long blink -- no network.
 * @return current network status
 */
EmberNetworkStatus dnjcIndicateNetworkState(void)
{
  EmberNetworkStatus nwkState = emberAfNetworkState();
  sl_zigbee_app_debug_println("%d Indicating Current Network State: %d", TIMESTAMP_MS, nwkState);

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
      rz_led_blink_pattern(1, sizeof(pattern)/sizeof(pattern[0]), pattern, COMMISSIONING_STATUS_LED);
      break;

    case EMBER_NO_NETWORK:
      rz_led_blink_blink_led_on(LED_BLINK_LONG_MS, COMMISSIONING_STATUS_LED);
      break;

    default:
      rz_led_blink_counted(2, LED_BLINK_LONG_MS, COMMISSIONING_STATUS_LED);
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

/**
 * @brief Device Network Join Control event handler. For now, just going to be called
 *        once upon startup, to indicate status. If the network is not up, then start network steering.
 */
void _event_handler(sl_zigbee_event_t *event)
{
  sl_zigbee_event_set_inactive(&dnjcState.dnjcEvent);

  // for non-sleepy end devices, set shorter polling intervals
#if SLI_ZIGBEE_PRIMARY_NETWORK_DEVICE_TYPE == SLI_ZIGBEE_NETWORK_DEVICE_TYPE_END_DEVICE && defined(SL_CATALOG_ZIGBEE_END_DEVICE_SUPPORT_PRESENT)
  uint32_t pollMs;
  pollMs = emberAfGetShortPollIntervalMsCallback();
  pollMs = pollMs >> 2;
  if ( pollMs ) {
    emberAfSetShortPollIntervalMsCallback( pollMs );
    sl_zigbee_app_debug_println("%d Setting short poll to %dms (non-sleepy ED)", TIMESTAMP_MS, pollMs);
  }

  pollMs = emberAfGetLongPollIntervalMsCallback();
  pollMs = pollMs >> 2;
  if ( pollMs ) {
    emberAfSetLongPollIntervalMsCallback( pollMs );
    sl_zigbee_app_debug_println("%d Setting long poll to %dms (non-sleepy ED)", TIMESTAMP_MS, pollMs);
  }
#endif // SLI_ZIGBEE_PRIMARY_NETWORK_DEVICE_TYPE

  if ( dnjcState.smPostTransition ) {
    dnjcState.smPostTransition();
  }
}

void _event_state_indicate_startup_nwk(void)
{
  dnjcState.smState = DNJC_SM_IDLE;
  dnjcState.smPostTransition = NULL;

  EmberNetworkStatus nwkState = emberAfNetworkState();

  if ( EMBER_JOINED_NETWORK == nwkState
       || EMBER_JOINED_NETWORK_NO_PARENT == nwkState ) {
    sl_zigbee_app_debug_println("%d dnjc startup nwk status", TIMESTAMP_MS);
    dnjcIndicateNetworkState();
  } else {
    // should not be leaving, but if we are, then reschedule
    if ( dnjcState.leavingNwk ) {
      sl_zigbee_event_set_delay_ms(&dnjcState.dnjcEvent, DNJC_STARTUP_STATUS_DELAY_MS >> 1);
      dnjcState.smState = DNJC_SM_INDICATE_STARTUP_NWK;
      dnjcState.smPostTransition = _event_state_indicate_startup_nwk;
    } else {
      _indicate_start_steering_nwk();
    }
  }
}

/**
 * @brief indicates the intent to leave the network and after the indications is complete
 *        leaves the network in the post transition step.
 */
void _indicate_leaving_nwk(void)
{
  rz_led_blink_blink_led_on(LED_BLINK_LONG_MS, COMMISSIONING_STATUS_LED);
  dnjcState.smPostTransition = _post_indicate_leaving_nwk;
  dnjcState.smState = DNJC_SM_INDICATE_LEAVING_NWK;
  sl_zigbee_event_set_inactive(&dnjcState.dnjcEvent);
  sl_zigbee_event_set_delay_ms(&dnjcState.dnjcEvent, LED_BLINK_LONG_MS);
}

/**
 * @brief post transition step after indicating the intent to leave the network.
 */
void _post_indicate_leaving_nwk(void)
{
  dnjcState.smState = DNJC_SM_IDLE;
  dnjcState.smPostTransition = NULL;
  dnjcState.leavingNwk = true;
  sl_zigbee_app_debug_println("%d Leaving Network", TIMESTAMP_MS);
  emberLeaveNetwork();
  emberClearBindingTable();
  emberAfClearReportTableCallback();
}

/**
 * @brief indicates the intent to start steering the network and starts
 *        steering. Since steering takes a bit, no neeed to wait for the
 *        indication to complete.
 */
void _indicate_start_steering_nwk(void)
{
  rz_led_blink_blink_led_on(LED_BLINK_LONG_MS << 1, COMMISSIONING_STATUS_LED);
  dnjcState.isCurrentlySteering = true;
  emberAfPluginNetworkSteeringStart();
}

void _post_indicate_start_steering_nwk(void)
{
  dnjcState.smState = DNJC_SM_IDLE;
  dnjcState.smPostTransition = NULL;
}
