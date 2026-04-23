
#include "common.h"
//#link "common.c"


#include <6502.h>

//#resource "c64-sid.cfg"
#define CFGFILE c64-sid.cfg

//#resource "sidmusic1.bin"
//#link "sidplaysfx.s"
#include "sidplaysfx.h"


char music_update() {
  sid_update();
  return IRQ_NOT_HANDLED;
}

void main(void) {
clrscr();

  // initialize SID player
  sid_init(0);


  // set IRQ routine called every frame
  set_irq(music_update, (void*)0x9f00, 0x100);

  sid_start();
  
  while (1) {
  }


  
  
}
