#pragma once

#include "mqtt_client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_app_start(void);
void mqtt_publish_throw_data(const char* user_id, bool hit, 
                             const float* ax, const float* ay, const float* az, 
                             const float* gx, const float* gy, const float* gz, int length);

#ifdef __cplusplus
}
#endif
