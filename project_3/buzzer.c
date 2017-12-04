#include <msp430.h>
#include "buzzer.h"


/* Code snippets were re-used from Lab 2 to create a sound when
   ball collides with a paddle.
*/ 

static unsigned int note[32] = {1318};
static unsigned int counter 0;
static unsigned int currentNote = 0;

void buzzer_init(){
  timerAUpmode();              /*used to drive speaker*/
  P2SEL2 &= ~(BIT6|BIT7);     
  P2SEL &= ~BIT7;
  P2SEL |= BIT6;
  P2DIR = BIT6;                /*half cycle*/

  buzzer_sound();
}

void buzzer_sound1(){
  counter++;
  if(counter == 50){
    buzzer_set_note(notes[currentNote]);
    currentNote++;
    if(currentNote > 31){
      currentNote = 0;
      state = 0;
      led_update();
    }
    counter = 0;
  }
}

void buzzer_sound(){

}

void buzzer_set_note(short note){
  CCR0 = note;
  CCR1 = note >> 1;
}
