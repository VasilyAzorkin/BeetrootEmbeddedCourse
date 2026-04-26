#include <SampleDataProcessor.h>

ring_buffer_t init_ring_buffer() {
    ring_buffer_t rb;
    rb.head = 0;
    return rb;
}

void add_sample(ring_buffer_t *rb, int32_t sample) {
    rb->buffer[rb->head] = sample;
    rb->head = (rb->head + 1) % BUFFER_SIZE;
}