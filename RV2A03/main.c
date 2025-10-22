#include <csr.h>
#include <uart.h>
#define printf uart_printf
#include <mul.h>
#include "rv2a03.h"

int a = 3;

int main() {
  while(1){
    for(int i='A'; i<='Z';++i){
      uart_putc((char)i);
      if(i == 'Z') i = 'A';
      for(int j=0; j<1000000;++j);
    }
  }
  return 0;
}
