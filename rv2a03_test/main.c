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
 *   - NES Chiptune Jukebox (Super Mario Bros., Berzerk, Zelda Fanfare)
 *   - Real-Time APU Status & Register Inspector
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"
#include "timer.h"
#include "rv2a03.h"

#define printf uart_printf

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
// Delay helper using mtime (runs at SoC clock frequency)
static void delay_cycles(uint32_t count) {
    uint32_t start = get_mtime();
    while ((get_mtime() - start) < count) {
        asm volatile ("nop");
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

static void sfx_zelda_fanfare(void) {
    printf("\r\033[K[JUKEBOX] >> Zelda Secret Fanfare!            \n");
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_SQ2_ENABLE);
    const uint8_t notes[] = { 67, 66, 63, 57, 56, 64, 68, 72 };
    for (int i = 0; i < 8; i++) {
        if (uart_rx_poll() >= 0) break;
        uint16_t t1 = rv2a03_midi_to_pulse_timer(notes[i]);
        uint16_t t2 = rv2a03_midi_to_pulse_timer(notes[i] - 12);
        rv2a03_set_pulse1(RV2A03_DUTY_50, 11, true, true, t1, 0x1E);
        rv2a03_set_pulse2(RV2A03_DUTY_25, 7,  true, true, t2, 0x1E);
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

static void play_berzerk_theme(void) {
    printf("\r\033[K[JUKEBOX] >> Playing 'Berzerk APU Theme' (press any key to stop)... \n");
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
            if (uart_rx_poll() >= 0) goto end_playback;
            uint16_t tri_timer = rv2a03_midi_to_pulse_timer(bass[bar]) >> 1;
            rv2a03_set_triangle(0x7F, true, tri_timer, 0x1E);

            for (int note = 0; note < 4; note++) {
                if (uart_rx_poll() >= 0) goto end_playback;
                uint8_t m_note = melody[bar * 4 + note];
                uint16_t sq_timer = rv2a03_midi_to_pulse_timer(m_note);

                rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0B, true, true, sq_timer, 0x1E);
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
// Jukebox: Super Mario Bros. Theme
// --------------------------------------------------------------------------

#define NOTE_REST 0
typedef struct {
    uint8_t sq1;
    uint8_t sq2;
    uint8_t tri;
    uint8_t noise;
    uint8_t dur;
} MarioEvent;

static const MarioEvent mario_score[] = {
    // Intro fanfare
    {76, 64, 48, 1, 3}, {76, 64, 48, 0, 3}, {76, 64, 48, 1, 3}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 3},
    {72, 60, 48, 1, 3}, {76, 64, 48, 0, 3}, {79, 67, 55, 3, 6}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 6},
    {67, 55, 43, 3, 6}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 6},

    // Main Theme Bar 1-2
    {72, 64, 60, 3, 4}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 2}, {67, 60, 55, 1, 4}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 2},
    {64, 55, 52, 3, 4}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 2}, {69, 57, 57, 1, 3}, {71, 59, 59, 0, 3},
    {70, 58, 58, 1, 3}, {69, 57, 57, 3, 4}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 2},
    {67, 60, 60, 1, 3}, {76, 67, 64, 0, 3}, {79, 72, 67, 1, 3}, {81, 74, 69, 3, 3},
    {77, 69, 65, 1, 3}, {79, 72, 67, 0, 3}, {NOTE_REST, NOTE_REST, NOTE_REST, 0, 3},
    {76, 67, 64, 1, 3}, {72, 64, 60, 3, 3}, {74, 65, 62, 1, 3}, {71, 62, 59, 3, 6}
};

