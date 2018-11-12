#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define IN_BUFFER 20

#define BTN_PIN   2
#define POT_PIN   A0 
#define RED_PIN   3
#define GRN_PIN   5
#define BLU_PIN   6

#define RADIO_CE  7
#define RADIO_CSN 8

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

#define INT_MAX   0xffffffffL
#define STEP_DLY  500L // 0.5ms

#define RADIO_STEPS 50  // 25ms
#define POT_STEPS   100 // 50ms
#define BTN_STEPS   20  // 10ms
#define FULL_SWP    765 // 765 colors in the sweep
#define CLR_STEPS   50  // 25ms
uint8_t SWP_STEPS;

uint8_t btnSteps, potSteps, swpSteps, clrSteps;
uint8_t mode, color;
uint8_t red, green, blue;
int sliderVal;
uint16_t sweepPos;
uint32_t t1, t2, dT, temp;
boolean prevState, currentState, btnClicked, manualControl;

const byte address[] = "test01";
RF24 radio(RADIO_CE, RADIO_CSN);
char inString[IN_BUFFER];
const char RADIO_BLUE[] = "BLUE";
const char RADIO_RIGHT[] = "RIGHT";
const char RADIO_LEFT[] = "LEFT";
uint8_t radioSteps;

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
      color = color < LAST_CLR ? color + 1 : CLR_BLK;
      if(color == CLR_BLK)
        mode = MODE_SWP;
      break;
    
    case MODE_SWP:
      mode = MODE_CLR;        
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
      manualControl = true;
      nextMode();
    }
  }
}

void checkRadio(){
  
  if(radioSteps++ >= RADIO_STEPS){
    
    radioSteps = 0;
  
    if(radio.available()){
      
      radio.read(&inString, IN_BUFFER);
      
      if(strncmp(inString, RADIO_BLUE, sizeof(RADIO_BLUE)) == 0){
        manualControl = false;
        nextMode();
      }
      
      if(mode == MODE_SWP || color == CLR_MAN){
      
        if(strncmp(inString, RADIO_RIGHT, sizeof(RADIO_RIGHT)) == 0){
          
          manualControl = false;
          
          sliderVal += 40;
          
          if(sliderVal > 1023){
            
            if(mode == MODE_SWP)
              sliderVal = 1023;
            
            else if(color == CLR_MAN)
              sliderVal -= 1024;
          }
        }
        
        if(strncmp(inString, RADIO_LEFT, sizeof(RADIO_LEFT)) == 0){
          
          manualControl = false;
          
          sliderVal -= 40;
          
          if(sliderVal < 0){
            
            if(mode == MODE_SWP)
              sliderVal = 0;
            
            else if(color == CLR_MAN)
              sliderVal += 1024;
          }          
        }
        
      }
      
    }
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
  
  if(t2 < t1)
    dT = (INT_MAX - t1) + t2 + 1;
  else
    dT = t2 - t1;
  
  dT = dT < STEP_DLY ? STEP_DLY - dT : 0;
  
  delayMicroseconds(dT);
  t1 = micros();
}

void setup(){
  
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();
  
  prevState = true;
  currentState = true;
  
  btnSteps   = 0;
  potSteps   = 0;
  swpSteps   = 0;
  clrSteps   = 0;
  radioSteps = 0;
  
  color = CLR_BLK;
  mode = MODE_CLR;
  
  sweepPos = 0;

  pinMode(BTN_PIN, INPUT_PULLUP);
  t1 = micros();
}

void loop() {

  readButton();
  checkRadio();
  
  if(manualControl)
    readPot();

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
