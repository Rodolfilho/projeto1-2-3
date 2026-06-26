#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
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
#define BUTTON_GPIO     GPIO_NUM_4

static const char *TAG = "NODE_A_PUBLISHER_V2";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static QueueHandle_t gpio_evt_queue = NULL;
static bool led_state = false;

// ISR (Handler da Interrupção do Botão)
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task do FreeRTOS para tratar o botão e publicar no MQTT
static void button_task(void* arg) {
    uint32_t io_num;
    TickType_t last_button_press = 0;
    
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            // Debounce simples via software (200ms)
            TickType_t now = xTaskGetTickCount();
            if (now - last_button_press > pdMS_TO_TICKS(200)) {
                last_button_press = now;
                
                // Inverte o estado do comando
                led_state = !led_state;
                const char *message = led_state ? "ON" : "OFF";
                
                if (mqtt_client != NULL) {
                    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC, message, 0, 1, 0);
                    ESP_LOGI(TAG, "Botão pressionado! Publicado '%s' no Gateway BBB. ID: %d", message, msg_id);
                } else {
                    ESP_LOGW(TAG, "MQTT não conectado ainda.");
                }
            }
        }
    }
}

// Event Handler do MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED: Conectado ao Gateway BBB!");
            mqtt_client = event->client;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED: Desconectado do Gateway.");
            mqtt_client = NULL;
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
        ESP_LOGI(TAG, "Wi-Fi Conectado! IP do Node A: " IPSTR, IP2STR(&event->ip_info.ip));
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
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(button_task, "button_task", 3072, NULL, 10, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, gpio_isr_handler, (void*) BUTTON_GPIO);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}