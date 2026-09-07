/*
 * SPDX-License-Identifier: Apache-2.0
 * RV2A03 NES APU Firmware Test Suite and Demo for TinyQV
 * Target: Tiny Tapeout Sky25a (Berzerk instance)
 *
 * Translates cocotb verification tests into self-checking RISC-V C code.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "uart.h"
#include "timer.h"
#include "rv2a03.h"

// Delay helper using mtime (runs at SoC clock frequency, ~64MHz)
static void delay_cycles(uint32_t count) {
    uint32_t start = get_mtime();
    while ((get_mtime() - start) < count) {
        asm volatile ("nop");
    }
}

static void delay_ms(uint32_t ms) {
    // Approx 64,000 cycles per millisecond at 64MHz
    delay_cycles(ms * 64000);
}

// --------------------------------------------------------------------------
// Cocotb Test Translations
// --------------------------------------------------------------------------

// 1. Equivalent to test_sq1_channel() in test.py
static bool test_sq1_channel(void) {
    printf("[TEST 1] Testing Square Channel 1 (440 Hz)... ");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE);

    // 440 Hz config matching cocotb: duty 50%, constant volume 15, timer 0x7E
    rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);

    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < 200; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }

    rv2a03_mute();

    if (non_zero > 10) {
        printf("PASS (%d/200 samples, peak: %d)\n", non_zero, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

// 2. Equivalent to test_sq2_channel() in test.py
static bool test_sq2_channel(void) {
    printf("[TEST 2] Testing Square Channel 2 (440 Hz)... ");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ2_ENABLE);

    // 440 Hz config: duty 50%, constant volume 15, timer 0x7E
    rv2a03_set_pulse2(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);

    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < 200; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }

    rv2a03_mute();

    if (non_zero > 10) {
        printf("PASS (%d/200 samples, peak: %d)\n", non_zero, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

// 3. Equivalent to test_tri_channel() in test.py
static bool test_tri_channel(void) {
    printf("[TEST 3] Testing Triangle Channel (440 Hz)... ");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_TRI_ENABLE);

    // 440 Hz config: linear reload 0x7F, halt true, timer 0x3E
    rv2a03_set_triangle(0x7F, true, 0x003E, 0x1E);

    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < 200; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }

    rv2a03_mute();

    if (non_zero > 10) {
        printf("PASS (%d/200 samples, peak: %d)\n", non_zero, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

// 4. Equivalent to test_noise_channel() in test.py
static bool test_noise_channel(void) {
    printf("[TEST 4] Testing Noise Channel.............. ");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_NOISE_ENABLE);

    // Noise config: volume 15, period index 0x0F
    rv2a03_set_noise(0x0F, true, true, 0x0F, false, 0x1E);

    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < 200; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }

    rv2a03_mute();

    if (non_zero > 10) {
        printf("PASS (%d/200 samples, peak: %d)\n", non_zero, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

// 5. Equivalent to test_all_channels_together() in test.py
static bool test_all_channels_together(void) {
    printf("[TEST 5] Testing All Channels Simultaneously. ");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_ALL_ENABLE);

    // Configure all channels concurrently
    rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0F, true, true, 0x007E, 0x1E);
    rv2a03_set_pulse2(RV2A03_DUTY_25, 0x0C, true, true, 0x00A0, 0x1E);
    rv2a03_set_triangle(0x7F, true, 0x003E, 0x1E);
    rv2a03_set_noise(0x08, true, true, 0x0C, false, 0x1E);

    delay_cycles(10000);

    int non_zero = 0;
    int16_t peak = 0;
    for (int i = 0; i < 200; i++) {
        int16_t s = rv2a03_read_sample();
        if (s != 0) non_zero++;
        if (s > peak) peak = s;
        else if (-s > peak) peak = -s;
        delay_cycles(20);
    }

    rv2a03_mute();

    if (non_zero > 10) {
        printf("PASS (%d/200 samples, combined peak: %d)\n", non_zero, peak);
        return true;
    } else {
        printf("FAIL (silence detected)\n");
        return false;
    }
}

// --------------------------------------------------------------------------
// Musical Playback Demo
// --------------------------------------------------------------------------

static void play_chiptune_demo(void) {
    printf("\nPlaying NES Chiptune Arpeggio Demo...\n");

    rv2a03_init();
    rv2a03_enable_channels(RV2A03_STATUS_SQ1_ENABLE | RV2A03_STATUS_TRI_ENABLE);

    // C-Major / A-minor arpeggio notes (MIDI numbers)
    const uint8_t melody[] = {
        RV2A03_NOTE_C4, RV2A03_NOTE_E4, RV2A03_NOTE_G4, RV2A03_NOTE_C5,
        RV2A03_NOTE_A4, RV2A03_NOTE_C5, RV2A03_NOTE_E5, RV2A03_NOTE_A5,
        RV2A03_NOTE_F4, RV2A03_NOTE_A4, RV2A03_NOTE_C5, RV2A03_NOTE_F5,
        RV2A03_NOTE_G4, RV2A03_NOTE_B4, RV2A03_NOTE_D5, RV2A03_NOTE_G5
    };

    const uint8_t bass[] = {
        RV2A03_NOTE_C3, RV2A03_NOTE_A3, RV2A03_NOTE_F3, RV2A03_NOTE_G3
    };

    for (int repeat = 0; repeat < 2; repeat++) {
        for (int bar = 0; bar < 4; bar++) {
            // Bass note on Triangle channel
            uint16_t tri_timer = rv2a03_midi_to_pulse_timer(bass[bar]) >> 1;
            rv2a03_set_triangle(0x7F, true, tri_timer, 0x1E);

            // 4 arpeggio notes on Square 1 channel
            for (int note = 0; note < 4; note++) {
                uint8_t m_note = melody[bar * 4 + note];
                uint16_t sq_timer = rv2a03_midi_to_pulse_timer(m_note);

                rv2a03_set_pulse1(RV2A03_DUTY_50, 0x0B, true, true, sq_timer, 0x1E);
                delay_ms(120);

                // Short staccato pause
                rv2a03_write_reg(RV2A03_REG_SQ1_VOL, 0x30);
                delay_ms(15);
            }
        }
    }

    rv2a03_mute();
    printf("Demo complete! APU muted.\n");
}

// --------------------------------------------------------------------------
// Main Entry Point
// --------------------------------------------------------------------------

int main(void) {
    printf("\n");
    printf("=====================================================\n");
    printf("  TinyQV RV2A03 NES APU Sound Peripheral Testsuite  \n");
    printf("  Target: Sky25a Berzerk (Peripheral Index 14)      \n");
    printf("=====================================================\n\n");

    int passed = 0;
    if (test_sq1_channel()) passed++;
    if (test_sq2_channel()) passed++;
    if (test_tri_channel()) passed++;
    if (test_noise_channel()) passed++;
    if (test_all_channels_together()) passed++;

    printf("\n-----------------------------------------------------\n");
    printf("Test Results: %d/5 tests passed successfully.\n", passed);
    printf("-----------------------------------------------------\n");

    if (passed == 5) {
        play_chiptune_demo();
    }

    while (1) {
        asm volatile ("wfi");
    }

    return 0;
}
