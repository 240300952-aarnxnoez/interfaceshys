#include <stdio.h>
#include <string.h>
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

static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static const char *TAG = "COLOR_TX";
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

static void tx_callback(const esp_now_send_info_t *info,
                        esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Mensaje enviado");
    } else {
        ESP_LOGW(TAG, "No se confirmo el envio");
    }
}

static esp_err_t espnow_setup(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(tx_callback));

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    peer.channel = ESPNOW_CHANNEL;
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

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

static void publish_color(const char *color)
{
    esp_err_t result = esp_now_send(
        broadcast_mac,
        (const uint8_t *)color,
        strlen(color) + 1
    );

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Error al enviar: %s", esp_err_to_name(result));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(wifi_setup());
    ESP_ERROR_CHECK(espnow_setup());
    ESP_ERROR_CHECK(led_setup());

    const char *colors[] = {
        "255|0|0",
        "0|255|0",
        "0|0|255"
    };

    int index = 0;

    while (1) {
        publish_color(colors[index]);

        if (index == 0) {
            led_strip_set_pixel(led_strip, 0, 255, 0, 0);
        } else if (index == 1) {
            led_strip_set_pixel(led_strip, 0, 0, 255, 0);
        } else {
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);
        }

        ESP_ERROR_CHECK(led_strip_refresh(led_strip));

        index = (index + 1) % 3;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
