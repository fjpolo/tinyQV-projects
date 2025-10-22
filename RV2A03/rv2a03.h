#ifndef _RV2A03_H_
#define _RV2A03_H_

#include <stdint.h>

// Helper macros for defining volatile memory-mapped pointers
// The 'volatile' keyword prevents the compiler from optimizing away memory accesses
#define VOLATILE_8BIT_PTR(addr) ((volatile uint8_t *)(addr))
#define VOLATILE_16BIT_PTR(addr) ((volatile uint16_t *)(addr))
#define VOLATILE_32BIT_PTR(addr) ((volatile uint32_t *)(addr))

#define RV2A03_REGISTER_START_ADDRESS 0x8000200

// --- NES APU Registers (0x8000200 to 0x800021F) ---

// Pulse Wave 1 Channel (Square 1)
#define APU_PULSE1_CONTROL 0x8000200 // $4000
#define APU_PULSE1_SWEEP 0x8000201 // $4001
#define APU_PULSE1_TIMER_LOW 0x8000202 // $4002
#define APU_PULSE1_TIMER_HIGH_AND_LENGTH 0x8000203 // $4003

// Pulse Wave 2 Channel (Square 2)
#define APU_PULSE2_CONTROL 0x8000204 // $4004
#define APU_PULSE2_SWEEP 0x8000205 // $4005
#define APU_PULSE2_TIMER_LOW 0x8000206 // $4006
#define APU_PULSE2_TIMER_HIGH_AND_LENGTH 0x8000207 // $4007

// Triangle Wave Channel
#define APU_TRIANGLE_LINEAR_COUNTER 0x8000208 // $4008
#define APU_TRIANGLE_UNUSED_4009 0x8000209 // $4009
#define APU_TRIANGLE_TIMER_LOW 0x800020A // $400A
#define APU_TRIANGLE_TIMER_HIGH_AND_LENGTH 0x800020B // $400B

// Noise Channel
#define APU_NOISE_CONTROL 0x800020C // $400C
#define APU_NOISE_UNUSED_400D 0x800020D // $400D
#define APU_NOISE_PERIOD 0x800020E // $400E
#define APU_NOISE_LENGTH_COUNTER 0x800020F // $400F

// Delta Modulator Channel (DMC)
#define APU_DMC_FREQUENCY_AND_FLAGS 0x8000210 // $4010
#define APU_DMC_DIRECT_LOAD 0x8000211 // $4011
#define APU_DMC_SAMPLE_ADDRESS 0x8000212 // $4012
#define APU_DMC_SAMPLE_LENGTH 0x8000213 // $4013

// APU/PPU/Joypad Registers
#define PPU_OAM_DMA_TRANSFER 0x8000214 // $4014
#define APU_CHANNEL_ENABLE_AND_STATUS 0x8000215 // $4015
#define JOYPAD1_DATA 0x8000216 // $4016
#define JOYPAD2_AND_FRAME_COUNTER 0x8000217 // $4017

// Unused/Expansion Registers (up to 0x800021F)
#define APU_EXPANSION_START 0x8000218 // $4018
#define APU_EXPANSION_END 0x800021F // $401F

// --- Non-APU Registers (Above 0x800021F) ---
#define RV2A03_REGISTER_CONFIGURATION0 0x8000220
#define RV2A03_REGISTER_STATUS0 0x8000222
#define RV2A03_REGISTER_DATA_INPUT 0x8000223
#define RV2A03_REGISTER_DATA_OUTPUT_MSB 0x8000224
#define RV2A03_REGISTER_DATA_OUTPUT_LSB 0x8000225

// -------------------------------------------------------------------------
// POINTER DEFINITIONS
// Use these definitions to easily read/write to the hardware registers:
// e.g., *APU_PULSE1_CONTROL_PTR = 0x1F;
// -------------------------------------------------------------------------

// --- NES APU Register Pointers (All 8-bit access) ---

