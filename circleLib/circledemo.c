#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include "abCircle.h"

AbRect rect10 = {abRectGetBounds, abRectCheck, {12,3}};; /**< 12x3 rectangle */

u_int bgColor = COLOR_BLACK;

//Layer that is to be represented as the lower paddle
Layer layer1 = {    
  (AbShape *)&rect10,
  {screenWidth/2,(screenHeight/2)+60}, /**< bottom of the screen */
  {0,0}, {0,0}, COLOR_BLUE, 0	   /* next & last pos */
};

//Layer to be represnted for the ball
Layer layer0 = { 
  (AbShape *)&circle8,
  {(screenWidth/2), (screenHeight/2)}, /**< centered at half of the screen */
  {0,0}, {0,0}, COLOR_WHITE, &layer1      /* next & last pos */
};

//Layer to be represented for the upper paddle

Layer layer2 = {
  (AbShape *)&rect10,
  {screenWidth/2, (screenHeight/2)+30},
  {0,0}, {0,0}, COLOR_BLUE, 0
};

int
main()
{
  configureClocks();
  lcd_init();

  clearScreen(COLOR_BLUE);
  drawString5x7(20,20, "hello", COLOR_GREEN, COLOR_RED);

  layerDraw(&layer0);

}
