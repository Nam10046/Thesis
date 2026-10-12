/*
 * TAMPERED firmware for the thesis — Attack 2: firmware tampering.
 *
 * This is a deliberately modified copy of hello_world_main.c. It still
 * runs the real gas_model inference, but the result is overridden
 * before anything is reported: the device always claims "water" (the
 * "safe" class) regardless of what the model actually computed.
 *
 * This simulates an attacker who altered the firmware on an unprotected
 * device so a monitoring system silently stops reporting real readings.
 * It is flashed the exact same way as the baseline firmware — the point
 * being demonstrated is that, with no protection enabled, there is
 * nothing on this device that distinguishes "the real firmware" from
 * "this tampered firmware." Both flash and both boot without complaint.
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "gas_model.h"

// Same fixed test window as the baseline build, unchanged — so any
// difference in the printed result comes only from the tampering below,
// not from different input data.
static const gas_window_t TEST_WINDOW = {
    {500000.0f, 25.0f, 100.0f, 3.0f},
    {573110.3f, 25.1f, 100.0f, 3.0f},
    {588197.3f, 25.2f, 100.0f, 3.0f},
    {540584.0f, 25.3f, 100.0f, 3.0f},
    {483239.8f, 25.4f, 100.0f, 3.0f},
    {478080.7f, 25.5f, 100.0f, 3.0f},
    {539043.8f, 25.6f, 100.0f, 3.0f},
    {619274.0f, 25.7f, 100.0f, 3.0f},
    {654201.9f, 25.8f, 100.0f, 3.0f},
    {620908.9f, 25.9f, 100.0f, 3.0f},
};

static const char *CLASS_NAMES[GAS_N_CLASSES] = {"ethanol", "water"};

void app_main(void)
{
    printf("Hello world!\n");

    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }
    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    printf("\n--- gas_model inference ---\n");
    float probs[GAS_N_CLASSES];
    int64_t t0 = esp_timer_get_time();
    int real_cls = gas_model_predict(TEST_WINDOW, probs);   // real model output, computed as normal
    int64_t t1 = esp_timer_get_time();

    // --- TAMPERING: override the real result before it is ever reported ---
    // A genuine attacker wouldn't leave a comment here, obviously — this
    // line stands in for whatever modification forces the "safe" class.
    int reported_cls = 1; // 1 = "water", forced regardless of real_cls

    printf("[internal] model actually computed: %s (p_ethanol=%.6f, p_water=%.6f)\n",
           CLASS_NAMES[real_cls], probs[0], probs[1]);
    printf("reported class: %d (%s)\n", reported_cls, CLASS_NAMES[reported_cls]);
    printf("inference latency: %lld us\n", (long long)(t1 - t0));

    printf("\nTAMPERED BUILD — reported result does not match the model's real output.\n");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
