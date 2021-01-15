// reads input from a button and sends '1' or '0' through bluetooth serial

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,1); //rx | tx
#define buttonPin 5
#define led 8
int buttonState = 0;
int analogVal = 0;
int flag = 0;

void setup() {
  mySerial.begin(38400);
  pinMode(buttonPin, INPUT);
  pinMode(led, OUTPUT);
}

void loop() {
 // analogVal = analogRead(A7);
  
  buttonState = digitalRead(buttonPin);

  if(buttonState == HIGH){
    //mySerial.print('1');
    flag = 1;
    digitalWrite(led, HIGH);
  }
  else{
    //mySerial.print('0');
    flag = 0;
    digitalWrite(led, LOW);
  }
  mySerial.println(flag);
}
