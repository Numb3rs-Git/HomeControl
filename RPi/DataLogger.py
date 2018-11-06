from serial import Serial
from datetime import datetime

serialport = serial.Serial("/dev/ttyAMA0", 9600, timeout=0.5)
validEvents = ["DOOR_OPENED", "DOOR_CLOSED", "RELAY_ON", "RELAY_OFF"]
validEvents += ["MOTION_EVENT", "OVERRIDE_ENABLED", "OVERRIDE_DISABLED"]
inString = ""


def logEvent(event):
  entry = str(datetime.now())[:-7]
  entry += event
  f = open("/home/pi/EventLog.txt", "a")
  f.write(entry + "\n")
  f.close()


def handleEvent(event):
  if event in validEvents:
    logEvent(event)
  

while True:    
    c = serialport.read()
    if c == "\n":
      handleEvent(inString)
      inString = ""
    else:
      inString += c
      