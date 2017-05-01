#include <msp430.h>
#include "buzzer.h"

void buzzer_init(){
  timerAUpmode();              /*used to drive speake*/
  P2SEL2 &= ~(BIT6|BIT7);     
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;                /*half cycle*/
}

void buzzer_set_note(short note){
  CCR0 = note;
  CCR1 = note >> 1;
}
