#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define IN_BUFFER 20

#define BTN_PIN   12
#define POT_PIN   A0 
#define RED_PIN   9
#define GRN_PIN   10
#define BLU_PIN   11

#define MODE_CLR  0
#define MODE_SWP  1
#define LAST_MODE 1

#define CLR_BLK   0
#define CLR_RED   1
#define CLR_GRN   2
#define CLR_BLU   3
#define CLR_CYN   4
#define CLR_YLW   5
#define CLR_MAG   6
#define CLR_WHT   7
#define CLR_MAN   8
#define LAST_CLR  8

#define SWP_MAN   0
#define SWP_AUTO  1

#define INT_MAX   0xffffffffL
#define STEP_DLY  500L // 0.5ms

#define POT_STEPS 100 // 50ms
#define BTN_STEPS 20 // 10ms
#define FULL_SWP  765 // 765 colors in the sweep
#define CLR_STEPS 50 // 25ms
uint8_t SWP_STEPS;

uint8_t btnSteps, potSteps, swpSteps, clrSteps;
uint8_t mode, swpMode, color;
uint8_t red, green, blue;
uint16_t potVal, sweepPos;
uint32_t t1, t2, dT, temp;
boolean prevState, currentState, btnClicked;

const byte address[6] = "test01";
RF24 radio(7, 8); // CE, CSN

char inString[IN_BUFFER];

// outputs the current rgb values
void applyColor(){
  analogWrite(RED_PIN, red);
  analogWrite(GRN_PIN, green);
  analogWrite(BLU_PIN, blue);
}

// maps an integer to its position in a full sweep
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
      
      case CLR_BLK:
        red = 0;
        green = 0;
        blue = 0;
        break;
      
      case CLR_RED:
        red = 255;
        green = 0;
        blue = 0;
        break;
        
      case CLR_GRN:
        red = 0;
        green = 255;
        blue = 0;
        break;
        
      case CLR_BLU:
        red = 0;
        green = 0;
        blue = 255;
        break;
      
      case CLR_CYN:
        red = 0;
        green = 255;
        blue = 255;
        break;
        
      case CLR_YLW:
        red = 255;
        green = 255;
        blue = 0;
        break;
        
      case CLR_MAG:
        red = 255;
        green = 0;
        blue = 255;
        break;
        
      case CLR_WHT:
        red = 255;
        green = 255;
        blue = 255;
        break;
        
      case CLR_MAN:
        temp = potVal;
        temp *= 764;
        temp /= 1023;
        sweepPos = temp;
        setSweepPos();
        break;
    }
    
    applyColor();
  }
}

// runs when the button is clicked
void onClick(){
  
  switch(mode){
    
    case MODE_CLR:
      color = color < LAST_CLR ? color + 1 : CLR_BLK;
      if(color == CLR_BLK)
        mode = MODE_SWP;
      break;
    
    case MODE_SWP:
      if(swpMode == SWP_AUTO){
        swpMode = SWP_MAN;
        mode = MODE_CLR;
      }
      else
        swpMode = SWP_AUTO;        
      break; 
  }
}

// checks the current button state, checks if it was clicked, and calls onClick()
void readButton(){
  
  if(btnSteps++ >= BTN_STEPS){
    
    currentState = digitalRead(BTN_PIN);
    btnClicked = currentState && !prevState; // button up
    prevState = currentState;
    
    if(btnClicked)
      onClick();
  }
}

// reads potentiometer
void readPot(){
  if(potSteps++ >= POT_STEPS){
    potSteps = 0;
    potVal = analogRead(POT_PIN);
  }
}

// moves to the next step of a sweep
void doSweep(){
  
  if(swpMode == SWP_MAN)
    SWP_STEPS = (potVal >> 5) + 1;
  
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
  
  if(t2 < t1)
    dT = (INT_MAX - t1) + t2 + 1;
  else
    dT = t2 - t1;
  
  dT = dT < STEP_DLY ? STEP_DLY - dT : 0;
  
  delayMicroseconds(dT);
  t1 = micros();
  
}

void checkRadio(){
  
  if(radio.available()){
    
    radio.read(&inString, IN_BUFFER);
    
    if(strncmp(inString, "COLOR_UP", 8) == 0)
      return;
    else if(strncmp(inString, "COLOR_DN", 8) == 0)
      return;
    else if(strncmp(inString, "SWEEP_UP", 8) == 0)
      return;
    else if(strncmp(inString, "SWEEP_DN", 8) == 0)
      return;
    
  }
}

void setup(){
  /*
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  */
  prevState = true;
  currentState = true;
  
  btnSteps = 0;
  potSteps = 0;
  swpSteps = 0;
  clrSteps = 0;
  
  color = CLR_BLK;
  mode = MODE_CLR;
  
  sweepPos = 0;
  swpMode = SWP_MAN;

  pinMode(BTN_PIN, INPUT_PULLUP);
  t1 = micros();
}

void loop() {

  readButton();
  readPot();
  checkRadio();

  switch(mode){
      
    case MODE_CLR:
      setSolid();
      break;
      
    case MODE_SWP:
      doSweep();
      break;
      
  }
  
  timeStep();
}
