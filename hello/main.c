#include <csr.h>
#include <uart.h>
#define printf uart_printf
#include <mul.h>

int a = 3;
volatile char tx_char;
int main() {
    while(1){
#ifdef FULL_PRINTF
        printf("Hello, world!\n");
        printf("Hello %d\n", a);
        a = mul32x16(12, a);
        printf("Hello %d\n", a);
        printf("Cycles %d, time %d, instret %d\n", read_cycle(), read_time(), read_instret());
#else
        tx_char = 'A';
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
        uart_putc(tx_char);
#endif
    }
  return 0;
}