// Pulse Wave 1 Channel (Square 1)
#define APU_PULSE1_CONTROL_PTR VOLATILE_8BIT_PTR(APU_PULSE1_CONTROL)
#define APU_PULSE1_SWEEP_PTR VOLATILE_8BIT_PTR(APU_PULSE1_SWEEP)
#define APU_PULSE1_TIMER_LOW_PTR VOLATILE_8BIT_PTR(APU_PULSE1_TIMER_LOW)
#define APU_PULSE1_TIMER_HIGH_AND_LENGTH_PTR VOLATILE_8BIT_PTR(APU_PULSE1_TIMER_HIGH_AND_LENGTH)

// Pulse Wave 2 Channel (Square 2)
#define APU_PULSE2_CONTROL_PTR VOLATILE_8BIT_PTR(APU_PULSE2_CONTROL)
#define APU_PULSE2_SWEEP_PTR VOLATILE_8BIT_PTR(APU_PULSE2_SWEEP)
#define APU_PULSE2_TIMER_LOW_PTR VOLATILE_8BIT_PTR(APU_PULSE2_TIMER_LOW)
#define APU_PULSE2_TIMER_HIGH_AND_LENGTH_PTR VOLATILE_8BIT_PTR(APU_PULSE2_TIMER_HIGH_AND_LENGTH)

// Triangle Wave Channel
#define APU_TRIANGLE_LINEAR_COUNTER_PTR VOLATILE_8BIT_PTR(APU_TRIANGLE_LINEAR_COUNTER)
#define APU_TRIANGLE_UNUSED_4009_PTR VOLATILE_8BIT_PTR(APU_TRIANGLE_UNUSED_4009)
#define APU_TRIANGLE_TIMER_LOW_PTR VOLATILE_8BIT_PTR(APU_TRIANGLE_TIMER_LOW)
#define APU_TRIANGLE_TIMER_HIGH_AND_LENGTH_PTR VOLATILE_8BIT_PTR(APU_TRIANGLE_TIMER_HIGH_AND_LENGTH)

// Noise Channel
#define APU_NOISE_CONTROL_PTR VOLATILE_8BIT_PTR(APU_NOISE_CONTROL)
#define APU_NOISE_UNUSED_400D_PTR VOLATILE_8BIT_PTR(APU_NOISE_UNUSED_400D)
#define APU_NOISE_PERIOD_PTR VOLATILE_8BIT_PTR(APU_NOISE_PERIOD)
#define APU_NOISE_LENGTH_COUNTER_PTR VOLATILE_8BIT_PTR(APU_NOISE_LENGTH_COUNTER)

// Delta Modulator Channel (DMC)
#define APU_DMC_FREQUENCY_AND_FLAGS_PTR VOLATILE_8BIT_PTR(APU_DMC_FREQUENCY_AND_FLAGS)
#define APU_DMC_DIRECT_LOAD_PTR VOLATILE_8BIT_PTR(APU_DMC_DIRECT_LOAD)
#define APU_DMC_SAMPLE_ADDRESS_PTR VOLATILE_8BIT_PTR(APU_DMC_SAMPLE_ADDRESS)
#define APU_DMC_SAMPLE_LENGTH_PTR VOLATILE_8BIT_PTR(APU_DMC_SAMPLE_LENGTH)

// APU/PPU/Joypad Registers
#define PPU_OAM_DMA_TRANSFER_PTR VOLATILE_8BIT_PTR(PPU_OAM_DMA_TRANSFER)
#define APU_CHANNEL_ENABLE_AND_STATUS_PTR VOLATILE_8BIT_PTR(APU_CHANNEL_ENABLE_AND_STATUS)
#define JOYPAD1_DATA_PTR VOLATILE_8BIT_PTR(JOYPAD1_DATA)
#define JOYPAD2_AND_FRAME_COUNTER_PTR VOLATILE_8BIT_PTR(JOYPAD2_AND_FRAME_COUNTER)

