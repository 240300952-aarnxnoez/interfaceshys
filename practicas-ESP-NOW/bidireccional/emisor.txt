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

static esp_now_peer_info_t remote_device = {};
static led_strip_handle_t rgb_led;

static const char *TAG = "TRANSMITTER";

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

static void receive_callback(
    const esp_now_recv_info_t *info,
    const uint8_t *data,
    int length)
{
    ESP_LOGI(TAG, "Respuesta recibida");
}

static void transmission_callback(
    const esp_now_send_info_t *info,
    esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        ESP_LOGI(TAG, "Mensaje enviado correctamente");
    }
    else
    {
        ESP_LOGW(TAG, "Error al enviar mensaje");
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

static esp_err_t add_receiver(void)
{
    memcpy(remote_device.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);

    remote_device.channel = 0;
    remote_device.ifidx = WIFI_IF_STA;

    ESP_ERROR_CHECK(esp_now_add_peer(&remote_device));

    return ESP_OK;
}

static esp_err_t send_message(const char *message)
{
    return esp_now_send(
        broadcast_mac,
        (const uint8_t *)message,
        strlen(message) + 1
    );
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
    ESP_ERROR_CHECK(add_receiver());
    ESP_ERROR_CHECK(led_setup());

    const char *colors[] = {
        "255|0|0",
        "0|255|0",
        "0|0|255"
    };

    uint8_t position = 0;

    while (1)
    {
        send_message(colors[position]);

        switch (position)
        {
            case 0:
                led_strip_set_pixel(rgb_led, 0, 255, 0, 0);
                break;

            case 1:
                led_strip_set_pixel(rgb_led, 0, 0, 255, 0);
                break;

            case 2:
                led_strip_set_pixel(rgb_led, 0, 0, 0, 255);
                break;
        }

        led_strip_refresh(rgb_led);

        position++;

        if (position >= 3)
        {
            position = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
