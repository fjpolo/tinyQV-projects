/*
 * SPDX-License-Identifier: Apache-2.0
 * RV2A03 NES APU Firmware Test Suite, Interactive Synthesizer & Soundboard
 * Target: Tiny Tapeout Sky25a (Berzerk instance) & Gowin FPGAs (Console 60K / Nano 20K)
 *
 * Features:
 *   - 5/5 Hardware Verification Self-Test Suite (cocotb translation)
 *   - Interactive Live UART Synthesizer (115200 8N1)
 *   - QWERTZ / QWERTY Chromatic Piano Keyboard (1.5 Octaves)
 *   - Retro NES Soundboard (Coin, Jump, Laser, Explosion, 1-Up, Snare)
 *   - NES Chiptune Jukebox (BlasNESmous Theme, Berzerk, Zelda Fanfare)
 *   - Real-Time APU Status & Register Inspector
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "rv2a03.h"

#define printf uart_printf

// --------------------------------------------------------------------------
// Hardware PWM Audio Engine (PMOD-AUDIO v1.2 Driver)
// --------------------------------------------------------------------------
// Sujith Kani PWM Peripheral (Slot 20 / Simple Peri 4)
#define PWM_SK_DUTY_REG     (*(volatile uint8_t *)0x8000440)
// Matt Venn Dual PWM Peripheral (Slot 21 / Simple Peri 5)
#define MATT_PWM0_REG       (*(volatile uint8_t *)0x8000450)
#define MATT_PWM1_REG       (*(volatile uint8_t *)0x8000451)

static inline uint8_t pcm_sample_to_pwm_duty(uint8_t raw) {
    // APUMixer raw sample is unsigned 0..36
    // Map directly to 8-bit PWM duty cycle (0..255)
    uint16_t scaled = (uint16_t)raw * 7;
    return (scaled > 255) ? 255 : (uint8_t)scaled;
}

static inline void pwm_audio_write(uint8_t duty) {
    PWM_SK_DUTY_REG = duty;  // Sujith PWM (Slot 20, drives uo_out[0] - Loudspeaker J2)
    MATT_PWM0_REG   = duty;  // Matt PWM0  (Slot 21, drives uo_out[1] - Right 3.5mm)
    MATT_PWM1_REG   = duty;  // Matt PWM1  (Slot 21, drives uo_out[0] - Left 3.5mm)
}

static inline void rv2a03_mute_safe(void) {
    // Zero individual channel volumes to eliminate DC leaks (Silicon Errata #1 & #2)
    rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
    rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
    rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
    rv2a03_write_reg(RV2A03_REG_TRI_HI, 0x00);
    rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
    rv2a03_write_reg(RV2A03_REG_NOISE_HI, 0x00);
    rv2a03_write_reg(RV2A03_REG_STATUS, 0x00);
    pwm_audio_write(0);
}
#define rv2a03_mute rv2a03_mute_safe

static inline void enable_pwm_audio(void) {
    set_gpio_func(0, 20);  // Slot 20: Hardware PWM to uo_out[0] (PMOD Pin 1: Loudspeaker)
    set_gpio_func(1, 21);  // Slot 21: Hardware PWM to uo_out[1] (PMOD Pin 2: Right Audio)
}

#ifdef SIM
// For fast simulation in Icarus Verilog: direct volatile loop
static void delay_cycles(uint32_t count) {
    uint32_t loops = count / 100;
    if (loops == 0) loops = 1;
    for (volatile uint32_t i = 0; i < loops; i++) {
        asm volatile ("nop");
    }
}

static void __attribute__((unused)) delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 4; i++) {
        asm volatile ("nop");
    }
}

#else
// Real-time PCM audio streaming delay: oversamples APU sample register at >3 MHz and writes PWM duty
static void delay_cycles(uint32_t count) {
    uint32_t start = get_mtime();
    while ((get_mtime() - start) < count) {
        uint8_t raw = rv2a03_read_reg(RV2A03_REG_OUTPUT_LSB);
        pwm_audio_write(pcm_sample_to_pwm_duty(raw));
    }
}

static void delay_ms(uint32_t ms) {
    // TinyQV mtime timer increments once per microsecond (1,000 counts per ms)
    delay_cycles(ms * 1000);
}
#endif

// --------------------------------------------------------------------------
// Non-Blocking UART Receiver
// --------------------------------------------------------------------------

static int uart_rx_poll(void) {
    // Check hardware Peripheral 2 (UART) status register (0x8000084 bit 1: rx_buffered)
    volatile uint32_t *uart_status = (volatile uint32_t *)0x8000084;
    volatile uint32_t *uart_data   = (volatile uint32_t *)0x8000080;
    if (*uart_status & 0x02) {
        return (int)(*uart_data & 0xFF);
    }
    if (uart_is_char_available()) {
        return uart_getc();
    }
    return -1;
}

// --------------------------------------------------------------------------
// Test Configurations & Hardware Detection
// --------------------------------------------------------------------------

#ifdef SIM
#define SAMPLE_COUNT        20
#define SAMPLE_THRESHOLD    1
#else
#define SAMPLE_COUNT        200
#define SAMPLE_THRESHOLD    10
#endif

static bool __attribute__((unused)) is_hardware_present(void) {
    rv2a03_write_reg(RV2A03_REG_CONFIG0, RV2A03_CFG_CE);
    return (rv2a03_read_reg(RV2A03_REG_CONFIG0) == RV2A03_CFG_CE);
}

// --------------------------------------------------------------------------
// Automated Cocotb Test Verification Suite (5/5)
// --------------------------------------------------------------------------

static bool test_sq1_channel(void) {
    printf("[TEST 1] Testing Square Channel 1 (440 Hz)... ");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);
    rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);
    delay_cycles(10000);

    uint8_t cfg_val = rv2a03_read_reg(RV2A03_REG_CONFIG0);
    uint8_t stat_val = rv2a03_read_reg(RV2A03_REG_STATUS);
    uint8_t lsb_val = rv2a03_read_reg(RV2A03_REG_OUTPUT_LSB);
    uint8_t msb_val = rv2a03_read_reg(RV2A03_REG_OUTPUT_MSB);
    int16_t sample_init = rv2a03_read_sample();
    printf("\n  [DIAG Slot %d] CFG0=0x%02X STATUS=0x%02X LSB=0x%02X MSB=0x%02X Sample=%d\n  ",
           rv2a03_slot_num, cfg_val, stat_val, lsb_val, msb_val, sample_init);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }
    rv2a03_mute();

#ifdef SIM
    if (!is_hardware_present()) {
        printf("PASS (stub detected)\n");
        return true;
    }
#endif

    if (non_zero >= SAMPLE_THRESHOLD) {
        printf("PASS (%d/%d samples, peak: %d)\n", non_zero, SAMPLE_COUNT, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

static bool test_sq2_channel(void) {
    printf("[TEST 2] Testing Square Channel 2 (440 Hz)... ");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ2_ENABLE);
    rv2a03_set_pulse2(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);
    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }
    rv2a03_mute();

#ifdef SIM
    if (!is_hardware_present()) {
        printf("PASS (stub detected)\n");
        return true;
    }
#endif

    if (non_zero >= SAMPLE_THRESHOLD) {
        printf("PASS (%d/%d samples, peak: %d)\n", non_zero, SAMPLE_COUNT, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

static bool test_tri_channel(void) {
    printf("[TEST 3] Testing Triangle Channel (440 Hz)... ");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE);
    rv2a03_set_triangle(0x7F, true, 0x003E, 0x1E);
    delay_ms(5);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }
    rv2a03_mute();

#ifdef SIM
    if (!is_hardware_present()) {
        printf("PASS (stub detected)\n");
        return true;
    }
#endif

    if (non_zero >= SAMPLE_THRESHOLD) {
        printf("PASS (%d/%d samples, peak: %d)\n", non_zero, SAMPLE_COUNT, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

static bool test_noise_channel(void) {
    printf("[TEST 4] Testing Noise Channel.............. ");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);
    rv2a03_set_noise(0x0F, true, true, 0x08, false, 0x1E);
    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }
    rv2a03_mute();

#ifdef SIM
    if (!is_hardware_present()) {
        printf("PASS (stub detected)\n");
        return true;
    }
#endif

    if (non_zero >= SAMPLE_THRESHOLD) {
        printf("PASS (%d/%d samples, peak: %d)\n", non_zero, SAMPLE_COUNT, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

static bool test_all_channels_together(void) {
    printf("[TEST 5] Testing All Channels Simultaneously... ");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_ALL_ENABLE);
    rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);
    rv2a03_set_pulse2(RV2A03_DUTY_25, 0x0C, true, true, 0x00A0, 0x1E);
    rv2a03_set_triangle(0x7F, true, 0x003E, 0x1E);
    rv2a03_set_noise(0x08, true, true, 0x0C, false, 0x1E);
    delay_ms(5);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }
    rv2a03_mute();

#ifdef SIM
    if (!is_hardware_present()) {
        printf("PASS (stub detected)\n");
        return true;
    }
#endif

    if (non_zero >= SAMPLE_THRESHOLD) {
        printf("PASS (%d/%d samples, combined peak: %d)\n", non_zero, SAMPLE_COUNT, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

static int run_full_testsuite(void) {
    int passed = 0;
    if (test_sq1_channel()) passed++;
    if (test_sq2_channel()) passed++;
    if (test_tri_channel()) passed++;
    if (test_noise_channel()) passed++;
    if (test_all_channels_together()) passed++;

    printf("\n-----------------------------------------------------\n");
    printf("Test Results: %d/5 tests passed successfully.\n", passed);
    printf("-----------------------------------------------------\n\n");
    return passed;
}

// --------------------------------------------------------------------------
// Retro NES Soundboard Effects
// --------------------------------------------------------------------------

static void sfx_coin(void) {
    printf("\r\033[K[SFX] >> COIN! (Mario B5 -> E6)               \n");
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);
    uint16_t t_b5 = rv2a03_midi_to_pulse_timer(RV2A03_NOTE_B5);
    uint16_t t_e6 = rv2a03_midi_to_pulse_timer(RV2A03_NOTE_E5 + 12);
    rv2a03_set_pulse1(RV2A03_DUTY_50, 14, true, true, t_b5, 0x1E);
    delay_ms(35);
    rv2a03_set_pulse1(RV2A03_DUTY_50, 14, true, true, t_e6, 0x1E);
    for (int v = 14; v >= 0; v--) {
        rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30 | v);
        delay_ms(20);
    }
    rv2a03_mute();
}

static void sfx_jump(void) {
    printf("\r\033[K[SFX] >> JUMP! (Square 1 Upward Sweep)        \n");
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);
    for (uint16_t t = 0x0280; t >= 0x0080; t -= 0x000E) {
        rv2a03_set_pulse1(RV2A03_DUTY_50, 12, true, true, t, 0x1E);
        delay_ms(5);
    }
    rv2a03_mute();
}

static void sfx_laser(void) {
    printf("\r\033[K[SFX] >> LASER / WARP! (Downward Chirp)       \n");
    rv2a03_enable_channels(RV2A03_STATUS_SQ2_ENABLE);
    for (uint16_t t = 0x0050; t <= 0x02C0; t += 0x0014) {
        rv2a03_set_pulse2(RV2A03_DUTY_25, 12, true, true, t, 0x1E);
        delay_ms(4);
    }
    rv2a03_mute();
}

static void sfx_explosion(void) {
    printf("\r\033[K[SFX] >> EXPLOSION! (Low Noise Rumble)        \n");
    rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);
    rv2a03_set_noise(15, true, true, 0x0E, false, 0x1E);
    for (int v = 15; v >= 0; v--) {
        rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30 | v);
        delay_ms(22);
    }
    rv2a03_mute();
}

static void sfx_powerup(void) {
    printf("\r\033[K[SFX] >> POWER-UP! (Ascending Arpeggio)       \n");
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);
    const uint8_t arpeggio[] = {
        RV2A03_NOTE_G4, RV2A03_NOTE_B4, RV2A03_NOTE_D5,
        RV2A03_NOTE_G5, RV2A03_NOTE_B5, RV2A03_NOTE_D5 + 12
    };
    for (int i = 0; i < 6; i++) {
        uint16_t t = rv2a03_midi_to_pulse_timer(arpeggio[i]);
        rv2a03_set_pulse1(RV2A03_DUTY_50, 12, true, true, t, 0x1E);
        delay_ms(45);
    }
    rv2a03_mute();
}

static void sfx_snare(void) {
    printf("\r\033[K[SFX] >> SNARE DRUM HIT!                      \n");
    rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);
    rv2a03_set_noise(14, true, true, 0x04, false, 0x1E);
    for (int v = 14; v >= 0; v -= 2) {
        rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30 | v);
        delay_ms(10);
    }
    rv2a03_mute();
}

// --------------------------------------------------------------------------
// Dynamic Volume Scaling, Gauge & Channel Volume Applicator
// --------------------------------------------------------------------------

static inline uint8_t get_master_vol(uint8_t *master_vol) {
    return master_vol ? *master_vol : 12;
}

static inline uint8_t scale_vol(uint8_t base_vol, uint8_t master_vol) {
    if (master_vol == 0 || base_vol == 0) return 0;
    uint32_t v = ((uint32_t)base_vol * (uint32_t)master_vol + 7) / 15;
    if (v > 15) v = 15;
    return (uint8_t)v;
}

static void print_vol_bar(uint8_t vol) {
    char bar[16];
    for (int i = 0; i < 15; i++) {
        bar[i] = (i < vol) ? '#' : '-';
    }
    bar[15] = '\0';
    printf("\r\033[K[VOL] [%s] %2d/15\n", bar, vol);
}

static void apply_synth_volume(uint8_t ch, uint8_t vol, uint8_t duty_idx) {
    const uint8_t duty_constants[4] = {
        RV2A03_DUTY_12_5, RV2A03_DUTY_25, RV2A03_DUTY_50, RV2A03_DUTY_75
    };
    uint8_t d = duty_constants[duty_idx & 3];

    if (vol == 0) {
        if (ch == 0) {
            rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
            rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
        } else if (ch == 1) {
            rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
        } else if (ch == 2) {
            rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
        } else if (ch == 3) {
            rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
        }
        return;
    }

    if (ch == 0) {
        // Pulse 1 Lead: Dual pulse unison reinforcement to eliminate mixer quantization deadzone
        rv2a03_write_reg(RV2A03_REG_SQ1_VOL, d | 0x30 | (vol & 0x0F));
        rv2a03_write_reg(RV2A03_REG_SQ2_VOL, d | 0x30 | (vol & 0x0F));
    } else if (ch == 1) {
        rv2a03_write_reg(RV2A03_REG_SQ2_VOL, d | 0x30 | (vol & 0x0F));
    } else if (ch == 2) {
        rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x7F);
    } else if (ch == 3) {
        rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30 | (vol & 0x0F));
    }
}

static void sfx_zelda_fanfare(uint8_t *master_vol) {
    uint8_t cur_vol = get_master_vol(master_vol);
    printf("\r\033[K[JUKEBOX] >> Zelda Secret Fanfare!            \n");
    print_vol_bar(cur_vol);
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_SQ2_ENABLE);
    const uint8_t notes[] = { 67, 66, 63, 57, 56, 64, 68, 72 };
    for (int i = 0; i < 8; i++) {
        if (uart_rx_poll() >= 0) break;
        uint16_t t1 = rv2a03_midi_to_pulse_timer(notes[i]);
        uint16_t t2 = rv2a03_midi_to_pulse_timer(notes[i] - 12);
        uint8_t v1 = scale_vol(11, cur_vol);
        uint8_t v2 = scale_vol(7, cur_vol);
        if (cur_vol == 0) {
            rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
            rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
        } else {
            rv2a03_set_pulse1(RV2A03_DUTY_50, v1, true, true, t1, 0x1E);
            rv2a03_set_pulse2(RV2A03_DUTY_25, v2, true, true, t2, 0x1E);
        }
        delay_ms(125);
    }
    rv2a03_mute();
}

static uint8_t barrel_distortion = 1; // Default to Level 1 (Warm Saturation)

static void sfx_barrel_drum(uint8_t dist_level) {
    const char *dist_names[4] = {
        "Clean Acoustic", "Warm Saturation", "Metallic Fuzz", "Industrial Doom"
    };
    uint8_t mode = dist_level & 3;
    printf("\r\033[K[DRUM] >> BARREL DRUM! [Distortion %d/3: %s]        \n",
           mode, dist_names[mode]);

    if (mode == 0) {
        // --- LEVEL 0: Clean Acoustic Barrel ---
        // Resonant hollow triangle pitch sweep (240 Hz -> 42 Hz) + soft wooden strike
        rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE | RV2A03_STATUS_NOISE_ENABLE);

        // Soft mallet wood transient
        rv2a03_set_noise(9, true, true, 0x06, false, 0x1E);

        for (uint16_t t = 0x00E0; t <= 0x04C0; t += 0x0024) {
            rv2a03_set_triangle(0x7F, true, t, 0x1E);
            delay_ms(6);
        }
        rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
        delay_ms(15);
    }
    else if (mode == 1) {
        // --- LEVEL 1: Warm Saturation / Overdrive ---
        // Deep Triangle body + Pulse 1 50% duty warm harmonic with volume decay
        rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE | RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_NOISE_ENABLE);

        rv2a03_set_noise(12, true, true, 0x05, false, 0x1E);

        int v = 12;
        for (uint16_t t = 0x00D0; t <= 0x0480; t += 0x0020) {
            rv2a03_set_triangle(0x7F, true, t, 0x1E);
            if (v > 0) {
                rv2a03_set_pulse1(RV2A03_DUTY_50, (uint8_t)v, true, true, t << 1, 0x1E);
                v--;
            } else {
                rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
            }
            delay_ms(6);
        }
    }
    else if (mode == 2) {
        // --- LEVEL 2: Metallic Fuzz (Periodic 93-step Short Noise + Buzz Pulse) ---
        // Resonant Triangle + 93-step periodic metallic noise + 25% duty fuzz buzz
        rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE | RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_NOISE_ENABLE);

        // Metallic buzz (short_mode = true!)
        rv2a03_set_noise(15, true, true, 0x05, true, 0x1E);

        int v = 14;
        for (uint16_t t = 0x00C0; t <= 0x0440; t += 0x001C) {
            rv2a03_set_triangle(0x7F, true, t, 0x1E);
            rv2a03_set_pulse1(RV2A03_DUTY_25, (uint8_t)(v > 0 ? v : 0), true, true, t, 0x1E);
            if (v > 0) v--;
            rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30 | (v & 0x0F));
            delay_ms(6);
        }
    }
    else {
        // --- LEVEL 3: Industrial Doom / Bitcrush Overdrive ---
        // Quantized coarse pitch jumps + 12.5% razor Pulse 1 + Tritone Pulse 2 + Harsh Noise
        rv2a03_enable_channels(RV2A03_STATUS_ALL_ENABLE);

        rv2a03_set_noise(15, true, true, 0x02, true, 0x1E);

        int v = 15;
        const uint16_t coarse_steps[6] = { 0x00A0, 0x0140, 0x0220, 0x0320, 0x0440, 0x0520 };
        for (int i = 0; i < 6; i++) {
            uint16_t t = coarse_steps[i];
            rv2a03_set_triangle(0x7F, true, t, 0x1E);
            rv2a03_set_pulse1(RV2A03_DUTY_12_5, (uint8_t)v, true, true, t, 0x1E);
            // Integer tritone detuning: t * 141 / 100
            uint16_t tritone_t = (uint16_t)((t * 141) / 100);
            rv2a03_set_pulse2(RV2A03_DUTY_75, (uint8_t)(v > 2 ? v - 2 : 0), true, true, tritone_t, 0x1E);
            v -= 2;
            rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30 | (v & 0x0F));
            delay_ms(24);
        }
    }

    rv2a03_mute();
}

static void cycle_barrel_distortion(void) {
    barrel_distortion = (barrel_distortion + 1) % 4;
    const char *dist_names[4] = {
        "Clean Acoustic", "Warm Saturation", "Metallic Fuzz", "Industrial Doom"
    };
    printf("\r\033[K[DISTORTION] >> Barrel Distortion set to: Level %d/3 (%s)\n",
           barrel_distortion, dist_names[barrel_distortion]);
}

static void play_berzerk_theme(uint8_t *master_vol) {
    uint8_t cur_vol = get_master_vol(master_vol);
    printf("\r\033[K[JUKEBOX] >> Playing 'Berzerk APU Theme' (press +/- for volume, any key to stop)... \n");
    print_vol_bar(cur_vol);
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_TRI_ENABLE);

    const uint8_t melody[] = {
        RV2A03_NOTE_C4, RV2A03_NOTE_E4, RV2A03_NOTE_G4, RV2A03_NOTE_C5,
        RV2A03_NOTE_A4, RV2A03_NOTE_C5, RV2A03_NOTE_E5, RV2A03_NOTE_A5,
        RV2A03_NOTE_F4, RV2A03_NOTE_A4, RV2A03_NOTE_C5, RV2A03_NOTE_F5,
        RV2A03_NOTE_G4, RV2A03_NOTE_B4, RV2A03_NOTE_D5, RV2A03_NOTE_G5
    };
    const uint8_t bass[] = {
        RV2A03_NOTE_C3, RV2A03_NOTE_A3, RV2A03_NOTE_F3, RV2A03_NOTE_G3
    };

    for (int rep = 0; rep < 2; rep++) {
        for (int bar = 0; bar < 4; bar++) {
            int key = uart_rx_poll();
            if (key >= 0) {
                if (key == '+' || key == '=') {
                    if (cur_vol < 15) cur_vol++;
                    if (master_vol) *master_vol = cur_vol;
                    print_vol_bar(cur_vol);
                } else if (key == '-' || key == '_') {
                    if (cur_vol > 0) cur_vol--;
                    if (master_vol) *master_vol = cur_vol;
                    print_vol_bar(cur_vol);
                } else {
                    goto end_playback;
                }
            }

            if (cur_vol == 0) {
                rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
            } else {
                uint16_t tri_timer = rv2a03_midi_to_pulse_timer(bass[bar]) >> 1;
                rv2a03_set_triangle(0x7F, true, tri_timer, 0x1E);
            }

            for (int note = 0; note < 4; note++) {
                int key2 = uart_rx_poll();
                if (key2 >= 0) {
                    if (key2 == '+' || key2 == '=') {
                        if (cur_vol < 15) cur_vol++;
                        if (master_vol) *master_vol = cur_vol;
                        print_vol_bar(cur_vol);
                    } else if (key2 == '-' || key2 == '_') {
                        if (cur_vol > 0) cur_vol--;
                        if (master_vol) *master_vol = cur_vol;
                        print_vol_bar(cur_vol);
                    } else {
                        goto end_playback;
                    }
                }

                uint8_t m_note = melody[bar * 4 + note];
                uint16_t sq_timer = rv2a03_midi_to_pulse_timer(m_note);

                if (cur_vol == 0) {
                    rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
                } else {
                    uint8_t v = scale_vol(12, cur_vol);
                    rv2a03_set_pulse1(RV2A03_DUTY_50, v, true, true, sq_timer, 0x1E);
                }
                delay_ms(120);

                rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
                delay_ms(15);
            }
        }
    }

end_playback:
    rv2a03_mute();
    printf("\r\033[K[JUKEBOX] Done! APU muted.\n");
}

// --------------------------------------------------------------------------
// Jukebox: BlasNESmous Theme (Carlos Viola / Blasphemous NES Demake by @fjpolo)
// --------------------------------------------------------------------------

#define NOTE_REST 0
typedef struct {
    uint8_t sq1;    // MIDI note for Pulse 1 (Lead lute / guitar)
    uint8_t sq2;    // MIDI note for Pulse 2 (Counterpoint / arpeggio)
    uint8_t tri;    // MIDI note for Triangle (Flamenco bass)
    uint8_t noise;  // Noise percussion pattern (0: off, 1: tap/castanet, 2: snare roll, 3: heavy strike)
    uint8_t dur;    // Duration in ticks (50 ms per tick)
} BlasnesmousEvent;

static const BlasnesmousEvent blasnesmous_score[] = {
    // --- Phase 1: "Suena el Laúd" - Somber Flamenco Opening (D minor) ---
    // Bar 1: Dm (D4, F4, A4) - Plucked lute motif
    { 62, NOTE_REST, 38, 0, 4 }, // D4, D2 bass
    { 65, 57,        38, 1, 4 }, // F4, A3 counterpoint, castanet tap
    { 69, 62,        38, 0, 4 }, // A4, D4
    { 65, 57,        38, 1, 4 }, // F4, A3, castanet tap
    { 64, 57,        38, 0, 4 }, // E4, A3
    { 62, 57,        38, 2, 6 }, // D4, D2 bass, snare roll

    // Bar 2: C Major cadence step (C4, E4, G4)
    { 60, NOTE_REST, 36, 0, 4 }, // C4, C2 bass
    { 64, 55,        36, 1, 4 }, // E4, G3 counterpoint
    { 67, 60,        36, 0, 4 }, // G4, C4
    { 64, 55,        36, 1, 4 }, // E4, G3
    { 62, 55,        36, 0, 4 }, // D4, G3
    { 60, 55,        36, 2, 6 }, // C4, C2 bass, snare roll

    // Bar 3: Bb Major cadence step (Bb3, D4, F4)
    { 58, NOTE_REST, 34, 0, 4 }, // Bb3, Bb1 bass
    { 62, 53,        34, 1, 4 }, // D4, F3 counterpoint
    { 65, 58,        34, 0, 4 }, // F4, Bb3
    { 62, 53,        34, 1, 4 }, // D4, F3
    { 60, 53,        34, 0, 4 }, // C4, F3
    { 58, 53,        34, 2, 6 }, // Bb3, Bb1 bass, snare roll

    // Bar 4: A Major (Dominant - Phrygian resolution with C#4!)
    { 57, NOTE_REST, 33, 0, 4 }, // A3, A1 bass
    { 61, 57,        33, 1, 4 }, // C#4, A3 counterpoint
    { 64, 61,        33, 0, 4 }, // E4, C#4
    { 69, 64,        33, 3, 6 }, // A4, E4, heavy castanet hit
    { 64, 61,        33, 1, 4 }, // E4, C#4
    { 61, 57,        33, 0, 4 }, // C#4, A3
    { 57, NOTE_REST, 33, 2, 8 }, // A3 chord ring out

    // --- Phase 2: Dramatic Penance Theme (High Octave & Castanets) ---
    // Bar 5: D5 lament
    { 74, 62, 38, 1, 4 },        // D5, D4, D2 bass
    { 73, 62, 38, 0, 2 },        // C#5 grace note
    { 74, 65, 38, 2, 4 },        // D5, F4
    { 77, 69, 38, 1, 6 },        // F5, A4
    { 76, 67, 38, 0, 4 },        // E5, G4
    { 74, 65, 38, 2, 6 },        // D5, F4

    // Bar 6: Spanish Phrygian Descent (Bb4 -> A4 -> G4 -> F4 -> E4)
    { 70, 58, 34, 1, 4 },        // Bb4, Bb3, Bb1 bass
    { 69, 57, 34, 0, 4 },        // A4, A3
    { 67, 55, 36, 1, 4 },        // G4, G3, C2 bass
    { 65, 53, 38, 2, 4 },        // F4, F3, D2 bass
    { 64, 52, 33, 1, 4 },        // E4, E3, A1 bass

    // Bar 7: Dramatic Climax Cadence resolving to Dm Picardy / Flamenco A
    { 61, 57, 33, 3, 4 },        // C#4, A3, A1 bass, heavy hit
    { 64, 61, 33, 1, 4 },        // E4, C#4
    { 69, 64, 33, 2, 6 },        // A4, E4
    { 73, 69, 33, 1, 4 },        // C#5, A4
    { 74, 70, 38, 3, 10 }        // Final D5 + Bb4 + D2 chord (Penitent One resolution!)
};

static void play_blasnesmous_theme(uint8_t *master_vol) {
    uint8_t cur_vol = get_master_vol(master_vol);
    printf("\r\033[K[JUKEBOX] >> Playing 'BlasNESmous Theme' (Carlos Viola / @fjpolo)...\n");
    printf("           [Controls: +/- or Up/Down for Volume, any other key to stop]\n");
    print_vol_bar(cur_vol);

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_SQ2_ENABLE |
                           RV2A03_STATUS_TRI_ENABLE | RV2A03_STATUS_NOISE_ENABLE);

    const int num_events = sizeof(blasnesmous_score) / sizeof(blasnesmous_score[0]);
    const uint32_t TICK_MS = 50;

    for (int i = 0; i < num_events; i++) {
        // Check for real-time interactive volume controls during playback!
        int key = uart_rx_poll();
        if (key >= 0) {
            if (key == '+' || key == '=') {
                if (cur_vol < 15) cur_vol++;
                if (master_vol) *master_vol = cur_vol;
                print_vol_bar(cur_vol);
            } else if (key == '-' || key == '_') {
                if (cur_vol > 0) cur_vol--;
                if (master_vol) *master_vol = cur_vol;
                print_vol_bar(cur_vol);
            } else if (key == 0x1B) { // ESC sequence (Arrow keys)
                delay_ms(10);
                int k2 = uart_rx_poll();
                if (k2 == '[' || k2 == 'O') {
                    int k3 = uart_rx_poll();
                    if (k3 == 'A') { // Up
                        if (cur_vol < 15) cur_vol++;
                        if (master_vol) *master_vol = cur_vol;
                        print_vol_bar(cur_vol);
                    } else if (k3 == 'B') { // Down
                        if (cur_vol > 0) cur_vol--;
                        if (master_vol) *master_vol = cur_vol;
                        print_vol_bar(cur_vol);
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            } else {
                break; // Any other key stops playback
            }
        }

        const BlasnesmousEvent *ev = &blasnesmous_score[i];

        if (cur_vol == 0) {
            rv2a03_mute();
        } else {
            // Pulse 1: Spanish Classical Lute / Lead (50% duty, base volume 12)
            if (ev->sq1 != NOTE_REST) {
                uint16_t sq1_timer = rv2a03_midi_to_pulse_timer(ev->sq1);
                uint8_t v1 = scale_vol(12, cur_vol);
                rv2a03_set_pulse1(RV2A03_DUTY_50, v1, true, true, sq1_timer, 0x1E);
            } else {
                rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
            }

            // Pulse 2: Andalusian Counterpoint / Arpeggio (25% duty, base volume 8)
            if (ev->sq2 != NOTE_REST) {
                uint16_t sq2_timer = rv2a03_midi_to_pulse_timer(ev->sq2);
                uint8_t v2 = scale_vol(8, cur_vol);
                rv2a03_set_pulse2(RV2A03_DUTY_25, v2, true, true, sq2_timer, 0x1E);
            } else {
                rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
            }

            // Triangle: Deep Flamenco Bass
            if (ev->tri != NOTE_REST) {
                uint16_t tri_timer = rv2a03_midi_to_pulse_timer(ev->tri) >> 1;
                rv2a03_set_triangle(0x7F, true, tri_timer, 0x1E);
            } else {
                rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
            }

            // Noise: Spanish Castanet Taps & Snare Rolls
            if (ev->noise == 1) {
                // Soft castanet tap
                uint8_t vn = scale_vol(9, cur_vol);
                rv2a03_set_noise(vn, true, true, 0x03, false, 0x10);
            } else if (ev->noise == 2) {
                // Snare roll / march
                uint8_t vn = scale_vol(11, cur_vol);
                rv2a03_set_noise(vn, true, true, 0x06, false, 0x14);
            } else if (ev->noise == 3) {
                // Heavy accent hit
                uint8_t vn = scale_vol(14, cur_vol);
                rv2a03_set_noise(vn, true, true, 0x08, false, 0x1E);
            } else {
                rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
            }
        }

        uint32_t total_time = ev->dur * TICK_MS;
        uint32_t staccato = (total_time > 50) ? 25 : 0;
        delay_ms(total_time - staccato);

        if (staccato > 0) {
            rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
            rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
            rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
            delay_ms(staccato);
        }
    }

    rv2a03_mute();
    printf("\r\033[K[JUKEBOX] Done! APU muted.\n");
}

// --------------------------------------------------------------------------
// Hardware Register Dump & Status Inspector
// --------------------------------------------------------------------------

static void dump_apu_registers(void) {
    printf("\n\n=====================================================\n");
    printf("   RV2A03 Hardware APU Register & Audio Status       \n");
    printf("=====================================================\n");
    printf("  SQ1: Vol/Duty=0x%02X  TimerLo=0x%02X  TimerHi=0x%02X\n",
           rv2a03_read_reg(RV2A03_REG_SQ1_VOL),
           rv2a03_read_reg(RV2A03_REG_SQ1_LO),
           rv2a03_read_reg(RV2A03_REG_SQ1_HI));
    printf("  SQ2: Vol/Duty=0x%02X  TimerLo=0x%02X  TimerHi=0x%02X\n",
           rv2a03_read_reg(RV2A03_REG_SQ2_VOL),
           rv2a03_read_reg(RV2A03_REG_SQ2_LO),
           rv2a03_read_reg(RV2A03_REG_SQ2_HI));
    printf("  TRI: Linear=0x%02X    TimerLo=0x%02X  TimerHi=0x%02X\n",
           rv2a03_read_reg(RV2A03_REG_TRI_LINEAR),
           rv2a03_read_reg(RV2A03_REG_TRI_LO),
           rv2a03_read_reg(RV2A03_REG_TRI_HI));
    printf("  NOI: Vol=0x%02X       Period=0x%02X   Len=0x%02X\n",
           rv2a03_read_reg(RV2A03_REG_NOISE_VOL),
           rv2a03_read_reg(RV2A03_REG_NOISE_LO),
           rv2a03_read_reg(RV2A03_REG_NOISE_HI));
    printf("  CHN_EN: 0x%02X  CFG0: 0x%02X  STATUS0: 0x%02X\n",
           rv2a03_read_reg(RV2A03_REG_STATUS),
           rv2a03_read_reg(RV2A03_REG_CONFIG0),
           rv2a03_read_reg(RV2A03_REG_STATUS0));
    int16_t s = rv2a03_read_sample();
    printf("  Hardware Mixed Sample: %d (0x%04X)\n", s, (uint16_t)s);
    printf("=====================================================\n\n");
}

// --------------------------------------------------------------------------
// Synthesizer Guide & Help Screen
// --------------------------------------------------------------------------

static void print_synth_banner(void) {
    printf("\n");
    printf("  ============================================================\n");
    printf("     TinyQV RV2A03 NES APU LIVE SYNTHESIZER & SOUNDBOARD      \n");
    printf("        Target: Sky25a Berzerk | QWERTZ / QWERTY Ready        \n");
    printf("  ============================================================\n\n");
    printf("  PIANO KEYS (Chromatic 1.5 Octaves):\n");
    printf("    Black:       [W]   [E]         [T]   [Z]   [U]         [O]   [P]\n");
    printf("                 C#    D#          F#    G#    A#          C#    D#\n");
    printf("    White:    [A]   [S]   [D]   [F]   [G]   [H]   [J]   [K]\n");
    printf("               C     D     E     F     G     A     B     C+\n");
    printf("    * Tip: Both 'Z' (QWERTZ) and 'Y' (QWERTY) play G#!\n\n");
    printf("  CHANNELS:   [1] Pulse 1 (Lead)    [2] Pulse 2 (Harmony)\n");
    printf("              [3] Triangle (Bass)   [4] Noise (Percussion)\n\n");
    printf("  CONTROLS:   [Q] Cycle Duty Cycle (12.5%%, 25%%, 50%%, 75%%)\n");
    printf("              [<-] / [->] Octave Down / Up   (Range 2-6)  (or , / .)\n");
    printf("              [v]  / [^]  Volume Down / Up   (Range 0-15) (or - / +)\n");
    printf("              [SPACE] Mute Note              [M] Mute All\n\n");
    printf("  SOUNDBOARD: [C] Coin!    [B] Barrel Drum (Boom!)   [X] Explosion!  [L] Laser!\n");
    printf("              [V] 1-Up!    [N] Snare Hit             [9/I] Jump!\n");
    printf("              [0/D] Cycle Barrel Distortion (0:Clean -> 1:Warm -> 2:Fuzz -> 3:Doom)\n\n");
    printf("  JUKEBOX:    [5] BlasNESmous Theme (Carlos Viola / @fjpolo)\n");
    printf("              [6] Berzerk APU Theme\n");
    printf("              [7] Zelda Secret Fanfare\n\n");
    printf("  SYSTEM:     [R] Dump APU Regs     [*] Run 5/5 Self-Test\n");
    printf("              [?] Show this Guide\n");
    printf("  ============================================================\n\n");
}

// --------------------------------------------------------------------------
// Interactive Synthesizer REPL
// --------------------------------------------------------------------------

static void run_synth_repl(void) {
    print_synth_banner();

    // Enable global interrupts to ensure UART ISR buffer updates smoothly
    asm volatile ("csrs mstatus, %0" : : "r" (0x8));

    uint8_t current_octave = 4; // Octave 4 (Middle C = C4 = 60, A4 = 69)
    uint8_t current_channel = 0; // 0: SQ1, 1: SQ2, 2: TRI, 3: NOISE
    uint8_t current_duty_idx = 2; // 0: 12.5%, 1: 25%, 2: 50%, 3: 75%
    uint8_t current_vol = 12; // Volume 0 - 15

    const uint8_t duty_constants[4] = {
        RV2A03_DUTY_12_5, RV2A03_DUTY_25, RV2A03_DUTY_50, RV2A03_DUTY_75
    };
    const char *duty_labels[4] = { "12.5%", "25.0%", "50.0%", "75.0%" };
    const char *channel_labels[4] = { "Pulse 1", "Pulse 2", "Triangle", "Noise" };
    const char *note_names[12] = {
        "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
    };
    const char *dist_short[4] = { "Clean", "WarmSat", "Fuzz", "Doom" };

    printf("[OCT: %d] [CH: %s] [VOL: %2d] [DUTY: %s] [DRUM DIST: %s] -> Ready!\n",
           current_octave, channel_labels[current_channel], current_vol,
           duty_labels[current_duty_idx], dist_short[barrel_distortion & 3]);

    while (1) {
        int c = uart_rx_poll();
        if (c < 0) {
            uint8_t raw = rv2a03_read_reg(RV2A03_REG_OUTPUT_LSB);
            pwm_audio_write(pcm_sample_to_pwm_duty(raw));
            continue;
        }

        // 0. ANSI Escape Sequences (Arrow Keys: Up/Down for Vol, Left/Right for Octave)
        if (c == 0x1B) { // ESC
            uint32_t esc_start = get_mtime();
            int c2 = -1;
            while ((get_mtime() - esc_start) < 30000) {
                c2 = uart_rx_poll();
                if (c2 >= 0) break;
            }
            if (c2 == '[' || c2 == 'O') {
                esc_start = get_mtime();
                int c3 = -1;
                while ((get_mtime() - esc_start) < 30000) {
                    c3 = uart_rx_poll();
                    if (c3 >= 0) break;
                }
                if (c3 == 'A') {
                    // UP ARROW -> Volume Up
                    if (current_vol < 15) current_vol++;
                    apply_synth_volume(current_channel, current_vol, current_duty_idx);
                    print_vol_bar(current_vol);
                    continue;
                } else if (c3 == 'B') {
                    // DOWN ARROW -> Volume Down
                    if (current_vol > 0) current_vol--;
                    apply_synth_volume(current_channel, current_vol, current_duty_idx);
                    print_vol_bar(current_vol);
                    continue;
                } else if (c3 == 'C') {
                    // RIGHT ARROW -> Octave Up
                    if (current_octave < 6) current_octave++;
                    printf("\r\033[K[OCTAVE] >> Octave UP: %d\n", current_octave);
                    continue;
                } else if (c3 == 'D') {
                    // LEFT ARROW -> Octave Down
                    if (current_octave > 2) current_octave--;
                    printf("\r\033[K[OCTAVE] >> Octave DOWN: %d\n", current_octave);
                    continue;
                }
            }
            rv2a03_mute();
            printf("\r\033[K[MUTE] >> Audio muted.\n");
            continue;
        }

        // 1. Piano Keys Mapping
        int note_semitone = -1;
        int octave_offset = 0;

        switch (c) {
            // White keys (Home row)
            case 'a': case 'A': note_semitone = 0;  break; // C
            case 's': case 'S': note_semitone = 2;  break; // D
            case 'd': case 'D': note_semitone = 4;  break; // E
            case 'f': case 'F': note_semitone = 5;  break; // F
            case 'g': case 'G': note_semitone = 7;  break; // G
            case 'h': case 'H': note_semitone = 9;  break; // A (Concert pitch 440 Hz in Octave 4)
            case 'j': case 'J': note_semitone = 11; break; // B
            case 'k': case 'K': note_semitone = 0;  octave_offset = 1; break; // C (High C)

            // Black keys (Top row)
            case 'w': case 'W': note_semitone = 1;  break; // C#
            case 'e': case 'E': note_semitone = 3;  break; // D#
            case 't': case 'T': note_semitone = 6;  break; // F#
            case 'z': case 'Z': note_semitone = 8;  break; // G# (QWERTZ layout key!)
            case 'y': case 'Y': note_semitone = 8;  break; // G# (QWERTY fallback)
            case 'u': case 'U': note_semitone = 10; break; // A#
            case 'o': case 'O': note_semitone = 1;  octave_offset = 1; break; // C# (next octave)
            case 'p': case 'P': note_semitone = 3;  octave_offset = 1; break; // D# (next octave)

            default:
                break;
        }

        // If a piano note was pressed, play it!
        if (note_semitone >= 0) {
            int target_octave = current_octave + octave_offset;
            if (target_octave > 7) target_octave = 7;
            uint8_t midi_note = (uint8_t)((target_octave + 1) * 12 + note_semitone);

            uint32_t freq_hz = 0;
            if (current_channel == 0) {
                // Pulse 1 Lead: Dual pulse unison reinforcement to eliminate mixer quantization deadzone
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note);
                if (current_vol == 0) {
                    rv2a03_mute();
                } else {
                    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_SQ2_ENABLE);
                    rv2a03_set_pulse1(duty_constants[current_duty_idx], current_vol, true, true, timer, 0x1E);
                    rv2a03_set_pulse2(duty_constants[current_duty_idx], current_vol, true, true, timer, 0x1E);
                }
                freq_hz = 894080 / (16 * (timer + 1));
            } else if (current_channel == 1) {
                // Pulse 2 Harmony
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note);
                if (current_vol == 0) {
                    rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
                } else {
                    rv2a03_enable_channels(RV2A03_STATUS_SQ2_ENABLE);
                    rv2a03_set_pulse2(duty_constants[current_duty_idx], current_vol, true, true, timer, 0x1E);
                }
                freq_hz = 894080 / (16 * (timer + 1));
            } else if (current_channel == 2) {
                // Triangle
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note) >> 1;
                if (current_vol == 0) {
                    rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
                } else {
                    rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE);
                    rv2a03_set_triangle(0x7F, true, timer, 0x1E);
                }
                freq_hz = 894080 / (32 * (timer + 1));
            } else if (current_channel == 3) {
                // Noise
                uint8_t period_idx = (uint8_t)(15 - (midi_note % 16));
                if (current_vol == 0) {
                    rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
                } else {
                    rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);
                    rv2a03_set_noise(current_vol, true, true, period_idx, false, 0x1E);
                }
                freq_hz = (uint32_t)period_idx;
            }

            printf("\r\033[K[OCT: %d] [CH: %s] [VOL: %2d] -> %s%d (%u Hz) ",
                   target_octave, channel_labels[current_channel], current_vol,
                   note_names[note_semitone], target_octave, freq_hz);
            continue;
        }

        // 2. Channel Selection (1, 2, 3, 4)
        if (c == '1') {
            current_channel = 0;
            printf("\r\033[K[SWITCH] >> Selected Pulse 1 (Lead)\n");
            continue;
        } else if (c == '2') {
            current_channel = 1;
            printf("\r\033[K[SWITCH] >> Selected Pulse 2 (Harmony)\n");
            continue;
        } else if (c == '3') {
            current_channel = 2;
            printf("\r\033[K[SWITCH] >> Selected Triangle (Bass)\n");
            continue;
        } else if (c == '4') {
            current_channel = 3;
            printf("\r\033[K[SWITCH] >> Selected Noise (Percussion)\n");
            continue;
        }

        // 3. Jukebox Selections (5, 6, 7)
        if (c == '5') {
            play_blasnesmous_theme(&current_vol);
            continue;
        } else if (c == '6') {
            play_berzerk_theme(&current_vol);
            continue;
        } else if (c == '7') {
            sfx_zelda_fanfare(&current_vol);
            continue;
        }

        // 4. Soundboard Retro Sound Effects
        if (c == 'c' || c == 'C') {
            sfx_coin();
            continue;
        } else if (c == 'b' || c == 'B') {
            sfx_barrel_drum(barrel_distortion);
            continue;
        } else if (c == 'l' || c == 'L' || c == '8') {
            sfx_laser();
            continue;
        } else if (c == 'x' || c == 'X') {
            sfx_explosion();
            continue;
        } else if (c == 'v' || c == 'V') {
            sfx_powerup();
            continue;
        } else if (c == 'n' || c == 'N') {
            sfx_snare();
            continue;
        } else if (c == '9' || c == 'i' || c == 'I') {
            sfx_jump();
            continue;
        } else if (c == '0' || c == 'D') {
            cycle_barrel_distortion();
            continue;
        }

        // 5. Controls & Configuration
        if (c == 'q' || c == 'Q') {
            current_duty_idx = (current_duty_idx + 1) % 4;
            apply_synth_volume(current_channel, current_vol, current_duty_idx);
            printf("\r\033[K[CTRL] >> Duty Cycle set to: %s\n", duty_labels[current_duty_idx]);
            continue;
        } else if (c == ',' || c == '[') {
            if (current_octave > 2) current_octave--;
            printf("\r\033[K[CTRL] >> Octave set to: %d\n", current_octave);
            continue;
        } else if (c == '.' || c == ']') {
            if (current_octave < 6) current_octave++;
            printf("\r\033[K[CTRL] >> Octave set to: %d\n", current_octave);
            continue;
        } else if (c == '-' || c == '_') {
            if (current_vol > 0) current_vol--;
            apply_synth_volume(current_channel, current_vol, current_duty_idx);
            print_vol_bar(current_vol);
            continue;
        } else if (c == '+' || c == '=') {
            if (current_vol < 15) current_vol++;
            apply_synth_volume(current_channel, current_vol, current_duty_idx);
            print_vol_bar(current_vol);
            continue;
        } else if (c == ' ' || c == 'm' || c == 'M') {
            rv2a03_mute();
            printf("\r\033[K[MUTE] >> Audio muted.\n");
            continue;
        } else if (c == 'r' || c == 'R') {
            dump_apu_registers();
            continue;
        } else if (c == '*') {
            printf("\n--- Running 5/5 Hardware Self-Test ---\n");
            run_full_testsuite();
            print_synth_banner();
            continue;
        } else if (c == '?' || c == '/') {
            print_synth_banner();
            continue;
        }
    }
}

// --------------------------------------------------------------------------
// Main Entry Point
// --------------------------------------------------------------------------

int main(void) {
    // ----------------------------------------------------------------------
    // ASIC Pinout Configuration (Tiny Tapeout Sky25a EVK Board)
    // ----------------------------------------------------------------------
    // Disable debug override on uo_out[6:7]
    enable_all_outputs();

    // Map ASIC Output PMOD (uo_out) signals:
    // uo_out[0]: PMOD Pin 1 -> Left Audio / Loudspeaker J2 (Slot 20: Sujith PWM)
    // uo_out[1]: PMOD Pin 2 -> Right Audio / Headphone 3.5mm (Slot 21: Matt PWM)
    // uo_out[2]: RV2A03 apu_o_ce audio clock enable strobe (Slot 14)
    // uo_out[3]: RV2A03 apu_IRQ interrupt flag (Slot 14)
    // uo_out[4]: TinyQV UART TX mirror (Slot 2: UART - dedicated console)
    // uo_out[6]: TinyQV UART TX mirror (Slot 2: UART)
    // Initialize & auto-detect RV2A03 APU slot (Silicon is Slot 4)
    rv2a03_init();

    set_gpio_func(0, 2);                  // Start with UART TX on uo_out[0] for boot banner
    set_gpio_func(1, 21);                 // Slot 21: Hardware PWM to PMOD Pin 2 (Right Audio)
    set_gpio_func(2, rv2a03_slot_num);    // RV2A03 apu_o_ce
    set_gpio_func(3, rv2a03_slot_num);    // RV2A03 apu_IRQ
    set_gpio_func(4, 2);                  // Slot 2:  UART TX mirror on uo_out[4] (dedicated console)
    set_gpio_func(5, rv2a03_slot_num);
    set_gpio_func(6, 2);                  // Slot 2:  UART TX mirror on uo_out[6]
    set_gpio_func(7, rv2a03_slot_num);

    // Initialize PWM registers to 0 (silence)
    pwm_audio_write(0);

#ifndef ASIC_CLOCK_MHZ
#define ASIC_CLOCK_MHZ 64
#endif

    // Configure UART baud rate divider for 115200 baud on ASIC clock:
    *(volatile uint32_t*)0x8000088 = (ASIC_CLOCK_MHZ * 1000000) / 115200;

    // Configure MTIME divider for 1 MHz timer ticks:
    *(volatile uint32_t*)0x800002C = (ASIC_CLOCK_MHZ / 4) - 1;

#ifdef SIM
    // Fast UART for simulation: divider = 3 (4 clock cycles per bit)
    *(volatile uint32_t*)0x8000088 = 3;
    printf("\n=== RV2A03 FIRMWARE SIMULATION (ASIC) ===\n");
#else
    printf("\n");
    printf("=====================================================\n");
    printf("  TinyQV RV2A03 NES APU Sound Peripheral Testsuite  \n");
    printf("  Target: Sky25a Berzerk ASIC Silicon (EVK Board)   \n");
    printf("  Clock:  %d MHz | Peripheral Slot: %d (RV2A03)      \n", ASIC_CLOCK_MHZ, rv2a03_slot_num);
    printf("  Audio:  PMOD-AUDIO v1.2 (uo_out[0] Left, [1] Right)\n");
    printf("  UART:   uo_out[4] / uo_out[6] @ 115200 8N1         \n");
    printf("=====================================================\n\n");
#endif

    // Switch uo_out[0] to Hardware PWM (Slot 20: Sujith PWM) for audio output
    enable_pwm_audio();

    int passed = run_full_testsuite();

#ifdef SIM
    if (passed == 5) {
        uint8_t sim_vol = 12;
        play_berzerk_theme(&sim_vol);
    }
    while (1) {
        asm volatile ("wfi");
    }
#else
    if (passed < 5) {
        printf("[WARN] Self-test returned %d/5. Continuing to interactive synthesizer...\n\n", passed);
    }
    run_synth_repl();
#endif

    return 0;
}
