#include <MathOperations.h>

float ema_filter_apply(int32_t sample, float* filter_state, float alpha) {
    *filter_state = alpha * *filter_state + (1 - alpha) * (float)sample;
    return *filter_state;
}