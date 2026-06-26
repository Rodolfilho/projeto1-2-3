#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_netif.h"

// =================================================================
// ATENÇÃO: Atualize com os dados do Roteador Físico e IP da BBB
// =================================================================
#define WIFI_SSID      "MERCUSYS_7E02"
#define WIFI_PASS      "70960594"
#define MQTT_BROKER_URI "mqtt://192.168.1.XXX" 
#define MQTT_TOPIC      "ifpb/projeto/led"
#define LED_GPIO        GPIO_NUM_5

static const char *TAG = "NODE_B_SUBSCRIBER_V2";

// Event Handler do MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED: Conectado ao Gateway BBB!");
            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC, 1);
            ESP_LOGI(TAG, "Inscrito com sucesso no tópico %s, ID: %d", MQTT_TOPIC, msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA: Dados recebidos do Gateway!");
            
            char data_buff[16] = {0};
            int data_len = event->data_len < sizeof(data_buff) ? event->data_len : sizeof(data_buff) - 1;
            memcpy(data_buff, event->data, data_len);

            ESP_LOGI(TAG, "Mensagem: %s", data_buff);

            // Validação usando strcmp para acionamento do LED
            if (strcmp(data_buff, "ON") == 0) {
                gpio_set_level(LED_GPIO, 1);
                ESP_LOGI(TAG, "LED ligado (HIGH)");
            } else if (strcmp(data_buff, "OFF") == 0) {
                gpio_set_level(LED_GPIO, 0);
                ESP_LOGI(TAG, "LED desligado (LOW)");
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED: Conexão perdida com o Gateway.");
            break;
        default:
            break;
    }
}

// Inicialização do Wi-Fi (Modo STA)
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Tentando reconectar ao Wi-Fi...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Wi-Fi Conectado! IP do Node B: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << LED_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_GPIO, 0);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}