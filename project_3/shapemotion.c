/** \file shapemotion.c
 *  \brief This is a simple pong demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains the paddle and the other a circle(ball).
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include <stdio.h>

#define GREEN_LED BIT6

// Score text area
char playerScore = 0;
// Checks if game is in session
char gameActive = 0; 
//If game is not in session or Game Over
char gameOver = 0;


//Initial screen Color
u_int bgColor = COLOR_WHITE;  //Initial screen color
//Boolean for whether screen needs to be redrawn
int redrawScreen = 1;
u_int score = 0;

Region fieldFence;          //Fence around playing field
Region fenceUpperPaddle;    //Fence for upperPaddle
Region fenceLowerPaddle;    //Fence for lowePaddle


//Rectangle that represents the upper paddle.
// size (12x3)
AbRect upperPaddle  = {abRectGetBounds, abRectCheck, {12,3}};

//Rectangle that represents the bottom paddle.
//size (12x3)
AbRect lowerPaddle = {abRectGetBounds, abRectCheck, {12,3}};

//The playing field outline
AbRectOutline fieldOutline = {
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 5}
};

//The playing field as a layer
Layer fieldLayer = {	  
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},
  {0,0}, {0,0},				 
  COLOR_BLACK,0
};


//Layer with the upper paddle
Layer layer2 = {
  (AbShape *)&upperPaddle,
  {screenWidth/2, (screenHeight/7)},
  {0,0}, {0,0},
  COLOR_BLUE,
  &fieldLayer
  };

//Layer with the bottom paddle
Layer layer1 = {	  
  (AbShape *)&lowerPaddle,
  {screenWidth/2, (screenHeight/2)+65},
  {0,0}, {0,0},  /* last & next pos */
  COLOR_BLUE,
  &layer2
 
};

//Layer with the ball
Layer layer0 = {	    
  (AbShape *)&circle8,
  {(screenWidth/2)+10, (screenHeight/2)+5},
  {0,0}, {0,0},   /* last & next pos */
  COLOR_BLACK,
  &layer1
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
//Not all layers move
MovLayer ml2 = { &layer0, {2,4}, 0};//layer for ball
MovLayer ml1 = { &layer2, {0,0}, &ml2};//layer for upper paddle 
MovLayer ml0 = { &layer1, {0,0}, &ml1};//layer for bottom paddle

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);  	/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  //for each moving layer
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */


void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary); 
    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (1*velocity);
	if(axis == 1){
	  score--;
	}
      }	// if outside of fence 
    } // for axis 
    ml->layer->posNext = newPos;
  } // for ml //
}


//Added collision from ball to paddles
void ballCheck(Region *pad1, Region *pad2, Region *ball, MovLayer *ballLayer)
{
  //checks pad1 topLeft and ball to determine collision
  if ((pad1->topLeft.axes[0] < ball->botRight.axes[0]) && 
      (pad1->botRight.axes[0] > ball->topLeft.axes[0]) &&
      (pad1->topLeft.axes[1] < ball->botRight.axes[1])) {
    //if collision detected, ball collides and changes velocity
    ballLayer->velocity.axes[1] = -ballLayer->velocity.axes[1];
    //if collided, score increases by 1
    score++;
    ml2.velocity.axes[1]--;
  }
  //check pad2 topLeft and ball to determine collision
  if ((pad2->topLeft.axes[0] < ball->botRight.axes[0]) &&
      (pad2->botRight.axes[0] > ball->topLeft.axes[0]) &&
      (pad2->botRight.axes[1] > ball->topLeft.axes[1])) {
    //if collision detected, ball collides and changes velocity
    ballLayer->velocity.axes[1] = -ballLayer->velocity.axes[1];
    //if collided, score increases by 1
    score++;
    ml2.velocity.axes[1]++;
  }
}


//method to update score
void displayScore(){
  drawString5x7(45, 10, "Score: 0", COLOR_BLUE, COLOR_WHITE);
}

void resetPositions(){

}

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */

