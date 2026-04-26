#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_wifi.h"

#define I2S_SCK_GPIO 6
#define I2S_WS_GPIO 5
#define I2S_SD_GPIO 4

#define MAX_DELAY_IN_SAMPLES 20
#define BUFFER_SIZE 2048
#define SAMPLES_BUFFER_SIZE (BUFFER_SIZE / 2) // Количество семплов в каждом канале

static const char *TAG = "I2S";
static const float ALPHA = 0.995f;

static float filter_state_0 = 0.0f;
static float filter_state_1 = 0.0f;

static float bufferL[SAMPLES_BUFFER_SIZE];
static float bufferR[SAMPLES_BUFFER_SIZE];

i2s_chan_handle_t rx_handle;

void init_i2s()
{

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 16;           // Увеличиваем количество DMA буферов для более стабильной работы
    chan_cfg.dma_frame_num = 256;        // Увеличиваем размер фрейма для уменьшения количества прерываний и повышения производительности
    chan_cfg.auto_clear_after_cb = true; // Включаем автоматическое очищение буфера после отправки данных, чтобы не отправлять старые данные

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));
    // ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_SCK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_SD_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            }}};

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle));
}

float filter_sample(float *filter_state, float input_sample)
{
    *filter_state = ALPHA * (*filter_state) + (1 - ALPHA) * input_sample;
    return *filter_state;
}

void log_out() {
    // printf("BufferL: ");
    for (int i = 0; i < SAMPLES_BUFFER_SIZE; i++) {
        printf("%f\t%f\n", bufferL[i], bufferR[i]);
    }
    printf("\n");

    // printf("BufferR: ");
    // for (int i = 0; i < SAMPLES_BUFFER_SIZE; i++) {
    //     printf("%f\n", bufferR[i]);
    // }
    // printf("\n");
}

// Функция будет вызываться, когда накопим BUFFER_SIZE семплов
void process_tdoa() {
    float max_corr = -1e20;
    int best_lag = 0;

    // Сдвигаем R относительно L
    for (int lag = -MAX_DELAY_IN_SAMPLES; lag <= MAX_DELAY_IN_SAMPLES; lag++) {
        float current_corr = 0;
        for (int i = 0; i < SAMPLES_BUFFER_SIZE; i++) {
            int r_idx = i + lag;
            if (r_idx >= 0 && r_idx < SAMPLES_BUFFER_SIZE) {
                current_corr += bufferL[i] * bufferR[r_idx];
            }
        }
        
        if (current_corr > max_corr) {
            max_corr = current_corr;
            best_lag = lag;
        }
    }
    
    // Выводим результат: если лаг > 0, звук справа, если < 0 - слева
    printf("Best Lag: %d\n", best_lag);
}

void app_main()
{
    esp_wifi_stop();
    init_i2s();

    size_t bytes_read;
    // int32_t i2s_data[64];
    //  Выделяем память
    
    size_t buffer_size = BUFFER_SIZE * sizeof(int32_t);
    int32_t *test_buffer = (int32_t *)malloc(buffer_size);

    if (test_buffer == NULL)
        return; // Всегда проверяй malloc

    vTaskDelay(pdMS_TO_TICKS(15000)); // Небольшая задержка, чтобы I2S успел инициализироваться
    // Читаем корректное количество байт
    printf("Reading I2S data...\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    // int k = 0;
    // while (k < 512*10){
    esp_err_t err = i2s_channel_read(rx_handle, test_buffer, buffer_size, &bytes_read, 1000);

    if (err == ESP_OK)
    {
        // Количество реально прочитанных СЭМПЛОВ
        int samples_count = bytes_read / sizeof(int32_t);

        for (int i = 0; i < samples_count; i += 2)
        {

            int32_t raw_L = test_buffer[i];
            int32_t raw_R = test_buffer[i + 1];

            // int32_t clean = (raw & 0xFFFFFF00);

            // Выравниваем обратно к 24-битному знаковому числу
            int32_t aligned_L = raw_L >> 8;
            int32_t aligned_R = raw_R >> 8;

            filter_state_0 = ALPHA * filter_state_0 + (1 - ALPHA) * (float)aligned_L;
            filter_state_1 = ALPHA * filter_state_1 + (1 - ALPHA) * (float)aligned_R;

            bufferL[i / 2] = aligned_L - filter_state_0; // Используем выровненное значение как "чистое"
            bufferR[i / 2] = aligned_R - filter_state_1;

            // float clean_L = aligned_L - filter_state_0; // Используем выровненное значение как "чистое"
            // float clean_R = aligned_R - filter_state_1;

            //printf("L: %f R: %f\n", clean_L, clean_R);

            // if (i % 2 == 0) {
            //     // Левый микрофон
            //     printf("L:%f ", filter_sample(&filter_state_0, (float)clean));
            // } else
            //     // Правый микрофон
            //     printf("R:%f\n", filter_sample(&filter_state_1, (float)clean));
            // }

            // if (i % 2 == 0) {
            //     // Левый микрофон
            //     printf("L:%ld ", clean);
            // } else
            //     // Правый микрофон
            //     printf("R:%ld\n", clean);
            // }
        }

        free(test_buffer);
        log_out();
        process_tdoa();

        // while (1)
        // {
        //     esp_err_t ret = i2s_channel_read(rx_handle, i2s_data, sizeof(i2s_data), &bytes_read, portMAX_DELAY);

        //     if (ret == ESP_OK)
        //     {
        //         // Вычисляем, сколько РЕАЛЬНО целых чисел к нам пришло
        //         int samples_count = bytes_read / sizeof(int32_t);

        //         for (int i = 0; i < samples_count; i++)
        //         {
        //             // 3. Берем целое число и сдвигаем его на 8 бит вправо.
        //             // Это убирает те самые 8 нулевых бит и возвращает 24-битное значение звука.
        //             int32_t full_sample = i2s_data[i];
        //             int32_t clean_sample = full_sample >> 8;

        //             printf("Clean sample: %ld\n", clean_sample);
        //         }
        //         vTaskDelay(pdMS_TO_TICKS(200)); // Небольшая задержка, чтобы не перегружать процессор
        //     }
        // }
    }
}