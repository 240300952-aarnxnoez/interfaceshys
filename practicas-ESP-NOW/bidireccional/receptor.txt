#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "led_strip.h"

#define WIFI_CHANNEL 1
#define RGB_PIN 48
#define NUM_LEDS 1

static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static esp_now_peer_info_t sender_device = {};
static led_strip_handle_t rgb_led;

static const char *TAG = "RECEIVER";

static esp_err_t wifi_setup(void)
{
    esp_err_t result = nvs_flash_init();

    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }

    ESP_ERROR_CHECK(result);

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi configurado");

    return ESP_OK;
}

static esp_err_t send_response(
    const uint8_t *destination,
    const uint8_t *message,
    size_t size)
{
    return esp_now_send(destination, message, size);
}

static void receive_callback(
    const esp_now_recv_info_t *info,
    const uint8_t *data,
    int length)
{
    ESP_LOGI(TAG, "Mensaje recibido");

    char rgb_data[32];

    if (length <= 0 || length >= sizeof(rgb_data))
    {
        return;
    }

    memcpy(rgb_data, data, length);
    rgb_data[length] = '\0';

    char *red_text = strtok(rgb_data, "|");
    char *green_text = strtok(NULL, "|");
    char *blue_text = strtok(NULL, "|");

    if (red_text == NULL ||
        green_text == NULL ||
        blue_text == NULL)
    {
        return;
    }

    uint8_t red = atoi(red_text);
    uint8_t green = atoi(green_text);
    uint8_t blue = atoi(blue_text);

    led_strip_set_pixel(
        rgb_led,
        0,
        red,
        green,
        blue
    );

    led_strip_refresh(rgb_led);

    send_response(
        broadcast_mac,
        data,
        length
    );
}

static void transmission_callback(
    const esp_now_send_info_t *info,
    esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "Respuesta enviada");
    }
    else
    {
        ESP_LOGW(TAG, "No se pudo enviar respuesta");
    }
}

static esp_err_t espnow_setup(void)
{
    ESP_ERROR_CHECK(esp_now_init());

    ESP_ERROR_CHECK(esp_now_register_recv_cb(receive_callback));
    ESP_ERROR_CHECK(esp_now_register_send_cb(transmission_callback));

    ESP_LOGI(TAG, "ESP-NOW iniciado");

    return ESP_OK;
}

static esp_err_t add_sender(void)
{
    memcpy(sender_device.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);

    sender_device.channel = 0;
    sender_device.ifidx = WIFI_IF_STA;

    ESP_ERROR_CHECK(esp_now_add_peer(&sender_device));

    return ESP_OK;
}

static esp_err_t led_setup(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_PIN,
        .max_leds = NUM_LEDS,
        .color_component_format =
            LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,
        .flags.with_dma = false
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config,
            &rmt_config,
            &rgb_led
        )
    );

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(wifi_setup());
    ESP_ERROR_CHECK(espnow_setup());
    ESP_ERROR_CHECK(add_sender());
    ESP_ERROR_CHECK(led_setup());

    ESP_LOGI(TAG, "Receptor listo");
}
