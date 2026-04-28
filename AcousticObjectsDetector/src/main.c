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
#define     WINDOW_SIZE             (DMA_FRAME_NUM * 2)

#define     DELAY_AFTER_INIT_MS     20000
#define     MIC_DISTANCE            0.15f

static i2s_chan_handle_t rx_handle_0;
static i2s_chan_handle_t rx_handle_1;
static float filter_state_0 = 0.0f;
static float filter_state_1 = 0.0f;
static float filter_state_2 = 0.0f;

static float buff_to_process[3][WINDOW_SIZE] = {0};


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
    i2s_chan_config_t chan_cfg_0 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg_0.dma_desc_num = DMA_DESC_NUM;
    chan_cfg_0.dma_frame_num = DMA_FRAME_NUM;
    chan_cfg_0.auto_clear_after_cb = true;
    
    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg_0, NULL, &rx_handle_0));
    #else
    if (retry_i2s_new_channel(&chan_cfg_0, &rx_handle_0, "mic 1/2", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif

    i2s_chan_config_t chan_cfg_1 = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_SLAVE);
    chan_cfg_1.dma_desc_num = DMA_DESC_NUM;
    chan_cfg_1.dma_frame_num = DMA_FRAME_NUM;
    chan_cfg_1.auto_clear_after_cb = true;
    
    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg_1, NULL, &rx_handle_1));
    #else
    if (retry_i2s_new_channel(&chan_cfg_1, &rx_handle_1, "mic 3", INIT_RETRY_COUNT) != ESP_OK) {
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
    i2s_channel_enable(rx_handle_0);
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
            .bclk = I2S_SCK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_MIC3_SD_GPIO
        }
    };

    #ifdef DEBUG
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_1, &i2s_std_cfg_1));
    i2s_channel_enable(rx_handle_1);
    #else
    if (retry_i2s_channel_init_and_enable(rx_handle_1, &i2s_std_cfg_1, "mic 2", INIT_RETRY_COUNT) != ESP_OK) {
        //send critical error event to logmanager
        abort();
    }
    #endif    
}

