#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;
Servo servos[6];

int servoPins[6] = {13,12,14,27,26,25};

int navButtons[3] = {32,33,16};
String navNames[3] = {"A","B","C"};

int primaryButtons[6] = {4,5,18,19,21,22};

int touchPins[3] = {15,2,17};

int vibrationPin = 23;

int lastNavState[3];
int lastPrimaryState[6];
int lastTouchState[3];

const int debounceDelay = 50;

// debounce timers
unsigned long lastDebounceTime[6] = {0,0,0,0,0,0};
unsigned long lastNavDebounceTime[3] = {0,0,0};
unsigned long lastTouchDebounceTime[3] = {0,0,0};

void setup() {

  Serial.begin(115200);
  SerialBT.begin("BrailleX"); // Bluetooth device name

  Serial.println("BrailleX USB Ready");
  SerialBT.println("BrailleX Bluetooth Ready");

  // attach servos
  for(int i=0;i<6;i++){
    servos[i].attach(servoPins[i]);
    servos[i].write(0); // initial position
  }

  // navigation buttons
  for(int i=0;i<3;i++){
    pinMode(navButtons[i], INPUT_PULLUP);
    lastNavState[i] = HIGH;
  }

  // braille buttons
  for(int i=0;i<6;i++){
    pinMode(primaryButtons[i], INPUT_PULLUP);
    lastPrimaryState[i] = HIGH;
  }

  // touch sensors
  for(int i=0;i<3;i++){
    pinMode(touchPins[i], INPUT);
    lastTouchState[i] = LOW;
  }

  pinMode(vibrationPin, OUTPUT);

}

void loop() {

  checkNavButtons();
  checkPrimaryButtons();
  checkTouchSensors();
  handleSerial();

  // Optional: Prevent Bluetooth crashes
  if(!SerialBT.hasClient()){
    delay(10);
  }

}

//////////////////////////////////////////////////////////////
// HELPER FUNCTION: SEND TO BOTH SERIAL & BLUETOOTH
//////////////////////////////////////////////////////////////

void sendMessage(String msg){
  Serial.println(msg);
  SerialBT.println(msg);
}

//////////////////////////////////////////////////////////////
// NAVIGATION BUTTONS
//////////////////////////////////////////////////////////////

void checkNavButtons(){

  for(int i=0;i<3;i++){

    int state = digitalRead(navButtons[i]);

    if(state != lastNavState[i]){

      if(millis() - lastNavDebounceTime[i] > debounceDelay){

        if(state == LOW)
          sendMessage(navNames[i] + "_PRESS");
        else
          sendMessage(navNames[i] + "_RELEASE");

        lastNavState[i] = state;
        lastNavDebounceTime[i] = millis();

      }

    }

  }

}

//////////////////////////////////////////////////////////////
// BRAILLE DOT BUTTONS
//////////////////////////////////////////////////////////////

void checkPrimaryButtons(){

  for(int i=0;i<6;i++){

    int state = digitalRead(primaryButtons[i]);

    if(state != lastPrimaryState[i]){

      if(millis() - lastDebounceTime[i] > debounceDelay){

        if(state == LOW){
          sendMessage("B" + String(i+1) + "_PRESS");
        }
        else{
          sendMessage("B" + String(i+1) + "_RELEASE");
        }

        lastPrimaryState[i] = state;
        lastDebounceTime[i] = millis();

      }

    }

  }

}

//////////////////////////////////////////////////////////////
// TOUCH SENSORS
//////////////////////////////////////////////////////////////

void checkTouchSensors(){

  for(int i=0;i<3;i++){

    int state = digitalRead(touchPins[i]);

    if(state != lastTouchState[i]){

      if(millis() - lastTouchDebounceTime[i] > debounceDelay){

        if(state == HIGH){
          sendMessage("T" + String(i+1) + "_PRESS");
        }
        else{
          sendMessage("T" + String(i+1) + "_RELEASE");
        }

        lastTouchState[i] = state;
        lastTouchDebounceTime[i] = millis();

      }

    }

  }

}

//////////////////////////////////////////////////////////////
// SERIAL COMMAND HANDLERS
//////////////////////////////////////////////////////////////

void handleSerial(){

  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    processCommand(cmd);
  }

  if(SerialBT.available()){
    String cmd = SerialBT.readStringUntil('\n');
    cmd.trim();
    processCommand(cmd);
  }

}

void processCommand(String cmd){

  //////////////////////////////////////////////
  // SERVO COMMAND
  // Example: S1:90
  //////////////////////////////////////////////

  if(cmd.startsWith("S")){

    int servoIndex = cmd.substring(1,2).toInt() - 1;
    int angle = cmd.substring(3).toInt();

    if(servoIndex >=0 && servoIndex <6){

      angle = constrain(angle,0,180);
      servos[servoIndex].write(angle);

    }

  }

  //////////////////////////////////////////////
  // BRAILLE CHORD CONTROL
  // Example: CHORD:100100
  //////////////////////////////////////////////

  if(cmd.startsWith("CHORD:")){

    String pattern = cmd.substring(6);

    for(int i=0;i<6;i++){

      if(pattern[i] == '1')
        servos[i].write(90);   // raise dot
      else
        servos[i].write(0);    // lower dot

    }

  }

  //////////////////////////////////////////////
  // VIBRATION COMMANDS
  //////////////////////////////////////////////

  if(cmd == "VIB:CONFIRM"){

    digitalWrite(vibrationPin,HIGH);
    delay(120);
    digitalWrite(vibrationPin,LOW);

  }

  if(cmd == "VIB:ERROR"){

    for(int i=0;i<3;i++){

      digitalWrite(vibrationPin,HIGH);
      delay(80);
      digitalWrite(vibrationPin,LOW);
      delay(80);

    }

  }

  if(cmd == "VIB:DOT"){

    digitalWrite(vibrationPin,HIGH);
    delay(40);
    digitalWrite(vibrationPin,LOW);

  }

}
