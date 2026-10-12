/*
 * Baseline (UNPROTECTED) firmware for the thesis.
 *
 * Runs the real "ethanol_detect" gas-classification model (ported from
 * the R&D project's trained ONNX model) on boot, against a fixed test
 * window, and prints the prediction + inference latency.
 *
 * This is the version that gets attacked first (Chapter 4): flash dump,
 * firmware tampering, buffer-overflow memory read. Later, Secure Boot V2
 * + Flash Encryption get enabled and the SAME model/logic is reflashed
 * as the protected build, so boot time / inference latency / flash size
 * can be compared directly against this baseline.
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

// Fixed test window standing in for a live BME688 reading until the
// real-board classifier (being built in gasnet_live) is swapped in.
// Same shape the ONNX model expects: 10 heater-cycle steps x
// [resistance, temperature, duration, heaterProfileID].
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

    /* Print chip information */
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

    /* --- Gas model inference + timing (what Chapter 5 measures) --- */
    printf("\n--- gas_model baseline inference ---\n");
    float probs[GAS_N_CLASSES];
    int64_t t0 = esp_timer_get_time();           // microseconds
    int cls = gas_model_predict(TEST_WINDOW, probs);
    int64_t t1 = esp_timer_get_time();

    printf("probs: ethanol=%.6f water=%.6f\n", probs[0], probs[1]);
    printf("predicted class: %d (%s)\n", cls, CLASS_NAMES[cls]);
    printf("inference latency: %lld us\n", (long long)(t1 - t0));

    printf("\nUNPROTECTED BUILD — no Secure Boot, no Flash Encryption.\n");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
