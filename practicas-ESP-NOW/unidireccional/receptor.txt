#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "led_strip.h"

#define ESPNOW_CHANNEL 1
#define LED_GPIO 48
#define NUM_LEDS 1

static const char *TAG = "COLOR_RX";
static led_strip_handle_t led_strip;

static esp_err_t wifi_setup(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

    return ESP_OK;
}

static void rx_callback(const esp_now_recv_info_t *info,
                        const uint8_t *data, int len)
{
    if (data == NULL || len <= 0 || len >= 32) {
        ESP_LOGW(TAG, "Mensaje invalido");
        return;
    }

    char message[32];
    memcpy(message, data, len);
    message[len] = '\0';

    int red, green, blue;
    char extra;

    if (sscanf(message, "%d|%d|%d%c",
               &red, &green, &blue, &extra) != 3) {
        ESP_LOGW(TAG, "Formato incorrecto: %s", message);
        return;
    }

    if (red < 0 || red > 255 ||
        green < 0 || green > 255 ||
        blue < 0 || blue > 255) {
        ESP_LOGW(TAG, "Valores fuera de rango");
        return;
    }

    ESP_LOGI(TAG, "Recibido de " MACSTR ": %s",
             MAC2STR(info->src_addr), message);

    led_strip_set_pixel(led_strip, 0, red, green, blue);
    led_strip_refresh(led_strip);
}

static esp_err_t espnow_setup(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(rx_callback));

    return ESP_OK;
}

static esp_err_t led_setup(void)
{
    led_strip_config_t config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = NUM_LEDS,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false
    };

    led_strip_rmt_config_t rmt = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false
    };

    return led_strip_new_rmt_device(&config, &rmt, &led_strip);
}

void app_main(void)
{
    ESP_ERROR_CHECK(wifi_setup());
    ESP_ERROR_CHECK(led_setup());
    ESP_ERROR_CHECK(espnow_setup());

    ESP_LOGI(TAG, "Esperando colores por ESP-NOW...");
}
