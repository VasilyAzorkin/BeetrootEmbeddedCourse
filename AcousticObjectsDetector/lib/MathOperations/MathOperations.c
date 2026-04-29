#include <MathOperations.h>
#include <math.h>
#include "dsps_fft2r.h"
#include "dsps_wind.h"
#include "dsps_view.h"

float ema_filter_apply(int32_t sample, float* filter_state, float alpha) {
    *filter_state = alpha * (*filter_state) + (1 - alpha) * (float)sample;
    return sample - *filter_state;   
}

void calculate_lags(int16_t* lag31, int16_t* lag32, float* buf_1, float* buf_2, float* buf_3, uint16_t size, uint8_t max_lag) {
    float max_corr_31 = -1e20;
    float max_corr_32 = -1e20;
    *lag31 = 0;
    *lag32 = 0;


    for (int16_t lag = -max_lag; lag <= max_lag; lag++) {
        float corr_31 = 0.0f;
        float corr_32 = 0.0f;
        for (uint16_t i = 0; i < size; i++) {
            int16_t j = i + lag;
            if (j >= 0 && j < size) {
                corr_31 += buf_3[i] * buf_1[j];
                corr_32 += buf_3[i] * buf_2[j];
            }
        }
        if (corr_31 > max_corr_31) {
            max_corr_31 = corr_31;
            *lag31 = lag;
        }
        if (corr_32 > max_corr_32) {
            max_corr_32 = corr_32;
            *lag32 = lag;
        }
    }
}

float calculate_angle_from_lags(int16_t lag31, int16_t lag32, float mic_distance, float frequency) {
    float speed_of_sound = 343.0f;
    float meters_per_sample = speed_of_sound / frequency;

    float distance_31 = (float)lag31 * meters_per_sample;
    float distance_32 = (float)lag32 * meters_per_sample;

    float vx = (distance_32 - distance_31) / mic_distance;

    float vy = (distance_31 + distance_32) / (1.732f * mic_distance);

    float angle_rad = atan2f(vx, vy);
    float angle_deg = angle_rad * (180.0f / M_PI);

    return angle_deg;
}

float calculate_energy(float* data, uint16_t head, uint16_t data_size, uint8_t window_size) {
    float current_energy = 0.0f;
    for(int i = 0; i < window_size; i++) {
        int idx = (head - i + data_size) % data_size;
        current_energy += fabsf(data[idx]);
    }
    current_energy /= window_size;
    return current_energy;
};

float get_max_amplitude(float a, float b, float c) {
    float amp0 = fabsf(a);
    float amp1 = fabsf(b);
    float amp2 = fabsf(c);

    float max_amp = amp0;
    if (amp1 > max_amp) max_amp = amp1;
    if (amp2 > max_amp) max_amp = amp2;

    return max_amp;
}

bool is_valid_spectrum_bandwith(float* data, float* fft_input, float* hann_window, uint16_t data_size,
     float frequency, float min_threshold, float max_threshold, float* peak_freq) {

    for (uint16_t i = 0; i < data_size; i++) {
        fft_input[i * 2] = data[i] * hann_window[i];
        fft_input[i * 2 + 1] = 0.0f;
    }

    dsps_fft2r_fc32(fft_input, data_size);
    dsps_bit_rev_fc32(fft_input, data_size);

    float max_power = 0.0f;
    int peak_bin = 0;
    
    for (uint16_t i = 0; i < data_size / 2; i++) {
        float real = fft_input[i * 2];
        float imag = fft_input[i * 2 + 1];
        float power = real * real + imag * imag;
        if (power > max_power) {
            max_power = power;
            peak_bin = i;
        }
    }

    *peak_freq = (float)peak_bin * frequency / (float)data_size;
    
    if (*peak_freq < min_threshold || *peak_freq > max_threshold) {
        return false;
    }

    return true;
}