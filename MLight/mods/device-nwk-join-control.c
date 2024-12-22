#include "af.h"
#include "app_button_press.h"
#include "device-nwk-join-control.h"
#include "app.h"

typedef struct {
    bool leavingNwk;
} DeviceNwkJoinControl_State_t;

static DeviceNwkJoinControl_State_t dnjcState = {
    .leavingNwk = false
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
    EmberNetworkStatus state = emberAfNetworkState();
    if (state == EMBER_NO_NETWORK) {
      // no network, short or long press -> start network steering
      emberAfPluginNetworkSteeringStart();
    } else {
      // there's network, short or long press
      if (longPress) {
        // network & long press -> leave network
        dnjcState.leavingNwk = true;
        emberAfAppPrintln("Button- Leave Nwk");
        emberLeaveNetwork();
        emberClearBindingTable();
      } else {
        // has network, short press 
        if (state == EMBER_JOINED_NETWORK) {
          emberAfAppPrintln("Button- Identify start");
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
  if (status == EMBER_SUCCESS) {
    startIdentifying();
  } else {
    stopIdentifying();
  }
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