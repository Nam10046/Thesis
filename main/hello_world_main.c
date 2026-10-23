/*
 * PROTECTED status build for the thesis.
 *
 * Runs on the ESP32-C6 after Secure Boot V2 + Flash Encryption were
 * burned. It reports the live security state (proof the protections are
 * active), runs the gas model once and times it (Chapter 5.3), and keeps
 * the deliberately-vulnerable command handler + weight-leak routine so
 * Attack 3 can still be re-run against the protected build (Chapter 4.6).
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_secure_boot.h"
#include "esp_flash_encrypt.h"

#include "gas_model.h"
#include "model_weights.h"

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

// Never called by normal control flow. The Attack 3 target (kept intact).
__attribute__((noinline))
void leak_model_weights_over_uart(void)
{
    printf("\n[LEAK] --- model weights dumped via hijacked control flow ---\n");
    printf("[LEAK] MEAN_28[0..3] = %.6f %.6f %.6f %.6f\n",
           MEAN_28[0], MEAN_28[1], MEAN_28[2], MEAN_28[3]);
    printf("[LEAK] STD_28[0..3]  = %.6f %.6f %.6f %.6f\n",
           STD_28[0], STD_28[1], STD_28[2], STD_28[3]);
    printf("[LEAK] W2_2x16[0][0..3] = %.6f %.6f %.6f %.6f\n",
           W2_2x16[0][0], W2_2x16[0][1], W2_2x16[0][2], W2_2x16[0][3]);
    printf("[LEAK] --- end dump ---\n\n");
}

// The planted bug: buf is 32 bytes, but fgets is told the buffer is 128.
__attribute__((noinline))
static void vulnerable_command_handler(void)
{
    char buf[32];
    printf("cmd> ");
    fflush(stdout);
    if (fgets(buf, 128, stdin) == NULL) { vTaskDelay(10 / portTICK_PERIOD_MS); return; }
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = 0;
    printf("echo: %s\n", buf);
}

void app_main(void)
{
    printf("\n======================================\n");
    printf(" ESP32-C6  PROTECTED BUILD\n");
    printf("======================================\n");

    // --- Live security state (thesis evidence, Ch 4.5 / 4.6) ---
    printf("Secure Boot      : %s\n", esp_secure_boot_enabled()      ? "ENABLED" : "disabled");
    printf("Flash Encryption : %s\n", esp_flash_encryption_enabled() ? "ENABLED" : "disabled");

    // --- Chip info ---
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("Chip             : %s, %d core(s), silicon rev v%d.%d\n",
           CONFIG_IDF_TARGET, chip_info.cores,
           chip_info.revision / 100, chip_info.revision % 100);
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        printf("Flash size       : %" PRIu32 " MB\n", flash_size / (uint32_t)(1024 * 1024));
    }
    printf("Free heap        : %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // --- Gas model inference + latency (Ch 5.3, protected) ---
    printf("\n--- gas_model inference (protected build) ---\n");
    float probs[GAS_N_CLASSES];
    int64_t t0 = esp_timer_get_time();
    int cls = gas_model_predict(TEST_WINDOW, probs);
    int64_t t1 = esp_timer_get_time();
    printf("probs: ethanol=%.6f water=%.6f\n", probs[0], probs[1]);
    printf("predicted class: %d (%s)\n", cls, CLASS_NAMES[cls]);
    printf("inference latency: %lld us\n", (long long)(t1 - t0));

    // --- Attack 3 target addresses (kept so Attack 3 stays testable) ---
    printf("\n--- exploit target addresses ---\n");
    printf("vulnerable_command_handler   = %p\n", (void *)vulnerable_command_handler);
    printf("leak_model_weights_over_uart = %p\n", (void *)leak_model_weights_over_uart);
    printf("--------------------------------\n");

    printf("\nReady. Entering command loop (send data over serial for Attack 3).\n\n");
    while (1) {
        vulnerable_command_handler();
    }
}