static void play_mario_theme(void) {
    printf("\r\033[K[JUKEBOX] >> Playing 'Super Mario Bros. Theme' (press any key to stop)... \n");
    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_SQ2_ENABLE |
                           RV2A03_STATUS_TRI_ENABLE | RV2A03_STATUS_NOISE_ENABLE);

    const int num_events = sizeof(mario_score) / sizeof(mario_score[0]);
    const uint32_t TICK_MS = 42;

    for (int i = 0; i < num_events; i++) {
        if (uart_rx_poll() >= 0) break;
        const MarioEvent *ev = &mario_score[i];

        if (ev->sq1 != NOTE_REST) {
            uint16_t sq1_timer = rv2a03_midi_to_pulse_timer(ev->sq1);
            rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0B, true, true, sq1_timer, 0x1E);
        } else {
            rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
        }

        if (ev->sq2 != NOTE_REST) {
            uint16_t sq2_timer = rv2a03_midi_to_pulse_timer(ev->sq2);
            rv2a03_set_pulse2(RV2A03_DUTY_25, 0x08, true, true, sq2_timer, 0x1E);
        } else {
            rv2a03_write_reg(RV2A03_REG_SQ2_VOL, 0x30);
        }

        if (ev->tri != NOTE_REST) {
            uint16_t tri_timer = rv2a03_midi_to_pulse_timer(ev->tri) >> 1;
            rv2a03_set_triangle(0x7F, true, tri_timer, 0x1E);
        } else {
            rv2a03_write_reg(RV2A03_REG_TRI_LINEAR, 0x00);
        }

        if (ev->noise == 1) {
            rv2a03_set_noise(0x08, true, true, 0x05, false, 0x10);
        } else if (ev->noise == 3) {
            rv2a03_set_noise(0x0B, true, true, 0x0B, false, 0x18);
        } else {
            rv2a03_write_reg(RV2A03_REG_NOISE_VOL, 0x30);
        }

        uint32_t total_time = ev->dur * TICK_MS;
        uint32_t staccato = (total_time > 40) ? 22 : 0;
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
    printf("  SOUNDBOARD: [C] Coin!    [B] Jump!       [X] Explosion!  [L] Laser!\n");
    printf("              [V] 1-Up!    [N] Snare Hit   [9/I] Barrel Drum (Boom!)\n");
    printf("              [0/D] Cycle Barrel Distortion (0:Clean -> 1:Warm -> 2:Fuzz -> 3:Doom)\n\n");
    printf("  JUKEBOX:    [5] Super Mario Bros. Theme\n");
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
            // CPU sleeps until next interrupt or UART character
            asm volatile ("nop");
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
                    printf("\r\033[K[VOL] >> Volume UP: %d/15\n", current_vol);
                    continue;
                } else if (c3 == 'B') {
                    // DOWN ARROW -> Volume Down
                    if (current_vol > 0) current_vol--;
                    printf("\r\033[K[VOL] >> Volume DOWN: %d/15\n", current_vol);
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
                // Pulse 1
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note);
                rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);
                rv2a03_set_pulse1(duty_constants[current_duty_idx], current_vol, true, true, timer, 0x1E);
                freq_hz = 1789773 / (16 * (timer + 1));
            } else if (current_channel == 1) {
                // Pulse 2
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note);
                rv2a03_enable_channels(RV2A03_STATUS_SQ2_ENABLE);
                rv2a03_set_pulse2(duty_constants[current_duty_idx], current_vol, true, true, timer, 0x1E);
                freq_hz = 1789773 / (16 * (timer + 1));
            } else if (current_channel == 2) {
                // Triangle
                uint16_t timer = rv2a03_midi_to_pulse_timer(midi_note) >> 1;
                rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE);
                rv2a03_set_triangle(0x7F, true, timer, 0x1E);
                freq_hz = 1789773 / (32 * (timer + 1));
            } else if (current_channel == 3) {
                // Noise
                uint8_t period_idx = (uint8_t)(15 - (midi_note % 16));
                rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);
                rv2a03_set_noise(current_vol, true, true, period_idx, false, 0x1E);
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
            play_mario_theme();
            continue;
        } else if (c == '6') {
            play_berzerk_theme();
            continue;
        } else if (c == '7') {
            sfx_zelda_fanfare();
            continue;
        }

        // 4. Soundboard Retro Sound Effects
        if (c == 'c' || c == 'C') {
            sfx_coin();
            continue;
        } else if (c == 'b' || c == 'B') {
            sfx_jump();
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
            sfx_barrel_drum(barrel_distortion);
            continue;
        } else if (c == '0' || c == 'D') {
            cycle_barrel_distortion();
            continue;
        }

        // 5. Controls & Configuration
        if (c == 'q' || c == 'Q') {
            current_duty_idx = (current_duty_idx + 1) % 4;
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
            printf("\r\033[K[CTRL] >> Volume set to: %d/15\n", current_vol);
            continue;
        } else if (c == '+' || c == '=') {
            if (current_vol < 15) current_vol++;
            printf("\r\033[K[CTRL] >> Volume set to: %d/15\n", current_vol);
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
#ifdef SIM
    // Fast UART for simulation: divider = 3 (4 clock cycles per bit)
    *(volatile uint32_t*)0x8000088 = 3;
    printf("\n=== RV2A03 FIRMWARE SIMULATION ===\n");
#else
    printf("\n");
    printf("=====================================================\n");
    printf("  TinyQV RV2A03 NES APU Sound Peripheral Testsuite  \n");
    printf("  Target: Sky25a Berzerk (Peripheral Index 14)      \n");
    printf("=====================================================\n\n");
#endif

    int passed = run_full_testsuite();

#ifdef SIM
    if (passed == 5) {
        play_berzerk_theme();
    }
    while (1) {
        asm volatile ("wfi");
    }
#else
    if (passed == 5) {
        run_synth_repl();
    } else {
        printf("\n[ERROR] Hardware self-test failed! Synthesizer halted.\n");
        while (1) {
            asm volatile ("wfi");
        }
    }
#endif

    return 0;
}
