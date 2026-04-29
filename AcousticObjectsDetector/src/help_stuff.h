#pragma once
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"

esp_err_t retry_i2s_new_channel(i2s_chan_config_t *cfg, i2s_chan_handle_t *handle, const char *name, uint8_t retry_count);
esp_err_t retry_i2s_channel_init_and_enable(i2s_chan_handle_t handle, i2s_std_config_t *cfg, const char *name, uint8_t retry_count);
char* create_log_message_for_angle(uint32_t timestamp, uint8_t lag31, uint8_t lag32, float angle, float amplitude);