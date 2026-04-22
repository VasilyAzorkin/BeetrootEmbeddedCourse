#include <stdio.h>
#include <SampleDataProcessor.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


void app_main() {
    ring_buffer_t rb = init_ring_buffer();
    ESP_LOGI("Main", "Ring buffer initialized with head at: %d", rb.head);
}