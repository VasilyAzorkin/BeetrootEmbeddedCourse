#include <MathOperations.h>
#include <math.h>

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