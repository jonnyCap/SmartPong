#include "mqtt_logic.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "mdns.h"
#include "cJSON.h"

static const char *TAG = "mqtt_logic";
static esp_mqtt_client_handle_t client = NULL;

#define MDNS_BROKER_HOST  "smartpong-broker"
#define MQTT_FALLBACK_URI "mqtt://192.168.1.100"
#define MDNS_RESOLVE_TIMEOUT_MS 4000

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

static void resolve_broker_uri(char *out_uri, size_t out_len)
{
    snprintf(out_uri, out_len, "%s", MQTT_FALLBACK_URI);

    if (mdns_init() != ESP_OK) {
        ESP_LOGW(TAG, "mdns_init failed; using fallback %s", MQTT_FALLBACK_URI);
        return;
    }

    esp_ip4_addr_t addr = { .addr = 0 };
    esp_err_t err = mdns_query_a(MDNS_BROKER_HOST, MDNS_RESOLVE_TIMEOUT_MS, &addr);
    if (err == ESP_OK && addr.addr != 0) {
        char ip[16];
        esp_ip4addr_ntoa(&addr, ip, sizeof(ip));
        snprintf(out_uri, out_len, "mqtt://%s", ip);
        ESP_LOGI(TAG, "mDNS resolved %s.local -> %s", MDNS_BROKER_HOST, ip);
    } else {
        ESP_LOGW(TAG, "mDNS resolve of %s.local failed (%s); using fallback %s",
                 MDNS_BROKER_HOST, esp_err_to_name(err), MQTT_FALLBACK_URI);
    }
}

void mqtt_app_start(void)
{
    static char broker_uri[48];
    resolve_broker_uri(broker_uri, sizeof(broker_uri));
    ESP_LOGI(TAG, "Connecting to MQTT broker: %s", broker_uri);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
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
