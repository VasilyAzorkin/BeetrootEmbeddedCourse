#pragma once
#include <stdint.h>

float ema_filter_apply(int32_t sample, float* filter_state, float alpha);
void calculate_lags(int16_t* lag31, int16_t* lag32, float* buf_1, float* buf_2, float* buf_3, uint16_t size, uint8_t max_lag);
float calculate_angle_from_lags(int16_t lag31, int16_t lag32, float mic_distance, float frequency); 