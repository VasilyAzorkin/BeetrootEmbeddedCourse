#include "help_stuff.h"
#include "esp_log.h"
// #include "driver/i2s_std.h"

esp_err_t retry_i2s_new_channel(i2s_chan_config_t *cfg, i2s_chan_handle_t *handle, const char *name, uint8_t retry_count) 
{
    uint8_t retries = 0;

    while (retries < retry_count) {
        if (i2s_new_channel(cfg, NULL, handle) == ESP_OK ) {
            return ESP_OK;
        }
        ESP_LOGE("I2S", "Failed to create I2S channel for %s, retrying... (%d/%d)",
                 name, retries + 1, retry_count);
        retries++;
    }

    ESP_LOGE("I2S", "Failed to create I2S channel for %s after %d attempts, aborting initialization",
             name, retry_count);
    return ESP_FAIL;
}

esp_err_t retry_i2s_channel_init_and_enable(i2s_chan_handle_t handle, i2s_std_config_t *cfg, const char *name, uint8_t retry_count)
{
    uint8_t retries = 0;

    while (retries < retry_count) {
        if (i2s_channel_init_std_mode(handle, cfg) == ESP_OK &&
            i2s_channel_enable(handle) == ESP_OK) {
            return ESP_OK;
        }
        ESP_LOGE("I2S", "Failed to initialize and enable I2S channel for %s, retrying... (%d/%d)",
                 name, retries + 1, retry_count);
        retries++;
    }

    ESP_LOGE("I2S", "Failed to initialize and enable I2S channel for %s after %d attempts, aborting initialization",
             name, retry_count);
    return ESP_FAIL;
}

char* create_log_message_for_angle(uint32_t timestamp, uint8_t lag31, uint8_t lag32, float angle, float amplitude){
    char *log_str = malloc(128);

    if (log_str) {
        snprintf(log_str, 128, "[%lu ms] Lags: %d, %d | Angle: %.2f | Amp: %.0f",
                 timestamp, lag31, lag32, angle, amplitude);
    }
    return log_str;
}