#pragma once
#include <stdint.h>

float ema_filter_apply(int32_t sample, float* filter_state, float alpha);