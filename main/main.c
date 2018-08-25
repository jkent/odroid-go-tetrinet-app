#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

#include "backlight.h"
#include "display.h"
#include "keypad.h"
#include "wifi.h"

#include "client.h"
#include "event.h"
#include "graphics.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"
#include "server.h"


static void tetrinet_task(void *arg);

void app_main(void)
{
    display_init();
    backlight_init();
    keypad_init();
    event_init();

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
   
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    wifi_enable();

    xTaskCreate(tetrinet_task, "tetrinet", 8192, NULL, 5, NULL);
    xTaskCreate(server_main, "server", 8192, NULL, 5, NULL);
}

static void tetrinet_task(void *arg)
{
    event_t event;
    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 0xFFFF, 0, TF_ALIGN_CENTER);

    while (true) {
        char s[64];
        ip4_addr_t ip = wifi_get_ip();

        rect_t r = {
            .x = 0,
            .y = 0,
            .width = fb->width,
            .height = fb->height,
        };
        fill_rectangle(fb, r, 0x0000);

        snprintf(s, sizeof(s), "Server running on " IPSTR "!", IP2STR(&ip));
        tf_metrics_t m = tf_get_str_metrics(tf, s);
        point_t p = {
            .x = fb->width/2 - m.width/2,
            .y = fb->height/2 - m.height/2,
        };
        tf_draw_str(fb, tf, s, p);

        display_update();

        if (!xQueueReceive(event_queue, &event, 100/portTICK_PERIOD_MS)) {
            continue;
        }

        if (event.type != EVENT_TYPE_KEYPAD) {
            continue;
        }

        if (event.keypad.pressed & KEYPAD_START) {
            client_main(NULL);
        }

        if (event.keypad.pressed & KEYPAD_MENU) {
            break;
        }
    }

    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    esp_ota_set_boot_partition(part);
    esp_restart();
}
