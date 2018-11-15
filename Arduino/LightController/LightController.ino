#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// radio read buffer
#define IN_BUFFER   20

// pins
#define BTN_PIN     2
#define POT_PIN     A0 
#define RED_PIN     3
#define GRN_PIN     5
#define BLU_PIN     6
#define RADIO_CE    7
#define RADIO_CSN   8

// modes (to remove or integrate with colors)
#define MODE_CLR    0
#define MODE_SWP    1
#define LAST_MODE   1

// colors
#define CLR_BLK     0
#define CLR_RED     1
#define CLR_GRN     2
#define CLR_BLU     3
#define CLR_CYN     4
#define CLR_YLW     5
#define CLR_MAG     6
#define CLR_WHT     7
#define CLR_SEL     8
#define LAST_CLR    8
#define FULL_SWP    765 // 765 colors in the sweep

// max 32-bit int value
#define INT_MAX     0xffffffffL

// time step duration, in microseconds
#define STEP_DLY    500L // 0.5ms

// delays, as multiples of STEP_DLY
#define RADIO_STEPS 50   // 25ms
#define POT_STEPS   100  // 50ms
#define BTN_STEPS   20   // 10ms
#define CLR_STEPS   50   // 25ms
uint8_t SWP_STEPS;

// flags
boolean prevState, currentState, btnClicked, radioControl;

// timing stuff
uint32_t t1, t2, dT, temp;
uint8_t btnSteps, potSteps, swpSteps, clrSteps;

// color stuff
uint8_t mode, color;
uint8_t red, green, blue;
int sliderVal;
uint16_t sweepPos;

// radio stuff
RF24 radio(RADIO_CE, RADIO_CSN);
const byte address[] = "test01";
char inString[IN_BUFFER];
const char RADIO_BLUE[] = "BLUE";
const char RADIO_RIGHT[] = "RIGHT";
const char RADIO_LEFT[] = "LEFT";
uint8_t radioSteps;

// output the current rgb values
void applyColor(){
  analogWrite(RED_PIN, red);
  analogWrite(GRN_PIN, green);
  analogWrite(BLU_PIN, blue);
}

// set color from sweep position
void setSweepPos(){
  if(sweepPos < 255){
    red = 255 - sweepPos;
    green = sweepPos;
    blue = 0;
  }else if(sweepPos < 510){
    red = 0;
    green = 510 - sweepPos;
    blue = sweepPos - 255;
  }else if(sweepPos < 765){
    red = sweepPos - 510;
    green = 0;
    blue = 765 - sweepPos;
  }
}

// applies one of the solid color modes
void setSolid(){
  
  if(clrSteps++ >= CLR_STEPS){
    
    clrSteps = 0;
    
    switch(color){
      
      // black
      case CLR_BLK:
        red = 0;
        green = 0;
        blue = 0;
        break;
      
      // red
      case CLR_RED:
        red = 255;
        green = 0;
        blue = 0;
        break;
      
      // green
      case CLR_GRN:
        red = 0;
        green = 255;
        blue = 0;
        break;
      
      // blue
      case CLR_BLU:
        red = 0;
        green = 0;
        blue = 255;
        break;
      
      // cyan
      case CLR_CYN:
        red = 0;
        green = 255;
        blue = 255;
        break;
      
      // yellow
      case CLR_YLW:
        red = 255;
        green = 255;
        blue = 0;
        break;
      
      // magenta
      case CLR_MAG:
        red = 255;
        green = 0;
        blue = 255;
        break;
      
      // white
      case CLR_WHT:
        red = 255;
        green = 255;
        blue = 255;
        break;
        
      // user select
      case CLR_SEL:
        temp = sliderVal;
        temp *= 764;
        temp /= 1023;
        sweepPos = temp;
        setSweepPos();
        break;
    }
    
    applyColor();
  }
}

// changes color mode
void nextMode(){
  
  switch(mode){
    
    case MODE_CLR:
      // go to next color or black if at end
      color = color < LAST_CLR ? color + 1 : CLR_BLK;
      if(color == CLR_BLK)
        mode = MODE_SWP;
      break;
    
    case MODE_SWP:
      mode = MODE_CLR; // go back to color mode        
      break; 
  }
}

