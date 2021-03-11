#include <SoftwareSerial.h>

#define pot A7
#define ledPin 13
int potVal = 0;
int plantNum = 0x00;
int desiredVal = 450;  // pot reading between 300 and 600
bool isWatered = true;
unsigned char myVal = 0x00;

SoftwareSerial mySerial(2, 1); // RX | TX

void setup()
{
  mySerial.begin(38400);
  pinMode(pot, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // set the pins to LOW
  digitalWrite(ledPin,LOW);
}

void loop()
{
  potVal = analogRead(A7);
  if (potVal < desiredVal){
    isWatered = bitWrite(myVal,2,0);
  }
  else{
    isWatered = bitWrite(myVal,2,1);
  }
  
  myVal = myVal | plantNum;
  myVal = myVal | (potVal/32) << 3;
  mySerial.print(myVal, HEX);
  myVal = 0x00;
  digitalWrite(ledPin,HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);

  delay(4000);
}