void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	       
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  layerInit(&layer0);
  layerDraw(&layer0);
  layerGetBounds(&fieldLayer, &fieldFence);
  
  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    
    movLayerDraw(&ml0, &layer0);
    //movLayerDraw(&ml3, &layer1);
    //movLayerDraw(&ml1, &layer2);
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  static long shortPeriod;
  
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence);
    Vec2 newPos;
    Region p1;   //p1 as a Region
    Region p2;   //p2 as a Region
    Region b;    //b(ball) as a region

    //changes the velocity for collision
    vec2Add(&newPos, &ml0.layer->posNext, &ml0.velocity);
    abShapeGetBounds(ml0.layer->abShape, &newPos, &p1);
    vec2Add(&newPos, &ml1.layer->posNext, &ml1.velocity);
    abShapeGetBounds(ml1.layer->abShape, &newPos, &p2);
    vec2Add(&newPos, &ml2.layer->posNext, &ml2.velocity);
    abShapeGetBounds(ml2.layer->abShape, &newPos, &b);
    
    ballCheck(&p1, &p2, &b, &ml2);

    //I had so much trouble converting a string to an int so i
    //dispalyed an Int as a score.
    char snum[10];
    itoa(score, snum, 10);
    drawString5x7(20,20, snum, COLOR_BLUE, COLOR_WHITE);
    redrawScreen = 1;

    //instructions to catch the button pressed
    u_int switches = p2sw_read(), i;

    if(!(switches & 1)){
      //if switch 1 being pressed, move paddle left
      ml0.velocity.axes[0] = -3; 
    }else if(!(switches & 2)){
      //if switch 2 being pressed, move paddle right
      ml0.velocity.axes[0] = 3;
    }else{
      ml0.velocity.axes[0] = 0;
    }

    /****** Move Paddle state (Assembly) *********/
    /*
                 .data
      switches : .word
      ml0      : .word

                 .text
      switch1   :
                 mov &switches  , r12
                 mov #-1        , r13
                 xor r12        , r13
                 and #1         , r13
                 cmp #0         , r13
                 JZ switch2
                 mov &ml0       , r4
                 mov #-3        , 0(r4)
		 JMP end

      switch2   :mov &switches  , r12
		 mov #-1        , r13
		 xor r12        , r13
		 and #2         , r13
		 cmp #0         , r13
		 JZ not1or2
		 mov &ml0       , r4
		 mov 2(r4)      , r5
		 mov #3         , 0(r5)
		 JMP end

      not1or2  :mov &ml0       , r4
                mov 2(r4)      , r5
                mov #0         , 0(r5)
		
      end      :

     */


    
    //same instructions as above but for switches 3 and 4
    if(!(switches & 4)){
      ml1.velocity.axes[0] = -3;
    }else if(!(switches & 8)){
      ml1.velocity.axes[0] = 3;
    }else{
      ml1.velocity.axes[0] = 0;
    }
    
    count = 0;
  }
  
  P1OUT &= ~GREEN_LED;   /**< Green LED off when cpu off */

  /* Game Loop where while the game isnt active, the paddles and balls and set
     into postition for a new round to begin */
   while(!gameActive){
    //Set for text is set
    drawString5x7(30, 30, "Double Pong", COLOR_RED, COLOR_WHITE);
    drawString5x7(35, 45, "SW1 = LEFT", COLOR_BLUE, COLOR_WHITE);
    drawString5x7(35, 60, "SW4 = RIGHT", COLOR_BLUE, COLOR_WHITE);
    if(++shortPeriod == 120){  //a short period before changing state
      shortPeriod = 0;
      gameActive = 1;
      bgColor = COLOR_WHITE;
    
     lcd_init(); 
     p2sw_init(15); //sense the switches 
      layerInit(&layer0); //initiate layer
      layerDraw(&layer0); //draw the layer
     layerGetBounds(&fieldLayer, &fieldFence);

     movLayerDraw(&ml0, &layer0);
    }
  }
}
