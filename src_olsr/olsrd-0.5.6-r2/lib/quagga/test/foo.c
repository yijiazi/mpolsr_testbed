#include "quagga.h"

int main (void) {
  
  init_zebra();
  zebra_redistribute (2);
  //  zebra_redistribute (1);
  while (!sleep (1)) zebra_check();
  return 0;
}
