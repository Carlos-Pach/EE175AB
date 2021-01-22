// reads input from a button and sends '1' or '0' through bluetooth serial

#include <SoftwareSerial.h>

SoftwareSerial mySerial(2,1); //rx | tx
//#define buttonPin 5
//#define led 8
//int buttonState = 0;

#define pot A7
int potVal = 0;
int plantNum = 1;
int desiredVal = 562;
bool isWatered = true;

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
    isWatered = false;
  }
  else{
    isWatered = true; 
  }

  mySerial.print("plant #: ");
  mySerial.println(plantNum);
  delay(2500);
  mySerial.print("potentiometer %: ");
  mySerial.println(potVal);
  delay(2500);
  mySerial.print("desired water level: ");
  mySerial.println(desiredVal);
  delay(2500);
  mySerial.print("water state: ");
  mySerial.println(isWatered);
  delay(2500);
  
  
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
