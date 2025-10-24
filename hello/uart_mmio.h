#ifndef __UART_MMIO_H
#define __UART_MMIO_H

// --- UART Register Definitions (Assuming a simple RISC-V/LiteX-style UART) ---
// Based on the previous trace, we assume the UART is mapped starting near 0x80000080.
#define UART_BASE 0x80000080

// Offsets for the registers
#define UART_RXTX_OFFSET    0x00 // Data Register (Read for RX, Write for TX)
#define UART_STATUS_OFFSET  0x04 // Status Register

// Macro for memory-mapped I/O (MMIO) read and write
// These macros use volatile to prevent the compiler from optimizing out the memory access.
#define MMIO_READ(addr) (*(volatile unsigned int *)(addr))
#define MMIO_WRITE(addr, data) (*(volatile unsigned int *)(addr) = (data))

// Status register bit definitions
#define UART_TX_FULL (1 << 1) // Example: Status register bit 1 is TX full/busy

// Function to check if the transmitter is ready to accept a new character
static inline int uart_tx_ready() {
    // Poll the status register. If the TX_FULL bit is clear, it is ready.
    return !(MMIO_READ(UART_BASE + UART_STATUS_OFFSET) & UART_TX_FULL);
}

// Function to put a character, polling until the TX buffer is ready
static inline void uart_putc_mmio(char c) {
    // 1. Wait until the TX buffer is not full (ready to receive data)
    while (!uart_tx_ready());

    // 2. Write the character directly to the data register.
    // THIS ACTION MUST GENERATE THE STORE INSTRUCTION (SW/SH/SB).
    MMIO_WRITE(UART_BASE + UART_RXTX_OFFSET, c);
}

#endif // __UART_MMIO_H
