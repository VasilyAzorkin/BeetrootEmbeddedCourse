#include <SampleDataProcessor.h>

void add_sample(mic_data_t *mic, float sample) {
    mic->buffer[mic->head] = sample;
    mic->head = (mic->head + 1) % BUFFER_SIZE;
}

void get_latest_samples(float *dest, mic_data_t *mic, uint16_t window_size) {
    for (uint16_t i = 0; i < window_size; i++) {
        int16_t index = (mic->head - window_size + i);
        if (index < 0) {
            index += BUFFER_SIZE;
        }
        dest[i] = mic->buffer[index];
    }
}