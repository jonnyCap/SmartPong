#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Starts the FreeRTOS task that reads the IMU and runs the classifier
void start_windowing_task(void);

// Used by the button interrupt to confirm if a throw was a hit or miss
// Only effective if the task is currently waiting for input after a throw
void confirm_throw_result(bool is_hit);

#ifdef __cplusplus
}
#endif
