#ifndef _MAIN_APP_H
#define _MAIN_APP_H

#ifdef SL_COMPONENT_CATALOG_PRESENT
#include "sl_component_catalog.h"
#endif

#include "sl_sleeptimer.h"
#define TIMESTAMP_MS (sl_sleeptimer_tick_to_ms(sl_sleeptimer_get_tick_count()))

#define BUTTON0         0
#define BUTTON1         1

#define CLUSTERS_TO_INIT_ON_OFF 4
#define CLUSTERS_TO_INIT_LEVEL  4

#endif // _MAIN_APP_H