// Unused/Expansion Registers (up to 0x800021F)
#define APU_EXPANSION_START_PTR VOLATILE_8BIT_PTR(APU_EXPANSION_START)

// --- Non-APU Register Pointers (Types based on alignment/likely usage) ---

// Assuming 32-bit access for configuration registers
#define RV2A03_REGISTER_CONFIGURATION0_PTR VOLATILE_32BIT_PTR(RV2A03_REGISTER_CONFIGURATION0)

// Assuming 16-bit access for status register (due to 0x222 address)
#define RV2A03_REGISTER_STATUS0_PTR VOLATILE_16BIT_PTR(RV2A03_REGISTER_STATUS0)

// Assuming 8-bit access
#define RV2A03_REGISTER_DATA_INPUT_PTR VOLATILE_8BIT_PTR(RV2A03_REGISTER_DATA_INPUT)

// Assuming 32-bit access for the primary output register (on 32-bit boundary)
#define RV2A03_REGISTER_DATA_OUTPUT_MSB_PTR VOLATILE_32BIT_PTR(RV2A03_REGISTER_DATA_OUTPUT_MSB)

// Assuming 8-bit access for LSB (or byte-specific access)
#define RV2A03_REGISTER_DATA_OUTPUT_LSB_PTR VOLATILE_8BIT_PTR(RV2A03_REGISTER_DATA_OUTPUT_LSB)

// -------------------------------------------------------------------------
// INLINE REGISTER ACCESS FUNCTIONS
// These functions provide a clean API for reading/writing hardware registers.
// -------------------------------------------------------------------------

// --- NES APU Registers (0x8000200 to 0x800021F) ---

// Pulse Wave 1 Channel (Square 1) - Write Only
static inline void apu_pulse1_control_write(uint8_t value);  //  *APU_PULSE1_CONTROL_PTR = value; }
static inline void apu_pulse1_sweep_write(uint8_t value);  //  *APU_PULSE1_SWEEP_PTR = value; }
static inline void apu_pulse1_timer_low_write(uint8_t value);  //  *APU_PULSE1_TIMER_LOW_PTR = value; }
static inline void apu_pulse1_timer_high_and_length_write(uint8_t value);  //  *APU_PULSE1_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Pulse Wave 2 Channel (Square 2) - Write Only
static inline void apu_pulse2_control_write(uint8_t value);  //  *APU_PULSE2_CONTROL_PTR = value; }
static inline void apu_pulse2_sweep_write(uint8_t value);  //  *APU_PULSE2_SWEEP_PTR = value; }
static inline void apu_pulse2_timer_low_write(uint8_t value);  //  *APU_PULSE2_TIMER_LOW_PTR = value; }
static inline void apu_pulse2_timer_high_and_length_write(uint8_t value);  //  *APU_PULSE2_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Triangle Wave Channel - Write Only
static inline void apu_triangle_linear_counter_write(uint8_t value);  //  *APU_TRIANGLE_LINEAR_COUNTER_PTR = value; }
static inline void apu_triangle_unused_4009_write(uint8_t value);  //  *APU_TRIANGLE_UNUSED_4009_PTR = value; }
static inline void apu_triangle_timer_low_write(uint8_t value);  //  *APU_TRIANGLE_TIMER_LOW_PTR = value; }
static inline void apu_triangle_timer_high_and_length_write(uint8_t value);  //  *APU_TRIANGLE_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Noise Channel - Write Only
static inline void apu_noise_control_write(uint8_t value);  //  *APU_NOISE_CONTROL_PTR = value; }
static inline void apu_noise_unused_400d_write(uint8_t value);  //  *APU_NOISE_UNUSED_400D_PTR = value; }
static inline void apu_noise_period_write(uint8_t value);  //  *APU_NOISE_PERIOD_PTR = value; }
static inline void apu_noise_length_counter_write(uint8_t value);  //  *APU_NOISE_LENGTH_COUNTER_PTR = value; }

