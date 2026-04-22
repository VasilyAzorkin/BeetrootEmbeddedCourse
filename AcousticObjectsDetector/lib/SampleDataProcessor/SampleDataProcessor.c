#include <SampleDataProcessor.h>

ring_buffer_t init_ring_buffer() {
    ring_buffer_t rb;
    rb.head = 0;
    return rb;
}