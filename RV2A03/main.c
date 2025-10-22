#include <csr.h>
#include <uart.h>
#define printf uart_printf
#include <mul.h>
#include <stdint.h>
#include "rv2a03.h"

int a = 3;

static int init_rv2a03(void){
  rv2a03_peripheral_init();

  return 0;
}

int main() {
  // int8_t config0_reg ;
  init_rv2a03();
  
  while(1){
    init_rv2a03();
    // config0_reg = rv2a03_peripheral_read_configuration0();
    // printf("config0_reg: %x\r\n", config0_reg);
    for(int j=0; j<1000000;++j);
  }

  rv2a03_peripheral_deinit();
  return 0;
}
