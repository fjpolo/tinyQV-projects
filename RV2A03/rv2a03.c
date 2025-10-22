#include "rv2a03.h"
#include <stdint.h> // Include standard integer types just in case the header doesn't fully cover it

// -------------------------------------------------------------------------
// PERIPHERAL CONTROL FUNCTIONS (NON-APU REGISTERS)
// Prototypes for these functions should be added to rv2a03.h
// -------------------------------------------------------------------------

/**
 * @brief Initializes the RV2A03 non-APU peripheral registers (0x8000220 to 0x8000225)
 * * Writes 0 to the R/W control/status registers to ensure a default, disabled state 
 * upon system startup.
 */
void rv2a03_peripheral_init(void)
{
    // Write 0 to the 32-bit Configuration Register (RV2A03_REGISTER_CONFIGURATION0)
    // b0 = CE
    rv2a03_configuration0_write(0x00000001); 

    // Write 0 to the 16-bit Status Register (RV2A03_REGISTER_STATUS0)
    // rv2a03_status0_write(0x0000);
    
    // Data Input/Output registers are only used for data transfer and do not require initialization.
}

/**
 * @brief De-initializes (disables) the RV2A03 non-APU peripheral registers.
 *
 * Sets the configuration and status registers back to a default (disabled/zero) state.
 */
void rv2a03_peripheral_deinit(void)
{
    // Disable the peripheral by writing 0 to the Configuration Register
    rv2a03_configuration0_write(0x00000000); 

    // Clear the Status Register
    rv2a03_status0_write(0x0000);
}

/**
 * @brief Read Congifuration0 register
 *
 */
int8_t rv2a03_peripheral_read_configuration0(void){
    return *(volatile int8_t *)RV2A03_REGISTER_CONFIGURATION0_PTR;
}
