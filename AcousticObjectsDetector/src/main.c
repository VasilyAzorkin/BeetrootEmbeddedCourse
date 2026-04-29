#include <stdio.h>
#include <SampleDataProcessor.h>
#include <MathOperations.h>
#include "help_stuff.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "esp_wifi.h"
#include "dsps_fft2r.h"
#include "dsps_wind.h"

#define     DEBUG
#define     INIT_RETRY_COUNT        3
#define     ALPHA                   0.98f

#define     I2S_SCK_GPIO            6
#define     I2S_WS_GPIO             5
#define     I2S_MIC1_MIC2_SD_GPIO   4
#define     I2S_MIC3_SD_GPIO        18
#define     MIC_FREQUENCY           44100
#define     DMA_DESC_NUM            16
#define     DMA_FRAME_NUM           256

#define     DELAY_AFTER_INIT_MS     20000
#define     MIC_DISTANCE            0.15f
#define     MIN_TRIGGER_INTERVAL_MS 500
#define     SOUND_EVENT_QUEUE_SIZE  5
#define     FFT_SIZE                SAMPLE_WINDOW_SIZE

static i2s_chan_handle_t rx_handle_0;
static i2s_chan_handle_t rx_handle_1;

static float fft_input[FFT_SIZE * 2] __attribute__((aligned(16)));
static float hann_window[FFT_SIZE] __attribute__((aligned(16)));
bool dsp_initialized = false;

static float background_energy = 0;

QueueHandle_t sound_event_queue;
QueueHandle_t log_queue;

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

esp_err_t init_dsp_hardware() {
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret != ESP_OK) return ret;

    dsps_wind_hann_f32(hann_window, FFT_SIZE);
    
    dsp_initialized = true;
    return ESP_OK;
}

