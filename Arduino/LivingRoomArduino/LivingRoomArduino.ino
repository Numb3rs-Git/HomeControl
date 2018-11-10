#include <IRremote.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define RECV_PIN   2
#define LED_PIN    4
#define BTN_PIN    3
#define RELAY_PIN  5
#define DOOR_PIN   6
#define MOTION_PIN 7
#define RADIO_CE   8
#define RADIO_CSN  9
#define IN_BUFFER 20

#define REMOTE_RED    0x4FB3AC5
#define REMOTE_GREEN  0x4FBBA45
#define REMOTE_YELLOW 0x4FB7A85
#define REMOTE_BLUE   0x4FBFA05
#define REMOTE_RIGHT  0x4FBD22D
#define REMOTE_UP     0x4FBE21D
#define REMOTE_LEFT   0x4FB926D
#define REMOTE_DOWN   0x4FBB24D
#define REMOTE_ENTER  0x4FB52AD

boolean overrideState;
boolean relayState;

boolean btnState;
boolean btnPrev;

boolean motionState;
boolean motionPrev;

boolean doorOpen;
boolean doorPrev;

boolean inputRecieved;
char inString[IN_BUFFER];
char charin;
uint8_t nChars;

IRrecv irrecv(RECV_PIN);
decode_results results;

RF24 radio(RADIO_CE, RADIO_CSN);
const byte address[6] = "test01";
const char RADIO_BLUE[4] = "BLUE";
const char RADIO_RIGHT[5] = "RIGHT";
const char RADIO_LEFT[4] = "LEFT";

void setup()
{
  overrideState = false;
  relayState = false;
  inputRecieved = false;
  nChars = 0;
  
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
  
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();
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
      case REMOTE_RED:
        toggleOverride();
        break;
    
      case REMOTE_GREEN:
        if(overrideState)
          setRelay(!relayState);
        break;

      case REMOTE_YELLOW:
        break;

      case REMOTE_BLUE:
        radio.write(&RADIO_BLUE, sizeof(RADIO_BLUE));
        break;
        
      case REMOTE_RIGHT:
        radio.write(&RADIO_RIGHT, sizeof(RADIO_RIGHT));
        break;
      
      case REMOTE_UP:
        break;
      
      case REMOTE_LEFT:
        radio.write(&RADIO_LEFT, sizeof(RADIO_LEFT));
        break;
      
      case REMOTE_DOWN:
        break;
      
      case REMOTE_ENTER:
        break;
        
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
