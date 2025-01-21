#ifndef _DEVICE_NWK_JOIN_CONTROL_H_
#define _DEVICE_NWK_JOIN_CONTROL_H_

#ifndef DNJC_STARTUP_STATUS_DELAY_MS
#define DNJC_STARTUP_STATUS_DELAY_MS 3000
#endif

#include <af-types.h>

/**
 * @brief Initialize the Device Network Join Control plugin
 *        Sets the delay to indicate the network status on startup.
 *        If the network is not up after the DNJC_STARTUP_STATUS_DELAY_MS,
 *        then initiate network steering.
 */
EmberNetworkStatus dnjcInit(void);

/**
 * @brief Indicate network status. Short 3 blinks is on network.
 *        Short and Long blink -- no parent. Long blink -- no network.
 */
EmberNetworkStatus dnjcIndicateNetworkState(void);

/**
 * @brief Weak definition of emberAfStackStatusCallback proxy.
 *        This function is called by the application framework from the stack status
 *        callback.
 */
void dnjcStackStatusCb(EmberStatus status);

/**
 * @brief Callback for device button control press
 */
void dnjcButtonPressCb(uint8_t button, uint8_t duration);

/**
 * @brief Callback when devices successfuly joins the network
 */
void dnjcDeviceJoinedNwkCb(void);

/**
 * @brief Callback after device leaves the network
 */
void dnjcDeviceLeftNwkCb(void);

#endif // _DEVICE_NWK_JOIN_CONTROL_H_