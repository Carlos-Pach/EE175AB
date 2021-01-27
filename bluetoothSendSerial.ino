// reads input from a button and sends '1' or '0' through bluetooth serial

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,1); //rx | tx
//#define buttonPin 5
//#define led 8
//int buttonState = 0;

#define pot A7
int potVal = 0;
int plantNum = 0x01;
int desiredVal = 562;
bool isWatered = true;
unsigned char myVal = 0x00;

void setup() {
  mySerial.begin(38400);
  //pinMode(buttonPin, INPUT);
  //pinMode(led, OUTPUT);
  pinMode(pot, INPUT);
}

void loop() {
  potVal = analogRead(A7);
  // pot reading between 750 and 0, if we get 75% = 562
  if (potVal < desiredVal){
    isWatered = bitWrite(myVal,2,0);
  }
  else{
    isWatered = bitWrite(myVal,2,1);
  }
  
  myVal = myVal | plantNum;
  myVal = myVal | potVal/32 << 3;
  mySerial.println(potVal);
  mySerial.println(myVal, HEX);
  myVal = 0x00;
  
  
  //~~~~~~~~~ button test ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /*buttonState = digitalRead(buttonPin);
  if(buttonState == HIGH){
    flag = 1;
    digitalWrite(led, HIGH);
  }
  else{
    flag = 0;
    digitalWrite(led, LOW);
  }
  mySerial.println(flag);*/
}
