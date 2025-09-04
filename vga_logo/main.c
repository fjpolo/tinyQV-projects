#include <stddef.h>
#include <gpio.h>
#include <stdio.h>
#include <csr.h>
#include <mul.h>
#include <uart.h>
#include <string.h>

extern void draw_logo(int angle);

// VGA gfx base addess is 0x800_0100 (0x800_0000 + 0x40*4)
volatile uint32_t* const vga_line = (volatile uint32_t*)0x8000100;

volatile uint8_t* const vga_intr = (volatile uint8_t*)0x8000101;
volatile uint8_t* const vga_ylow = (volatile uint8_t*)0x8000102;
volatile uint8_t* const vga_yhigh = (volatile uint8_t*)0x8000103;
volatile uint16_t* const vga_yboth = (volatile uint16_t*)0x8000102;

volatile uint8_t* const vga_colour1 = (volatile uint8_t*)0x8000105;
volatile uint8_t* const vga_colour2 = (volatile uint8_t*)0x8000106;
volatile uint8_t* const vga_colour3 = (volatile uint8_t*)0x8000107;

uint32_t framebuffer1[16*192];
uint32_t framebuffer2[16*192];
uint32_t* front_buffer = framebuffer1;
uint32_t* back_buffer = framebuffer2;

#if 0
void tqv_user_interrupt04(void) {
  int y = mul32x16(*vga_yhigh, 48);
  y += *vga_ylow;
  if (++y >= 768) y = 0;
  y &= 0x3fc;

  (void)*vga_intr;  // Clear interrupt
  uint32_t* buffer = &front_buffer[y << 2];
  for (int i = 0; i < 16; ++i) vga_line[i] = buffer[i];
}
#endif

void flip() {
  while (*vga_yhigh != 16);
  uint32_t* tmp = front_buffer;
  front_buffer = back_buffer;
  back_buffer = tmp;
}

int main(void) {
  // Set line
  for (int i = 0; i < 16; ++i) vga_line[i] = 0x39393939;

  // Set interrupt cfg
  *vga_intr = 0x3c;

  // Set colours
  *vga_colour1 = 0x38;
  *vga_colour2 = 0x24;
  *vga_colour3 = 0x03;

  // Set all outputs to regular mode (not debug)
  set_debug_sel(0xff);

  // Set all outputs to peripheral 4
  for (int i = 0; i < 8; ++i) set_gpio_func(i, 4);

  const uint32_t line[] = {0x39393939, 0x4e4e4e4e, 0x93939393, 0xe4e4e4e4};
  for (int i = 0; i < 16*192; ++i) framebuffer1[i] = line[(i>>4) & 3];

  // Draw the logo
  draw_logo(0);
  flip();

  // Enable interrupt
  enable_interrupt(4);

  int angle = 0;
  while (1) {
    memset(back_buffer, 0, 192*64);
    angle += 5;
    if (angle >= 360) angle -= 360;
    draw_logo(angle);
    flip();
  }

  return 0;
}
