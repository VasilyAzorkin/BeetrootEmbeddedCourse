#include <stdio.h>
#include <SampleDataProcessor.h>
#include <MathOperations.h>
#include "help_stuff.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "esp_wifi.h"


#define     DEBUG
#define     INIT_RETRY_COUNT        3
#define     ALPHA                   0.995f

#define     I2S_SCK_GPIO            6
#define     I2S_WS_GPIO             5
#define     I2S_MIC1_MIC2_SD_GPIO   4
// #define     I2S_MIC2_SD_GPIO        7
#define     I2S_MIC3_SD_GPIO        7//15
#define     MIC_FREQUENCY           44100
#define     DMA_DESC_NUM            16
#define     DMA_FRAME_NUM           256

#define     DELAY_AFTER_INIT_MS     15000

static i2s_chan_handle_t rx_handle_0;
static i2s_chan_handle_t rx_handle_1;

static mic_data_t mic_data[3] = {
    {
        .name = "LEFT",
        .buffer = {0},
        .head = 0,
        .filter_state = 0.0f
    },
    {
        .name = "RIGHT",
        .buffer = {0},
        .head = 0,
        .filter_state = 0.0f
    },
    {
        .name = "REAR",
        .buffer = {0},
        .head = 0,
        .filter_state = 0.0f
    }
};

void log_out();

void init_mic_devices(){
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = DMA_DESC_NUM;
    chan_cfg.dma_frame_num = DMA_FRAME_NUM;
    chan_cfg.auto_clear_after_cb = true;
    
    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle_0));
    #else
    if (retry_i2s_new_channel(&chan_cfg, &rx_handle_0, "mic 1/2", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif

    chan_cfg.id = I2S_NUM_1;
    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle_1));
    #else
    if (retry_i2s_new_channel(&chan_cfg, &rx_handle_1, "mic 3", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif
    
    i2s_std_config_t i2s_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_FREQUENCY),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SCK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC1_MIC2_SD_GPIO
        }
    };

    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_0, &i2s_std_cfg));
    #else
    if (retry_i2s_channel_init_and_enable(rx_handle_0, &i2s_std_cfg, "mic 0/1", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif

    i2s_std_config_t i2s_std_cfg_1 = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_FREQUENCY),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_GPIO_UNUSED,
            .ws = I2S_GPIO_UNUSED,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC3_SD_GPIO
        }
    };

    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_1, &i2s_std_cfg_1));
    #else
    if (retry_i2s_channel_init_and_enable(rx_handle_1, &i2s_std_cfg_1, "mic 2", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif    
}

void i2s_read_task(void *arg) {
    size_t bytes_read_0, bytes_read_1;
    int32_t sample_buffer_0[DMA_FRAME_NUM * 2]; // Buffer to hold raw audio samples
    int32_t sample_buffer_1[DMA_FRAME_NUM * 2]; // Buffer to hold raw audio samples

    while (1) {
        // Read audio data from the I2S channel
        esp_err_t err_0 = i2s_channel_read(rx_handle_0, sample_buffer_0, sizeof(sample_buffer_0), &bytes_read_0, portMAX_DELAY);
        esp_err_t err_1 = i2s_channel_read(rx_handle_1, sample_buffer_1, sizeof(sample_buffer_1), &bytes_read_1, portMAX_DELAY);

        if (err_0 == ESP_OK && err_1 == ESP_OK) {
            // Process the audio samples in sample_buffer
            // For example, you can convert them to mic_data format and store in mic_data buffers
            uint16_t sample_count = bytes_read_0 / sizeof(int32_t);

            for(int i = 0; i < sample_count; i+=2) {
                int32_t samples_current[3] = { sample_buffer_0[i] >> 8, sample_buffer_0[i + 1] >> 8, sample_buffer_1[i] >> 8 };

                for (int j = 0; j < 3; j++) {
                    add_sample(&mic_data[j], ema_filter_apply(samples_current[j], &mic_data[j].filter_state, ALPHA));
                    mic_data[j].head = (mic_data[j].head + 1) % BUFFER_SIZE;
                }
            }
            log_out();
        } else if (err_0 != ESP_OK) {
            ESP_LOGE("I2S", "Failed to read from I2S channel 0: %s", esp_err_to_name(err_0));
        } else {
            ESP_LOGE("I2S", "Failed to read from I2S channel 1: %s", esp_err_to_name(err_1));
        }
    }
}

void app_main() {
    esp_wifi_stop();
    init_mic_devices();

    vTaskDelay(pdMS_TO_TICKS(DELAY_AFTER_INIT_MS));

    // for (int i = 0; i < 3; i++) {
    //     mic_data[i] = init_ring_buffer();
    // }
}

void log_out() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        printf("%ld\t%ld\n", mic_data[0].buffer[i], mic_data[1].buffer[i]);
    }
    printf("\n");
}