//reads from serial and outputs to led

#include <SoftwareSerial.h>

SoftwareSerial mySerial(0,1); //rx | tx
//#define ledPin 15
int state = 0;
char mychar = 32;

void setup() {
  mySerial.begin(38400);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if(mySerial.available()>0){
    state = mySerial.read();
  }
  // read state as dec and save as char to output as char
  mychar = state;
  Serial.print(mychar);
  delay(35);


// ~~~~~~~~~~~~ button test ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /*if(mySerial.available()>0){
    state = mySerial.read();
  }
  if (state == '1'){
    digitalWrite(ledPin, HIGH);
    Serial.println('1');
    state = 0;
  }
  else if(state == '0'){
    digitalWrite(ledPin, LOW);
    Serial.println('0');
    state = 0;
  }*/
    
}
