#pragma once
#include <stdint.h>
#include <stdbool.h>

float ema_filter_apply(int32_t sample, float* filter_state, float alpha);
void calculate_lags(int16_t* lag31, int16_t* lag32, float* buf_1, float* buf_2, float* buf_3, uint16_t size, uint8_t max_lag);
float calculate_angle_from_lags(int16_t lag31, int16_t lag32, float mic_distance, float frequency);
float calculate_energy(float* data, uint16_t head, uint16_t data_size, uint8_t window_size);
float get_max_amplitude(float a, float b, float c);
bool is_valid_spectrum_bandwith(float* data, float* fft_input, float* hann_window, uint16_t data_size, float frequency, float min_threshold, float max_threshold, float* peak_freq);