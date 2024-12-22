#ifndef _DEVICE_NWK_JOIN_CONTROL_H_
#define _DEVICE_NWK_JOIN_CONTROL_H_

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