#include <csr.h>
#include <uart.h>
#define printf uart_printf
#include <timer.h>

#if 0
void timer_callback(void*) {
  int mtime = get_mtime();
  set_alarm(200, timer_callback, NULL);
  printf("Timer @ %d\n", mtime);
}

void timer_callback2(void*) {
  int mtime = get_mtime();
  set_alarm(1000, timer_callback2, NULL);
  printf("Timer2 @ %d\n", mtime);
}

void timer_callback3(void*) {
  int mtime = get_mtime();
  set_alarm(1500, timer_callback3, NULL);
  printf("Timer3 @ %d\n", mtime);
}

int main() {
  printf("Hello\n");
  set_alarm(10, timer_callback, NULL);
  set_alarm(15, timer_callback2, NULL);
  set_alarm(20, timer_callback3, NULL);
  
  while (1);
  return 0;
}
#else

void tqv_timer_interrupt(void) {
  int mtime = get_mtime();
  printf("Timer interrupt @ %d\n", mtime);
  set_mtimecmp(1000000 + mtime);
}

int main() {
  printf("Hello\n");
  trigger_timer_interrupt(1000000);
  while (1);
  return 0;
}
#endif