#include <IRremote.h>

#define RECV_PIN   13
#define LED_PIN     4
#define BTN_PIN     5
#define RELAY_PIN  10
#define DOOR_PIN   11
#define MOTION_PIN  8
#define IN_BUFFER  20

boolean overrideState = false;
boolean relayState = false;
boolean btnState;
boolean btnPrev;
boolean motionState;
boolean motionPrev;
boolean doorOpen;
boolean doorPrev;
boolean inputRecieved = false;
char inString[IN_BUFFER];
char charin;
uint8_t nChars = 0;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, overrideState);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  pinMode(BTN_PIN, INPUT_PULLUP);
  btnState = digitalRead(BTN_PIN);

  pinMode(MOTION_PIN, INPUT);
  motionState = digitalRead(MOTION_PIN);
  
  pinMode(DOOR_PIN, INPUT);
  doorOpen = digitalRead(DOOR_PIN);
  
  irrecv.enableIRIn();
  Serial.begin(9600);
}

void loop() {

  handleRemote();
  handleSerial();
  
  checkButton();
  checkDoor();
  checkMotion();
  
  delay(10);
}

void checkButton(){
  btnPrev = btnState;
  btnState = digitalRead(BTN_PIN);
  if(!btnState && btnPrev)
    toggleOverride();
}

void checkMotion(){
  motionPrev = motionState;
  motionState = digitalRead(MOTION_PIN);
  if(motionState && !motionPrev)
    Serial.print(F("MOTION_EVENT\n"));
}

void checkDoor(){
  doorPrev = doorOpen;
  doorOpen = digitalRead(DOOR_PIN);

  if(doorOpen && !doorPrev)
    Serial.print(F("DOOR_OPENED\n"));

  if(!doorOpen && doorPrev)
    Serial.print(F("DOOR_CLOSED\n"));
  
  if(!overrideState)
    setRelay(doorOpen);
}

void handleRemote(){

  if(irrecv.decode(&results)){

    switch(results.value){
      case 0x4FB3AC5: // red button
        toggleOverride();
        break;
    
      case 0x4FBBA45: // green button
        if(overrideState)
          setRelay(!relayState);
        break;

      case 0x4FB7A85: // yellow button
        break;

      case 0x4FBFA05: // blue button
        break;
    }
    
    irrecv.resume();
  }
}

void handleSerial(){
  if(inputRecieved){
    
    /* process messages here */
    
    inputRecieved = false;
    nChars = 0;
    for(uint8_t i = 0; i < IN_BUFFER; i++)
      inString[i] = ' ';
  }
}

void serialEvent(){
  while(Serial.available()){
    charin = (char)Serial.read();
    if(charin == '\n')
      inputRecieved = true;
    else if(nChars < IN_BUFFER)
      inString[nChars++] = charin;
  }
}

void toggleOverride(){
  overrideState = !overrideState;
  if(overrideState)
    Serial.print(F("OVERRIDE_ENABLED\n"));
  else
    Serial.print(F("OVERRIDE_DISABLED\n"));
  digitalWrite(LED_PIN, overrideState);
}

void setRelay(boolean state){
  if(state && !relayState)
    Serial.print(F("RELAY_ON\n"));
  else if(!state && relayState)
    Serial.print(F("RELAY_OFF\n"));
  relayState = state;
  digitalWrite(RELAY_PIN, relayState);
}
