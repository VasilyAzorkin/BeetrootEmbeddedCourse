#pragma once

#include <stdint.h>
#define BUFFER_SIZE 2048

typedef struct{
    const char *name;
    float buffer[BUFFER_SIZE];
    uint16_t head;
    float filter_state;
} mic_data_t;

void add_sample(mic_data_t *mic, float sample);
void get_latest_samples(float *dest, mic_data_t *mic, uint16_t window_size);