void i2s_read_task(void *arg) {
    ESP_LOGI("I2S", "I2S read task started");

    size_t bytes_read_0, bytes_read_1;
    static int32_t sample_buffer_0[DMA_FRAME_NUM * 2]; // Buffer to hold raw audio samples
    static int32_t sample_buffer_1[DMA_FRAME_NUM * 2]; // Buffer to hold raw audio samples
    static bool ready_to_process = false;

    int k = 0;
    while (1) {
        //ESP_LOGI("I2S", "Reading audio data from I2S channels...");
        // Read audio data from the I2S channel
        esp_err_t err_0 = i2s_channel_read(rx_handle_0, sample_buffer_0, sizeof(sample_buffer_0), &bytes_read_0, 1000);
        esp_err_t err_1 = i2s_channel_read(rx_handle_1, sample_buffer_1, sizeof(sample_buffer_1), &bytes_read_1, 1000);

        //ESP_LOGI("I2S", "Read %d bytes from channel 0, %d bytes from channel 1", bytes_read_0, bytes_read_1);

        if (err_0 == ESP_OK && err_1 == ESP_OK) {
            // Process the audio samples in sample_buffer
            // For example, you can convert them to mic_data format and store in mic_data buffers
            uint16_t sample_count = bytes_read_0 / sizeof(int32_t);

            for(int i = 0; i < sample_count; i += 2) {
                
                int32_t raw_left = sample_buffer_0[i];
                int32_t raw_right = sample_buffer_0[i + 1];
                int32_t raw_rear = sample_buffer_1[i];

                int32_t aligned_left = raw_left >> 8;
                int32_t aligned_right = raw_right >> 8;
                int32_t aligned_rear = raw_rear >> 8;
                // samples_current[1] = (sample_buffer_0[i * 2 + 1]) >> 8; 
                //samples_current[2] = sample_buffer_1[i * 2] >> 8;

                // filter_state_0 = ALPHA * filter_state_0 + (1 - ALPHA) * (float)aligned_left;
                // filter_state_1 = ALPHA * filter_state_1 + (1 - ALPHA) * (float)aligned_right;
                // filter_state_2 = ALPHA * filter_state_2 + (1 - ALPHA) * (float)aligned_rear;

                // add_sample(&mic_data[0], aligned_left - filter_state_0); // Store the filter state instead of raw sample
                // add_sample(&mic_data[1], aligned_right - filter_state_1);
                // add_sample(&mic_data[2], aligned_rear - filter_state_2);


                add_sample(&mic_data[0], ema_filter_apply(aligned_left, &(mic_data[0].filter_state), ALPHA));
                add_sample(&mic_data[1], ema_filter_apply(aligned_right, &(mic_data[1].filter_state), ALPHA));
                add_sample(&mic_data[2], ema_filter_apply(aligned_rear, &(mic_data[2].filter_state), ALPHA));

                // for (int j = 0; j < 2; j++) {
                //     // printf("Sample %d: %ld\n", j, samples_current[j]);                    
                //     add_sample(&mic_data[j], ema_filter_apply(samples_current[j], &(mic_data[j].filter_state), ALPHA));
                //     //mic_data[j].head = (mic_data[j].head + 1) % BUFFER_SIZE;
                // }
            }
        } else if (err_0 != ESP_OK) {
            ESP_LOGE("I2S", "Failed to read from I2S channel 0: %s", esp_err_to_name(err_0));
        } else {
            ESP_LOGE("I2S", "Failed to read from I2S channel 1: %s", esp_err_to_name(err_1));
        }

        if(ready_to_process && mic_data[2].buffer[mic_data[2].head] > 10000.0f){ // check if rear mic has data, adjust index if needed
            int16_t lag31, lag32;
            get_latest_samples(buff_to_process[0], &mic_data[0], WINDOW_SIZE);
            get_latest_samples(buff_to_process[1], &mic_data[1], WINDOW_SIZE);
            get_latest_samples(buff_to_process[2], &mic_data[2], WINDOW_SIZE);
            calculate_lags(&lag31, &lag32, buff_to_process[0], buff_to_process[1], buff_to_process[2], WINDOW_SIZE, 20);
            float angle = calculate_angle_from_lags(lag31, lag32, MIC_DISTANCE, MIC_FREQUENCY);
            printf("Lag 31: %d, Lag 32: %d, Angle: %.2f, mic0: %.2f, mic1: %.2f, mic2: %.2f\n", lag31, lag32, angle, mic_data[0].buffer[mic_data[0].head], mic_data[1].buffer[mic_data[1].head], mic_data[2].buffer[mic_data[2].head]);
        }
        else{
            ready_to_process = true;
        }

        if (k == 7) log_out();
        k++;
        vTaskDelay(pdMS_TO_TICKS(1)); // to avoid task watchdog timeout, adjust delay as needed based on processing time
    }
}

void app_main() {
    esp_wifi_stop();
    init_mic_devices();

    vTaskDelay(pdMS_TO_TICKS(DELAY_AFTER_INIT_MS));

    xTaskCreatePinnedToCore(i2s_read_task, "i2s_read_task", 8192, NULL, 10, NULL, 1);

    // for (int i = 0; i < 3; i++) {
    //     mic_data[i] = init_ring_buffer();
    // }
}

void log_out() {
    printf("DATA: Latest samples:\n");
    printf("%d\t%d", mic_data[0].head, mic_data[1].head);
    int i = 0;
    // while (i < mic_data[0].head && i < mic_data[1].head && i < mic_data[2].head) {
    //     printf("%ld\t%ld\t%ld\t%f\t%f\t%f\n", mic_data[0].buffer[i], mic_data[1].buffer[i], mic_data[2].buffer[i], mic_data[0].filter_state, mic_data[1].filter_state, mic_data[2].filter_state);
    //     i++;
    // }
    while (i < BUFFER_SIZE) {
        printf("%f\t%f\t%f\t%f\n", mic_data[0].buffer[i], mic_data[1].buffer[i], mic_data[2].buffer[i], mic_data[2].filter_state);
        i++;
    }
    //mic_data[0].head = 0; mic_data[1].head = 0;
    printf("\n");
}