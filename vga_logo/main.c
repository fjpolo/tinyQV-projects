#include <stddef.h>
#include <gpio.h>
#include <stdio.h>
#include <csr.h>
#include <mul.h>
#include <uart.h>
#include <string.h>
#include <peripherals/vga_gfx.h>

extern void draw_logo(int angle);

int main(void) {
  vga_gfx_start();

  // Set colours
  vga_gfx_set_colour1(0x38);
  vga_gfx_set_colour2(0x24);
  vga_gfx_set_colour3(0x03);

  const uint32_t line[] = {0x39393939, 0x4e4e4e4e, 0x93939393, 0xe4e4e4e4};
  for (int i = 0; i < 16*192; ++i) vga_gfx_front_buffer[i] = line[(i>>4) & 3];

  // Draw the logo
  draw_logo(0);
  vga_gfx_flip();

  int angle = 0;
  while (1) {
    memset(vga_gfx_back_buffer, 0, 192*16*4);
    angle += 5;
    if (angle >= 360) angle -= 360;
    draw_logo(angle);
    vga_gfx_flip();
  }

  return 0;
}