// checks the current button state, checks if it was clicked, and calls nextMode()
void readButton(){
  
  if(btnSteps++ >= BTN_STEPS){
    
    currentState = digitalRead(BTN_PIN);
    btnClicked = currentState && !prevState; // button up
    prevState = currentState;
    
    if(btnClicked){
      radioControl = false;
      nextMode();
    }
  }
}

// check for commands from radio module
void checkRadio(){
  
  if(radioSteps++ >= RADIO_STEPS){
    
    radioSteps = 0;
  
    if(radio.available()){
      
      radio.read(&inString, IN_BUFFER);
      
      // blue remote button
      if(strncmp(inString, RADIO_BLUE, sizeof(RADIO_BLUE)) == 0){
        radioControl = true;
        nextMode();
      }
      
      // only check the arrow buttons if the current mode allows it
      if(mode == MODE_SWP || color == CLR_SEL){
      
        // right remote button
        if(strncmp(inString, RADIO_RIGHT, sizeof(RADIO_RIGHT)) == 0){
          
          radioControl = true; // take control of slider
          
          sliderVal += 40; // move slider forward
          
          if(sliderVal > 1023){
            
            // stop increasing at 1023 for sweep delay
            if(mode == MODE_SWP)
              sliderVal = 1023;
            
            // cycle back to 0 for color select
            else if(color == CLR_SEL)
              sliderVal -= 1024;
          }
        }
        
        // left radio button
        if(strncmp(inString, RADIO_LEFT, sizeof(RADIO_LEFT)) == 0){
          
          radioControl = true; // take control of slider
          
          sliderVal -= 40; // move slider back
          
          if(sliderVal < 0){
            
            // stop decreasing at 0 for sweep delay
            if(mode == MODE_SWP)
              sliderVal = 0;
            
            // cycle back to 1023 for color select
            else if(color == CLR_SEL)
              sliderVal += 1024;
          }          
        }
      } // mode check
      
    } // radio availability check    
  }
}

// reads potentiometer
void readPot(){
  if(potSteps++ >= POT_STEPS){
    potSteps = 0;
    sliderVal = analogRead(POT_PIN);
  }
}

// moves to the next step of a sweep
void doSweep(){
  
  SWP_STEPS = 64 - (sliderVal >> 4);
  
  if(swpSteps++ >= SWP_STEPS){    
    swpSteps = 0;
    sweepPos = sweepPos < FULL_SWP ? sweepPos + 1 : 0;
    setSweepPos();
    applyColor();
  }
}

// waits up to STEP_DLY microseconds after the previous call
void timeStep(){
  
  t2 = micros();
  
  // get time since last call
  if(t2 < t1)
    dT = (INT_MAX - t1) + t2 + 1; // handle overflow case
  else
    dT = t2 - t1;
  
  // get required delay (or none) to extend cycle to STEP_DLY microseconds
  dT = dT < STEP_DLY ? STEP_DLY - dT : 0;
  
  delayMicroseconds(dT);
  t1 = micros();
}

// setup
void setup(){
  
  // initialize radio
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  
  // initialize booleans
  prevState    = true;
  currentState = true;
  radioControl = true;
  
  // initialize delay steps
  // stagger actions
  btnSteps   = 0;
  potSteps   = 1;
  swpSteps   = 2;
  clrSteps   = 3;
  radioSteps = 4;
  
  // initialize colors
  color = CLR_BLK;
  mode  = MODE_CLR;
  sweepPos = 0;

  // initialize button pin and get time
  pinMode(BTN_PIN, INPUT_PULLUP);
  t1 = micros();
}

// main loop
void loop() {

  // check for commands
  readButton();
  checkRadio();
  
  // use pot if not being controlled by radio
  if(!radioControl)
    readPot();

  // apply current mode
  switch(mode){
      
    case MODE_CLR:
      setSolid();
      break;
      
    case MODE_SWP:
      doSweep();
      break; 
  }
  
  timeStep(); // go to next time step
}
