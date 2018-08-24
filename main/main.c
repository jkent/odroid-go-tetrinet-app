#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "backlight.h"
#include "display.h"
#include "keypad.h"

#include "graphics.h"
#include "tf.h"
#include "OpenSans_Regular_11X12.h"


static void helloworld_task(void *arg);

void app_main(void)
{
    display_init();
    backlight_init();
    keypad_init();

    xTaskCreate(helloworld_task, "helloworld", 8192, NULL, 5, NULL);
}

static void helloworld_task(void *arg)
{
    tf_t *tf = tf_new(&font_OpenSans_Regular_11X12, 0xFFFF, 0, TF_ALIGN_CENTER);

    QueueHandle_t keypad = keypad_get_queue();

    const esp_partition_t *partition = esp_ota_get_running_partition();

    char s[32];
    snprintf(s, sizeof(s), "Hello %s world!", partition->label);
    tf_metrics_t m = tf_get_str_metrics(tf, s);
    point_t p = {
        .x = fb->width/2 - m.width/2,
        .y = fb->height/2 - m.height/2,
    };
    tf_draw_str(fb, tf, s, p);
    display_update();

    keypad_info_t keys;
    while (true) {
        if (keypad_queue_receive(keypad, &keys, 50/portTICK_RATE_MS)) {
            if (keys.pressed & KEYPAD_MENU) {
                break;
            }
        }
    }

    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    esp_ota_set_boot_partition(part);
    esp_restart();
}
