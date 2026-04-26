#pragma once

#include <stdint.h>
#define BUFFER_SIZE 1024

typedef struct{
    int32_t buffer[BUFFER_SIZE];
    uint16_t head;
} ring_buffer_t;

ring_buffer_t init_ring_buffer();
void add_sample(ring_buffer_t *rb, int32_t sample);