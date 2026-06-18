#include "mqtt_logic.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "mqtt_logic";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        esp_mqtt_client_subscribe(client, "/smartpong/throws/results", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Received message on topic %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA: %.*s", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_publish_throw_data(const char* user_id, bool hit, 
                             const float* ax, const float* ay, const float* az, 
                             const float* gx, const float* gy, const float* gz, int length)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "user_id", user_id);
    cJSON_AddBoolToObject(root, "hit", hit);

    cJSON *ax_arr = cJSON_CreateArray();
    cJSON *ay_arr = cJSON_CreateArray();
    cJSON *az_arr = cJSON_CreateArray();
    cJSON *gx_arr = cJSON_CreateArray();
    cJSON *gy_arr = cJSON_CreateArray();
    cJSON *gz_arr = cJSON_CreateArray();

    for (int i = 0; i < length; i++) {
        cJSON_AddItemToArray(ax_arr, cJSON_CreateNumber(ax[i]));
        cJSON_AddItemToArray(ay_arr, cJSON_CreateNumber(ay[i]));
        cJSON_AddItemToArray(az_arr, cJSON_CreateNumber(az[i]));
        
        cJSON_AddItemToArray(gx_arr, cJSON_CreateNumber(gx ? gx[i] : 0.0));
        cJSON_AddItemToArray(gy_arr, cJSON_CreateNumber(gy ? gy[i] : 0.0));
        cJSON_AddItemToArray(gz_arr, cJSON_CreateNumber(gz ? gz[i] : 0.0));
    }

    cJSON_AddItemToObject(root, "ax", ax_arr);
    cJSON_AddItemToObject(root, "ay", ay_arr);
    cJSON_AddItemToObject(root, "az", az_arr);
    cJSON_AddItemToObject(root, "gx", gx_arr);
    cJSON_AddItemToObject(root, "gy", gy_arr);
    cJSON_AddItemToObject(root, "gz", gz_arr);

    char *json_str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(client, "/smartpong/throws/data", json_str, 0, 0, 0);
    ESP_LOGI(TAG, "Published throw data to /smartpong/throws/data");

    free(json_str);
    cJSON_Delete(root);
}
