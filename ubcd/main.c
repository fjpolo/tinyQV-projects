#include <csr.h>
#include <gpio.h>
#include <uart.h>
#define printf uart_printf
#include <timer.h>
#include <stdbool.h>

#define UBCD_PERI 17
#define UBCD_BASE (0x8000400 + 0x10 * UBCD_PERI)
#define UBCD_REG(x) (volatile uint8_t*)(UBCD_BASE + x)

volatile uint8_t* ubcd_data = UBCD_REG(0);
volatile uint8_t* ubcd_dcr = UBCD_REG(1);
volatile uint8_t* ubcd_vcr = UBCD_REG(2);
volatile uint8_t* ubcd_pcr = UBCD_REG(3);

void timer_callback(void*) {
    static bool units = true;

    // Run every ~5ms
    set_alarm(5, timer_callback, NULL);
    *ubcd_dcr = 0xc0;               // Display off
    set_outputs(units ? 0x80 : 0);  // Select digit
    *ubcd_pcr = units ? 0 : 1;      // Units or Tens
    *ubcd_dcr = 0xe0;               // Display on
    units = !units;
}

inline void display_num(int n) {
    uint8_t val = n % 10 + ((n / 10) << 4);
    *ubcd_data = val;
}

int main() {
    // Set outputs for UBCD peripheral, 7 seg PMOD
    enable_all_outputs();
    for (int i = 0; i < 7; ++i) set_gpio_func(i, UBCD_PERI);
    *ubcd_dcr = 0xc0;
    *ubcd_vcr = 0;
    *ubcd_pcr = 0;
    set_outputs(0x80);

    // Start the display
    set_alarm(1, timer_callback, NULL);

    while (1) {
        for (int i = 0; i < 100; ++i) {
            display_num(i);
            delay_us(100000);
        }
        for (int i = 99; i >= 0; --i) {
            display_num(i);
            delay_us(100000);
        }
    }
    return 0;
}
