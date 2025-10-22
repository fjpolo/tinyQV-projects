#include "rv2a03.h"
#include <stdint.h> // Include standard integer types just in case the header doesn't fully cover it

// NOTE: These 'static inline' function definitions are now placed here in the C file.
// If you are using the C99 or later standard, and these functions are small, the compiler will
// likely inline them at the call site, which is optimal for hardware access.

// --- NES APU Registers (0x8000200 to 0x800021F) ---

// Pulse Wave 1 Channel (Square 1) - Write Only
static inline void apu_pulse1_control_write(uint8_t value) { *APU_PULSE1_CONTROL_PTR = value; }
static inline void apu_pulse1_sweep_write(uint8_t value) { *APU_PULSE1_SWEEP_PTR = value; }
static inline void apu_pulse1_timer_low_write(uint8_t value) { *APU_PULSE1_TIMER_LOW_PTR = value; }
static inline void apu_pulse1_timer_high_and_length_write(uint8_t value) { *APU_PULSE1_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Pulse Wave 2 Channel (Square 2) - Write Only
static inline void apu_pulse2_control_write(uint8_t value) { *APU_PULSE2_CONTROL_PTR = value; }
static inline void apu_pulse2_sweep_write(uint8_t value) { *APU_PULSE2_SWEEP_PTR = value; }
static inline void apu_pulse2_timer_low_write(uint8_t value) { *APU_PULSE2_TIMER_LOW_PTR = value; }
static inline void apu_pulse2_timer_high_and_length_write(uint8_t value) { *APU_PULSE2_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Triangle Wave Channel - Write Only
static inline void apu_triangle_linear_counter_write(uint8_t value) { *APU_TRIANGLE_LINEAR_COUNTER_PTR = value; }
static inline void apu_triangle_unused_4009_write(uint8_t value) { *APU_TRIANGLE_UNUSED_4009_PTR = value; }
static inline void apu_triangle_timer_low_write(uint8_t value) { *APU_TRIANGLE_TIMER_LOW_PTR = value; }
static inline void apu_triangle_timer_high_and_length_write(uint8_t value) { *APU_TRIANGLE_TIMER_HIGH_AND_LENGTH_PTR = value; }

// Noise Channel - Write Only
static inline void apu_noise_control_write(uint8_t value) { *APU_NOISE_CONTROL_PTR = value; }
static inline void apu_noise_unused_400d_write(uint8_t value) { *APU_NOISE_UNUSED_400D_PTR = value; }
static inline void apu_noise_period_write(uint8_t value) { *APU_NOISE_PERIOD_PTR = value; }
static inline void apu_noise_length_counter_write(uint8_t value) { *APU_NOISE_LENGTH_COUNTER_PTR = value; }

// Delta Modulator Channel (DMC) - Write Only
static inline void apu_dmc_frequency_and_flags_write(uint8_t value) { *APU_DMC_FREQUENCY_AND_FLAGS_PTR = value; }
static inline void apu_dmc_direct_load_write(uint8_t value) { *APU_DMC_DIRECT_LOAD_PTR = value; }
static inline void apu_dmc_sample_address_write(uint8_t value) { *APU_DMC_SAMPLE_ADDRESS_PTR = value; }
static inline void apu_dmc_sample_length_write(uint8_t value) { *APU_DMC_SAMPLE_LENGTH_PTR = value; }

// APU/PPU/Joypad Registers - R/W or Mixed Access
static inline void ppu_oam_dma_transfer_write(uint8_t value) { *PPU_OAM_DMA_TRANSFER_PTR = value; }

// 0x215: APU Status (R) / APU Enable (W)
static inline uint8_t apu_channel_status_read(void) { return *APU_CHANNEL_ENABLE_AND_STATUS_PTR; }
static inline void apu_channel_enable_write(uint8_t value) { *APU_CHANNEL_ENABLE_AND_STATUS_PTR = value; }

// 0x216: Joypad 1 Strobe (W) / Joypad 1 Data (R)
static inline uint8_t joypad1_data_read(void) { return *JOYPAD1_DATA_PTR; }
static inline void joypad1_strobe_write(uint8_t value) { *JOYPAD1_DATA_PTR = value; }

// 0x217: Frame Counter Control (W) / Joypad 2 Data (R)
static inline uint8_t joypad2_data_read(void) { return *JOYPAD2_AND_FRAME_COUNTER_PTR; }
static inline void apu_frame_counter_write(uint8_t value) { *JOYPAD2_AND_FRAME_COUNTER_PTR = value; }

// Unused/Expansion Registers (up to 0x800021F) - Assuming R/W for generic expansion
static inline void apu_expansion_start_write(uint8_t value) { *APU_EXPANSION_START_PTR = value; }
static inline uint8_t apu_expansion_start_read(void) { return *APU_EXPANSION_START_PTR; }

// --- Non-APU Register Functions (Above 0x800021F) ---

// RV2A03_REGISTER_CONFIGURATION0 (32-bit R/W)
static inline void rv2a03_configuration0_write(uint32_t value) { *RV2A03_REGISTER_CONFIGURATION0_PTR = value; }
static inline uint32_t rv2a03_configuration0_read(void) { return *RV2A03_REGISTER_CONFIGURATION0_PTR; }

// RV2A03_REGISTER_STATUS0 (16-bit R/W)
static inline void rv2a03_status0_write(uint16_t value) { *RV2A03_REGISTER_STATUS0_PTR = value; }
static inline uint16_t rv2a03_status0_read(void) { return *RV2A03_REGISTER_STATUS0_PTR; }

// RV2A03_REGISTER_DATA_INPUT (8-bit Write-Only)
static inline void rv2a03_data_input_write(uint8_t value) { *RV2A03_REGISTER_DATA_INPUT_PTR = value; }

// RV2A03_REGISTER_DATA_OUTPUT_MSB (32-bit R/W)
static inline void rv2a03_data_output_msb_write(uint32_t value) { *RV2A03_REGISTER_DATA_OUTPUT_MSB_PTR = value; }
static inline uint32_t rv2a03_data_output_msb_read(void) { return *RV2A03_REGISTER_DATA_OUTPUT_MSB_PTR; }

// RV2A03_REGISTER_DATA_OUTPUT_LSB (8-bit R/W)
static inline void rv2a03_data_output_lsb_write(uint8_t value) { *RV2A03_REGISTER_DATA_OUTPUT_LSB_PTR = value; }
static inline uint8_t rv2a03_data_output_lsb_read(void) { return *RV2A03_REGISTER_DATA_OUTPUT_LSB_PTR; }
