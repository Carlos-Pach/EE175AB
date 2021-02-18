#include <SoftwareSerial.h>

#define bt_power 12 //5v
#define pot A7
int potVal = 0;
int plantNum = 0x02;
int desiredVal = 562;
bool isWatered = true;
unsigned char myVal = 0x00;

SoftwareSerial mySerial(2,1); // RX | TX

void setup()
{
    mySerial.begin(38400);
    pinMode(pot, INPUT);
    pinMode(bt_power, OUTPUT);
    
    digitalWrite(bt_power, LOW);
}

void loop()
{
  delay(50100);
  
  // power on the BT
  digitalWrite(bt_power, HIGH);
  delay(30000); // on for some time

  potVal = analogRead(A7);
  // pot reading between 750 and 0, if we get 75% = 562
  if (potVal < desiredVal){
    isWatered = bitWrite(myVal,2,0);
  }
  else{
    isWatered = bitWrite(myVal,2,1);
  }
  
  myVal = myVal | plantNum;
  myVal = myVal | (potVal/32) << 3;
  mySerial.println(myVal, HEX);
  myVal = 0x00;
  
  delay(100);
  digitalWrite(bt_power, LOW);
  
  delay(70100);
}
