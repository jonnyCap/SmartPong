#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    APP_RECORDER_STATE_BOOTING = 0,
    APP_RECORDER_STATE_BLOCKED,
    APP_RECORDER_STATE_READY,
    APP_RECORDER_STATE_RECORDING,
    APP_RECORDER_STATE_UPLOADING,
    APP_RECORDER_STATE_WAIT_SAVE,
    APP_RECORDER_STATE_ERROR,
} app_recorder_state_t;

typedef struct {
    int64_t timestamp_us;
    uint32_t elapsed_ms;
    float ax_g;
    float ay_g;
    float az_g;
} sensor_sample_t;

typedef struct {
    app_recorder_state_t recorder_state;
    bool config_valid;
    bool wifi_connected;
    bool tcp_connected;
    bool session_limit_reached;
    float ax_g;
    float ay_g;
    float az_g;
    uint32_t sample_count;
    uint32_t max_samples;
    uint32_t sample_rate_hz;
    uint32_t elapsed_ms;
    uint32_t session_id;
    char ip_address[16];
    char server_host[32];
    uint16_t server_port;
    char status_line[24];
    char detail_line[32];
    char server_message[32];
} app_display_snapshot_t;

static inline const char *app_recorder_state_to_string(app_recorder_state_t state)
{
    switch (state) {
    case APP_RECORDER_STATE_BOOTING:
        return "BOOTING";
    case APP_RECORDER_STATE_BLOCKED:
        return "BLOCKED";
    case APP_RECORDER_STATE_READY:
        return "READY";
    case APP_RECORDER_STATE_RECORDING:
        return "RECORDING";
    case APP_RECORDER_STATE_UPLOADING:
        return "UPLOADING";
    case APP_RECORDER_STATE_WAIT_SAVE:
        return "WAIT_SAVE";
    case APP_RECORDER_STATE_ERROR:
    default:
        return "ERROR";
    }
}
