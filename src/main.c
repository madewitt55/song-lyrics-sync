#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2s_std.h"

// ---------- I2S43434 Inter-IC Sound MEMS Microphone ----------
// Controller: ESP32-WROOM-32, Peripheral: ICS43434
#define I2S_BCLK GPIO_NUM_5 // Bit clock: controller toggles once per data bit, 32 pulses per audio sample
// Left-right clock (word select): controller toggles once per sample, pulls LOW for left channel, drives HIGH for right channel
// Defines when the peripheral should send a sample on left or right channel
// I2S_SEL pin is tied to GND, meaning the peripheral sends samples when left channel is active
#define I2S_LRCL GPIO_NUM_18
#define I2S_DOUT GPIO_NUM_19 // Serial data: peripheral pulses a 24 bit sample followed by 8 bits of padding to fill 32 bit sample, controller reads data

#define I2S_SAMPLE_RATE       16000 // 16kHz (16000 audio samples per second)
#define I2S_READ_CHUNK_SAMPLES 256  // Samples per i2s_channel_read() call while filling the recording buffer

static i2s_chan_handle_t rx_chan; // Recieve channel; I2S channel object

/**
 * @brief Configure and enable the ESP32's I2S hardware to receive audio from the peripheral
 *
 * @note Assumes the peripheral's L/R pin is tied to GND (left channel)
 */
static void i2s_mic_init(void)
{
    // Use default channel configuration; I2S role is master: controller only recieves data from the peripheral
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan)); // Create I2S channel, bubble up error to serial

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED, // Peripheral does not require master clock
            .bclk = I2S_BCLK, // Bit clock
            .ws   = I2S_LRCL, // Left-right clock aka word select
            .dout = I2S_GPIO_UNUSED, // Controller sends no data
            .din  = I2S_DOUT, // Peripheral data out mapped to controller data in
            .invert_flags = {
                // Nothing inverted
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; // Capture only left I2S channel to buffer

    // Initialize and enable I2S channel, bubble up errors to serial
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

void app_main(void)
{
    i2s_mic_init(); // Enable I2S hardware to recieve audio from peripheral mic
}
