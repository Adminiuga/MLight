/***************************************************************************//**
 * @file
 * @brief sl_battery_monitor Config
 *******************************************************************************
 * # License
 * <b>Copyright 2019 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_BATTERY_MONITOR_CONFIG_H
#define SL_BATTERY_MONITOR_CONFIG_H

#include "em_iadc.h"

// <<< Use Configuration Wizard in Context Menu

// <h> Battery Monitor settings
// <o SL_BATTERY_MONITOR_TIMEOUT_MINUTES> Monitor Timeout (Minutes)
// <1..1000:1>
// <i> Default: 30
// <i> The length of time between battery reads.
#define SL_BATTERY_MONITOR_TIMEOUT_MINUTES 60

// <o SL_BATTERY_MONITOR_SAMPLE_FIFO_SIZE> Sample Collection FIFO Size
// <1..20:1>
// <i> Default: 16
// <i> The number of entries in the fifo used to collect ADC reads of the battery
#define SL_BATTERY_MONITOR_SAMPLE_FIFO_SIZE 16

// <o SL_BATTERY_MONITOR_IADC_POS> IADC Pos Select Input
// <i> Default: Avdd
// <iadcPosInputAvdd=> Avdd
// <iadcPosInputVddio=> Vddio
// <iadcPosInputDvdd=> Dvdd
// <iadcPosInputDecouple=> Decouple
// <iadcPosInputPortAPin0=> PortAPin0
// <iadcPosInputPortAPin1=> PortAPin1
// <iadcPosInputPortAPin2=> PortAPin2
// <iadcPosInputPortAPin3=> PortAPin3
// <iadcPosInputPortAPin4=> PortAPin4
// <iadcPosInputPortAPin5=> PortAPin5
// <iadcPosInputPortAPin6=> PortAPin6
// <iadcPosInputPortAPin7=> PortAPin7
// <iadcPosInputPortAPin8=> PortAPin8
// <iadcPosInputPortAPin9=> PortAPin9
// <iadcPosInputPortAPin10=> PortAPin10
// <iadcPosInputPortAPin11=> PortAPin11
// <iadcPosInputPortAPin12=> PortAPin12
// <iadcPosInputPortAPin13=> PortAPin13
// <iadcPosInputPortAPin14=> PortAPin14
// <iadcPosInputPortAPin15=> PortAPin15
// <iadcPosInputPortBPin0=> PortBPin0
// <iadcPosInputPortBPin1=> PortBPin1
// <iadcPosInputPortBPin2=> PortBPin2
// <iadcPosInputPortBPin3=> PortBPin3
// <iadcPosInputPortBPin4=> PortBPin4
// <iadcPosInputPortBPin5=> PortBPin5
// <iadcPosInputPortBPin6=> PortBPin6
// <iadcPosInputPortBPin7=> PortBPin7
// <iadcPosInputPortBPin8=> PortBPin8
// <iadcPosInputPortBPin9=> PortBPin9
// <iadcPosInputPortBPin10=> PortBPin10
// <iadcPosInputPortBPin11=> PortBPin11
// <iadcPosInputPortBPin12=> PortBPin12
// <iadcPosInputPortBPin13=> PortBPin13
// <iadcPosInputPortBPin14=> PortBPin14
// <iadcPosInputPortBPin15=> PortBPin15
// <iadcPosInputPortCPin0=> PortCPin0
// <iadcPosInputPortCPin1=> PortCPin1
// <iadcPosInputPortCPin2=> PortCPin2
// <iadcPosInputPortCPin3=> PortCPin3
// <iadcPosInputPortCPin4=> PortCPin4
// <iadcPosInputPortCPin5=> PortCPin5
// <iadcPosInputPortCPin6=> PortCPin6
// <iadcPosInputPortCPin7=> PortCPin7
// <iadcPosInputPortCPin8=> PortCPin8
// <iadcPosInputPortCPin9=> PortCPin9
// <iadcPosInputPortCPin10=> PortCPin10
// <iadcPosInputPortCPin11=> PortCPin11
// <iadcPosInputPortCPin12=> PortCPin12
// <iadcPosInputPortCPin13=> PortCPin13
// <iadcPosInputPortCPin14=> PortCPin14
// <iadcPosInputPortCPin15=> PortCPin15
// <iadcPosInputPortDPin0=> PortDPin0
// <iadcPosInputPortDPin1=> PortDPin1
// <iadcPosInputPortDPin2=> PortDPin2
// <iadcPosInputPortDPin3=> PortDPin3
// <iadcPosInputPortDPin4=> PortDPin4
// <iadcPosInputPortDPin5=> PortDPin5
// <iadcPosInputPortDPin6=> PortDPin6
// <iadcPosInputPortDPin7=> PortDPin7
// <iadcPosInputPortDPin8=> PortDPin8
// <iadcPosInputPortDPin9=> PortDPin9
// <iadcPosInputPortDPin10=> PortDPin10
// <iadcPosInputPortDPin11=> PortDPin11
// <iadcPosInputPortDPin12=> PortDPin12
// <iadcPosInputPortDPin13=> PortDPin13
// <iadcPosInputPortDPin14=> PortDPin14
// <iadcPosInputPortDPin15=> PortDPin15
#define SL_BATTERY_MONITOR_IADC_POS             iadcPosInputPortDPin0

// <o SL_BATTERY_MONITOR_IADC_ANALOG_GAIN> IADC Analog Gain
// <i> Default: x0.5
// <_IADC_CFG_ANALOGGAIN_ANAGAIN0P5=> x0.5
// <_IADC_CFG_ANALOGGAIN_ANAGAIN1=> x1
// <_IADC_CFG_ANALOGGAIN_ANAGAIN2=> x2
// <_IADC_CFG_ANALOGGAIN_ANAGAIN4=> x4
#define SL_BATTERY_MONITOR_IADC_ANALOG_GAIN     _IADC_CFG_ANALOGGAIN_ANAGAIN0P5

// <e SL_BATTERY_MONITOR_R_DIVIDER_ENABLED> Battery Voltage is using R1/R2 resistive divider
#define SL_BATTERY_MONITOR_R_DIVIDER_ENABLED    1

// <o SL_BATTERY_MONITOR_R_DIVIDER_R1> R1 Resistive divider Input Voltage leg
#define SL_BATTERY_MONITOR_R_DIVIDER_R1         2700

// <o SL_BATTERY_MONITOR_R_DIVIDER_R2> R2 Resistive divider Ground leg
#define SL_BATTERY_MONITOR_R_DIVIDER_R2         2700
// </e>

// </h> end Battery Monitor config
// <<< end of configuration section >>>

#define SL_BATTERY_MONITOR_R_DIVIDER_R1_R2 (SL_BATTERY_MONITOR_R_DIVIDER_R1 \
                                            + SL_BATTERY_MONITOR_R_DIVIDER_R2)
#define SL_BATTERY_MONITOR_R_DIVIDER_COEF  SL_BATTERY_MONITOR_R_DIVIDER_R1_R2 \
                                           / SL_BATTERY_MONITOR_R_DIVIDER_R2

// <<< sl:start pin_tool >>>
// <prs gpio=true > SL_BATTERY_MONITOR_TX_ACTIVE
// $[PRS_SL_BATTERY_MONITOR_TX_ACTIVE]
#ifndef SL_BATTERY_MONITOR_TX_ACTIVE_CHANNEL    
#define SL_BATTERY_MONITOR_TX_ACTIVE_CHANNEL     6
#endif

// PRS ASYNCH6 on PC01
#ifndef SL_BATTERY_MONITOR_TX_ACTIVE_PORT       
#define SL_BATTERY_MONITOR_TX_ACTIVE_PORT        gpioPortC
#endif
#ifndef SL_BATTERY_MONITOR_TX_ACTIVE_PIN        
#define SL_BATTERY_MONITOR_TX_ACTIVE_PIN         1
#endif
// [PRS_SL_BATTERY_MONITOR_TX_ACTIVE]$
// <<< sl:end pin_tool >>>

#endif // SL_BATTERY_MONITOR_CONFIG_H