static bool triggerd_by_noise(){
    int idx[3];
    for(int i = 0; i < 3; i++) {
        idx[i] = (mic_data[i].head - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    }
    
    float current_energy = calculate_energy(mic_data[2].buffer, idx[2], BUFFER_SIZE, 128);

    float max_amp = get_max_amplitude(mic_data[0].buffer[idx[0]], mic_data[1].buffer[idx[1]], mic_data[2].buffer[idx[2]]);

    bool triggered = (current_energy > background_energy * 5.0f + 50000) && (max_amp > 100000);
    float alpha = triggered ? 0.001f : 0.05f;
    background_energy = background_energy * (1.0f - alpha) + current_energy * alpha;
    
    return triggered;
}

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
    static uint32_t last_trigger_time = 0;

    int k = 0;
    while (1) {
        esp_err_t err_0 = i2s_channel_read(rx_handle_0, sample_buffer_0, sizeof(sample_buffer_0), &bytes_read_0, 1000);
        esp_err_t err_1 = i2s_channel_read(rx_handle_1, sample_buffer_1, sizeof(sample_buffer_1), &bytes_read_1, 1000);

        if (err_0 == ESP_OK && err_1 == ESP_OK) {
            uint16_t sample_count = bytes_read_0 / sizeof(int32_t);

            for(int i = 0; i < sample_count; i += 2) {
                
                int32_t raw_left = sample_buffer_0[i];
                int32_t raw_right = sample_buffer_0[i + 1];
                int32_t raw_rear = sample_buffer_1[i];

                int32_t aligned_left = raw_left >> 8;
                int32_t aligned_right = raw_right >> 8;
                int32_t aligned_rear = raw_rear >> 8;
                

                add_sample(&mic_data[0], ema_filter_apply(aligned_left, &(mic_data[0].filter_state), ALPHA));
                add_sample(&mic_data[1], ema_filter_apply(aligned_right, &(mic_data[1].filter_state), ALPHA));
                add_sample(&mic_data[2], ema_filter_apply(aligned_rear, &(mic_data[2].filter_state), ALPHA));
            }
        } else if (err_0 != ESP_OK) {
            ESP_LOGE("I2S", "Failed to read from I2S channel 0: %s", esp_err_to_name(err_0));
        } else {
            ESP_LOGE("I2S", "Failed to read from I2S channel 1: %s", esp_err_to_name(err_1));
        }
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if(ready_to_process && current_time - last_trigger_time > MIN_TRIGGER_INTERVAL_MS && triggerd_by_noise()){ // check if rear mic has data, adjust index if needed
            last_trigger_time = current_time;
            sound_event_t event;
            event.timestamp = current_time;
            for (int i = 0; i < 3; i++) {
                get_latest_samples(event.buff_to_process[i], &mic_data[i], SAMPLE_WINDOW_SIZE);
            }
            if (xQueueSend(sound_event_queue, &event, 0) != pdPASS) {
                // ESP_LOGW("I2S", "Sound event queue is full, dropping event");
            }
        }
        else{
            ready_to_process = true;
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // to avoid task watchdog timeout, adjust delay as needed based on processing time
    }
}

void calculation_task(void *arg) {
    sound_event_t event;
    while (1) {
        if (xQueueReceive(sound_event_queue, &event, portMAX_DELAY) == pdPASS) {

            if(!dsp_initialized) {
                continue;
            }

            int i = 0;
            float peak_freq;
            bool all_mics_valid = false;


            for (; i < 3; i++) {
                if (is_valid_spectrum_bandwith(event.buff_to_process[i], fft_input, hann_window, FFT_SIZE, MIC_FREQUENCY, 2000.0f, 10000.0f, &peak_freq)) {
                    // ESP_LOGW("Processing", "Invalid spectrum bandwidth for mic %d, skipping event", i);
                    all_mics_valid = true;
                    break;
                }
            }
            
            if (!all_mics_valid) continue; // if all mics have valid spectrum, proceed with processing
            
            printf("Peak frequency: %.2f Hz on mic %d\n", peak_freq, i);


            int16_t lag31, lag32;
            calculate_lags(&lag31, &lag32, event.buff_to_process[0], event.buff_to_process[1], event.buff_to_process[2], SAMPLE_WINDOW_SIZE, 32);
            if (abs(lag31) >= 31 || abs(lag32) >= 31) {
                //ESP_LOGW("Processing", "Calculated lags are out of expected range: lag31=%d, lag32=%d", lag31, lag32);
            }
            else {
                float angle = calculate_angle_from_lags(lag31, lag32, MIC_DISTANCE, MIC_FREQUENCY);
                if (lag31 != 0 && lag32 != 0) {
                    int real_idx = (mic_data[2].head - 1 + BUFFER_SIZE) % BUFFER_SIZE;
                    char *log_message = create_log_message_for_angle(event.timestamp, lag31, lag32, angle, mic_data[2].buffer[real_idx]);
                    if (log_message) {
                        if (xQueueSend(log_queue, &log_message, 0) != pdPASS) {
                            free(log_message); // Free the log message if it cannot be queued
                        }
                    }

                    printf("[%lu ms] Lags: %d, %d | Angle: %.2f | Amp: %.0f\n", 
                        event.timestamp, 
                        lag31, lag32, 
                        angle, 
                        mic_data[2].buffer[real_idx]);
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void logger_task(void *arg) {
    char *log_message;
    while (1) {
        // Implement logging mechanism here, e.g., read from a log queue and write to storage or send over network
        if (xQueueReceive(log_queue, &log_message, 0) == pdPASS) {
            printf("LOG: %s\n", log_message);
        }
        if (log_message) free(log_message); // Free the allocated log message after processing
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust delay as needed
    }
}

void app_main() {
    esp_wifi_stop();
    init_mic_devices();
    init_dsp_hardware();

    vTaskDelay(pdMS_TO_TICKS(DELAY_AFTER_INIT_MS));
    sound_event_queue = xQueueCreate(SOUND_EVENT_QUEUE_SIZE, sizeof(sound_event_t));
    log_queue = xQueueCreate(10, sizeof(char[256])); 

    xTaskCreatePinnedToCore(i2s_read_task, "i2s_read_task", 8192, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(calculation_task, "calculation_task", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(logger_task, "logger_task", 8192, NULL, 1, NULL, 0);
}