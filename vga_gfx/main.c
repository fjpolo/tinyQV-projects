#include <stddef.h>
#include <gpio.h>
#include <stdio.h>
#include <csr.h>

// VGA gfx base addess is 0x800_0100 (0x800_0000 + 0x40*4)
volatile uint32_t* vga_line = (volatile uint32_t*)0x8000100;

volatile uint8_t* vga_intr = (volatile uint8_t*)0x8000101;
volatile uint8_t* vga_ylow = (volatile uint8_t*)0x8000102;
volatile uint8_t* vga_yhigh = (volatile uint8_t*)0x8000103;
volatile uint16_t* vga_yboth = (volatile uint16_t*)0x8000102;

volatile uint8_t* vga_colour1 = (volatile uint8_t*)0x8000105;
volatile uint8_t* vga_colour2 = (volatile uint8_t*)0x8000106;
volatile uint8_t* vga_colour3 = (volatile uint8_t*)0x8000107;

int count = 0;

void tqv_user_interrupt04(void) {
  const uint32_t line[] = {0x39393939, 0x4e4e4e4e, 0x93939393, 0xe4e4e4e4};
  if (*vga_yboth >= 0x0f2f) count = 0;

  (void)*vga_intr;  // Clear interrupt
  for (int i = 0; i < 16; ++i) vga_line[i] = line[count];

  if (++count == 4) count = 0;
}

int main(void) {
  // Set line
  for (int i = 0; i < 16; ++i) vga_line[i] = 0x39393939;

  // Set interrupt cfg
  *vga_intr = 0x1c;

  // Set colours
  *vga_colour1 = 0x30;
  *vga_colour2 = 0x0c;
  *vga_colour3 = 0x03;

  // Set all outputs to regular mode (not debug)
  set_debug_sel(0xff);

  // Set all outputs to peripheral 4
  for (int i = 0; i < 8; ++i) set_gpio_func(i, 4);

  // Enable interrupt
  enable_interrupt(4);

  // Loop forever - all further work will be done in the
  // interrupt.
  while(1);
  return 0;
}
