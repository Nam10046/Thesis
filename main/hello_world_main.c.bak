/*
 * VULNERABLE firmware for the thesis — Attack 3: buffer-overflow exploit.
 *
 * Deliberately contains a classic stack buffer overflow: a 32-byte
 * command buffer is read into with a 128-byte size limit. This is a
 * standard class of real-world bug (the buffer's size constant wasn't
 * updated when the code was written/refactored).
 *
 * leak_model_weights_over_uart() is compiled into the firmware but is
 * never called by normal program flow. The exploit's goal is to
 * overflow the command buffer far enough to overwrite the saved return
 * address on the stack so that, when vulnerable_command_handler()
 * returns, execution jumps into leak_model_weights_over_uart() instead
 * of back to app_main() — reading the model out of live RAM rather
 * than from flash (contrast with Attack 1).
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

#include "gas_model.h"
#include "model_weights.h"

// Never called by normal control flow. The exploit's target.
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

// The planted bug: buf is 32 bytes, but fgets is told the buffer is
// 128 bytes. Sending more than ~31 bytes before a newline overflows
// the stack past buf[], into saved registers / the return address.
__attribute__((noinline))
static void vulnerable_command_handler(void)
{
    char buf[32];
    printf("cmd> ");
    fflush(stdout);
    fgets(buf, 128, stdin);   // BUG: should be sizeof(buf), i.e. 32
    // trim trailing newline for clean printing
    size_t n = strlen(buf);
    if (n && buf[n - 1] == '\n') buf[n - 1] = 0;
    printf("echo: %s\n", buf);
}

void app_main(void)
{
    printf("Hello world!\n");

    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s)\n", CONFIG_IDF_TARGET, chip_info.cores);
    if (esp_flash_get_size(NULL, &flash_size) == ESP_OK) {
        printf("%" PRIu32 "MB flash\n", flash_size / (uint32_t)(1024 * 1024));
    }
    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    // Print the two function addresses we need to build the exploit.
    // (Reading these off the live, running device, rather than only
    // from the .elf, so there's no doubt they match what's actually
    // flashed and running.)
    printf("\n--- exploit target addresses ---\n");
    printf("vulnerable_command_handler = %p\n", (void *)vulnerable_command_handler);
    printf("leak_model_weights_over_uart = %p\n", (void *)leak_model_weights_over_uart);
    printf("--------------------------------\n\n");

    printf("gas_model baseline present (unchanged): ready.\n");
    printf("Entering command loop. Send data over serial to interact.\n\n");

    while (1) {
        vulnerable_command_handler();
    }
}
