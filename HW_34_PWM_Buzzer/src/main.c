#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

#define BUZZER_GPIO 7
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_DUTY (512) // 50% of 10-bit (1024)

// Note frequencies (Hz)
#define NOTE_E7  2637
#define NOTE_C7  2093
#define NOTE_G7  3136
#define NOTE_G6  1568
#define NOTE_E6  1319
#define NOTE_A6  1760
#define NOTE_B6  1976
#define NOTE_AS6 1865
#define NOTE_F7  2794
#define NOTE_D7  2349
#define NOTE_A7  3520

#define NOTE_G3   196
#define NOTE_A3   220
#define NOTE_AS3  233
#define NOTE_B3   247
#define NOTE_C4   262
#define NOTE_D4   294
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_G4   392
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523

typedef struct {
    int frequency;
    int duration_ms;
} note_t;

// Super Mario Bros main theme (intro)
static const note_t melody[] = {
    {NOTE_E7, 150}, {NOTE_E7, 150}, {0, 150},
    {NOTE_E7, 150}, {0, 100},
    {NOTE_C7, 150}, {NOTE_E7, 150}, {0, 150},
    {NOTE_G7, 300}, {0, 300},
    {NOTE_G6, 300}, {0, 300},

    {NOTE_C7, 150}, {0, 150},
    {NOTE_G6, 150}, {0, 150},
    {NOTE_E6, 150}, {0, 150},
    {NOTE_A6, 150}, {NOTE_B6, 150},
    {NOTE_AS6, 150}, {NOTE_A6, 150},
    {NOTE_G6, 200}, {NOTE_E7, 200},
    {NOTE_G7, 200}, {NOTE_A7, 150},
    {NOTE_F7, 150}, {NOTE_G7, 150},
    {0, 150},
    {NOTE_E7, 150}, {NOTE_C7, 150},
    {NOTE_D7, 150}, {NOTE_B6, 300}
};

// static const note_t melody[] = {
//     // --- Intro ---
//     {NOTE_E4,150}, {NOTE_E4,150}, {NOTE_E4,150},
//     {NOTE_C4,150}, {NOTE_E4,150}, {NOTE_G4,300},
//     {NOTE_G3,300},

//     // --- Phrase 1 ---
//     {NOTE_C4,300}, {NOTE_G3,300}, {NOTE_E4,300},
//     {NOTE_A3,150}, {NOTE_B3,150}, {NOTE_AS3,150}, {NOTE_A3,300},

//     // --- Phrase 2 ---
//     {NOTE_G3,200}, {NOTE_E4,200}, {NOTE_G4,200},
//     {NOTE_A4,300}, {NOTE_F4,150}, {NOTE_G4,150},
//     {NOTE_E4,300}, {NOTE_C4,150}, {NOTE_D4,150}, {NOTE_B3,300},

//     // --- Phrase 3 ---
//     {NOTE_C4,300}, {NOTE_G3,300}, {NOTE_E4,300},
//     {NOTE_A3,150}, {NOTE_B3,150}, {NOTE_AS3,150}, {NOTE_A3,300},

//     // --- Ending ---
//     {NOTE_G3,200}, {NOTE_E4,200}, {NOTE_G4,200},
//     {NOTE_A4,300}, {NOTE_F4,150}, {NOTE_G4,150},
//     {NOTE_E4,300}, {NOTE_C4,150}, {NOTE_D4,150}, {NOTE_B3,450},
// };

// static const note_t melody[] = {
//     // Baby shark, doo doo doo doo doo doo
//     {NOTE_D4, 800},
//     {NOTE_E4, 800},
//     {NOTE_G4, 800},
//     {NOTE_G4, 800},
//     {NOTE_G4, 800},
//     {NOTE_D4, 800},
//     {NOTE_E4, 800}
// };

static void buzzer_init(void)
{
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = 2000, // default, will be changed per note
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_config);

    ledc_channel_config_t channel_config = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = BUZZER_GPIO,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_config);
}

static void play_tone(int frequency, int duration_ms)
{
    if (frequency == 0) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(duration_ms));
        return;
    }

    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void app_main(void)
{
    buzzer_init();

    while (1) {
        for (size_t i = 0; i < sizeof(melody) / sizeof(melody[0]); i++) {
            play_tone(
                melody[i].frequency,
                melody[i].duration_ms
            );
            vTaskDelay(pdMS_TO_TICKS(20)); // small gap between notes
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // wait before replay
    }
}