#include <stdio.h>
#include <string.h>

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

#define ESP_CHANNEL 1
#define LED_STRIP 48
#define LED_STRIP_MAX_LEDS 1

#define ESP_NOW_PMK "pmkshjshdjsdhfcdfdsf"
#define ESP_NOW_LMK "lmkshjshdjsdhfcdfdsf"

static uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static esp_now_peer_info_t peer_config = {};
static led_strip_handle_t led_strip;

static const char *TAG = "color_sender";

static esp_err_t init_wifi(void)
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

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    return ESP_OK;
}

static void recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "Respuesta recibida");
}

static void send_cb(const esp_now_send_info_t *info,
                    esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Transmision exitosa");
    } else {
        ESP_LOGW(TAG, "Error de transmision");
    }
}

static esp_err_t init_esp_now(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));
    ESP_ERROR_CHECK(esp_now_set_pmk((const uint8_t *)ESP_NOW_PMK));

    return ESP_OK;
}

static esp_err_t register_broadcast(void)
{
    memcpy(peer_config.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
    peer_config.channel = 0;
    peer_config.ifidx = WIFI_IF_STA;
    peer_config.encrypt = false;

    ESP_ERROR_CHECK(esp_now_add_peer(&peer_config));

    return ESP_OK;
}

static esp_err_t send_color(const char *color)
{
    return esp_now_send(
        broadcast_addr,
        (const uint8_t *)color,
        strlen(color) + 1
    );
}

static esp_err_t init_led_strip(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP,
        .max_leds = LED_STRIP_MAX_LEDS,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
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
            &led_strip
        )
    );

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(register_broadcast());
    ESP_ERROR_CHECK(init_led_strip());

    const char *colors[] = {
        "255|0|0",
        "0|255|0",
        "0|0|255"
    };

    int index = 0;

    while (true) {
        ESP_ERROR_CHECK(send_color(colors[index]));

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
