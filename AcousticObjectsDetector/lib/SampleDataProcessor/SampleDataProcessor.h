#pragma once

#include <stdint.h>
#define BUFFER_SIZE 2048

typedef struct{
    const char *name;
    int32_t buffer[BUFFER_SIZE];
    uint16_t head;
    float filter_state;
} mic_data_t;

void add_sample(mic_data_t *mic, int32_t sample);