// Delta Modulator Channel (DMC) - Write Only
static inline void apu_dmc_frequency_and_flags_write(uint8_t value);  //  *APU_DMC_FREQUENCY_AND_FLAGS_PTR = value; }
static inline void apu_dmc_direct_load_write(uint8_t value);  //  *APU_DMC_DIRECT_LOAD_PTR = value; }
static inline void apu_dmc_sample_address_write(uint8_t value);  //  *APU_DMC_SAMPLE_ADDRESS_PTR = value; }
static inline void apu_dmc_sample_length_write(uint8_t value);  //  *APU_DMC_SAMPLE_LENGTH_PTR = value; }

// APU/PPU/Joypad Registers - R/W or Mixed Access
static inline void ppu_oam_dma_transfer_write(uint8_t value);  //  *PPU_OAM_DMA_TRANSFER_PTR = value; }

// 0x215: APU Status (R) / APU Enable (W)
static inline uint8_t apu_channel_status_read(void);  //  return *APU_CHANNEL_ENABLE_AND_STATUS_PTR; }
static inline void apu_channel_enable_write(uint8_t value);  //  *APU_CHANNEL_ENABLE_AND_STATUS_PTR = value; }

// 0x216: Joypad 1 Strobe (W) / Joypad 1 Data (R)
static inline uint8_t joypad1_data_read(void);  //  return *JOYPAD1_DATA_PTR; }
static inline void joypad1_strobe_write(uint8_t value);  //  *JOYPAD1_DATA_PTR = value; }

// 0x217: Frame Counter Control (W) / Joypad 2 Data (R)
static inline uint8_t joypad2_data_read(void);  //  return *JOYPAD2_AND_FRAME_COUNTER_PTR; }
static inline void apu_frame_counter_write(uint8_t value);  //  *JOYPAD2_AND_FRAME_COUNTER_PTR = value; }

// Unused/Expansion Registers (up to 0x800021F) - Assuming R/W for generic expansion
static inline void apu_expansion_start_write(uint8_t value);  //  *APU_EXPANSION_START_PTR = value; }
static inline uint8_t apu_expansion_start_read(void);  //  return *APU_EXPANSION_START_PTR; }

// --- Non-APU Register Functions (Above 0x800021F) ---

// RV2A03_REGISTER_CONFIGURATION0 (32-bit R/W)
static inline void rv2a03_configuration0_write(uint32_t value);  //  *RV2A03_REGISTER_CONFIGURATION0_PTR = value; }
static inline uint32_t rv2a03_configuration0_read(void);  //  return *RV2A03_REGISTER_CONFIGURATION0_PTR; }

// RV2A03_REGISTER_STATUS0 (16-bit R/W)
static inline void rv2a03_status0_write(uint16_t value);  //  *RV2A03_REGISTER_STATUS0_PTR = value; }
static inline uint16_t rv2a03_status0_read(void);  //  return *RV2A03_REGISTER_STATUS0_PTR; }

// RV2A03_REGISTER_DATA_INPUT (8-bit Write-Only)
static inline void rv2a03_data_input_write(uint8_t value);  //  *RV2A03_REGISTER_DATA_INPUT_PTR = value; }

// RV2A03_REGISTER_DATA_OUTPUT_MSB (32-bit R/W)
static inline void rv2a03_data_output_msb_write(uint32_t value);  //  *RV2A03_REGISTER_DATA_OUTPUT_MSB_PTR = value; }
static inline uint32_t rv2a03_data_output_msb_read(void);  //  return *RV2A03_REGISTER_DATA_OUTPUT_MSB_PTR; }

// RV2A03_REGISTER_DATA_OUTPUT_LSB (8-bit R/W)
static inline void rv2a03_data_output_lsb_write(uint8_t value);  //  *RV2A03_REGISTER_DATA_OUTPUT_LSB_PTR = value; }
static inline uint8_t rv2a03_data_output_lsb_read(void);  //  return *RV2A03_REGISTER_DATA_OUTPUT_LSB_PTR; }

#endif // _RV2A03_H_
