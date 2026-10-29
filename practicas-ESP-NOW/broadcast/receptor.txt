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

#define ESP_CHANNEL 1
#define LED_STRIP 48
#define LED_STRIP_MAX_LEDS 1

static led_strip_handle_t led_strip;
static const char *TAG = "broadcast_receiver";

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

    ESP_LOGI(TAG, "WiFi iniciado");
    return ESP_OK;
}

static void recv_cb(const esp_now_recv_info_t *info,
                    const uint8_t *data, int data_len)
{
    char message[32];

    if (data_len <= 0 || data_len >= sizeof(message)) {
        ESP_LOGW(TAG, "Longitud de mensaje invalida");
        return;
    }

    memcpy(message, data, data_len);
    message[data_len] = '\0';

    char *red_text = strtok(message, "|");
    char *green_text = strtok(NULL, "|");
    char *blue_text = strtok(NULL, "|");

    if (red_text == NULL ||
        green_text == NULL ||
        blue_text == NULL) {
        ESP_LOGW(TAG, "Formato de color incorrecto");
        return;
    }

    int red = atoi(red_text);
    int green = atoi(green_text);
    int blue = atoi(blue_text);

    if (red < 0 || red > 255 ||
        green < 0 || green > 255 ||
        blue < 0 || blue > 255) {
        ESP_LOGW(TAG, "Valores RGB fuera de rango");
        return;
    }

    ESP_LOGI(TAG, "Color recibido: %d, %d, %d",
             red, green, blue);

    led_strip_set_pixel(
        led_strip, 0, red, green, blue
    );

    led_strip_refresh(led_strip);
}

static void send_cb(const esp_now_send_info_t *info,
                    esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "Envio completado");
    } else {
        ESP_LOGW(TAG, "Error de envio");
    }
}

static esp_err_t init_esp_now(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(recv_cb));
    ESP_ERROR_CHECK(esp_now_register_send_cb(send_cb));

    ESP_LOGI(TAG, "Esperando broadcast");
    return ESP_OK;
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
        .resolution_hz = 10 * 1000 * 1000,
        .flags.with_dma = false
    };

    ESP_ERROR_CHECK(
        led_strip_new_rmt_device(
            &strip_config, &rmt_config, &led_strip
        )
    );

    return ESP_OK;
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_wifi());
    ESP_ERROR_CHECK(init_esp_now());
    ESP_ERROR_CHECK(init_led_strip());

    ESP_LOGI(TAG, "Receptor listo");
}
