#pragma once
#include <stdbool.h>

#include "m5stickc_plus.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_windowing_task(m5stickc_plus_t *device);

void confirm_throw_result(bool is_hit);

#ifdef __cplusplus
}
#endif
