#include <SampleDataProcessor.h>

void add_sample(mic_data_t *mic, int32_t sample) {
    mic->buffer[mic->head] = sample;
    mic->head = (mic->head + 1) % BUFFER_SIZE;
}