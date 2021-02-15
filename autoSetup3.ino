#include <SoftwareSerial.h>

#define bt_power 12 //5v
#define pot A7
int potVal = 0;
int plantNum = 0;
int desiredVal = 562;
bool isWatered = true;
unsigned char myVal = 0x00;

SoftwareSerial mySerial(2,1); // RX | TX

void setup()
{
  mySerial.begin(38400);
  
  // set the pins to OUTPUT
  pinMode(bt_power, OUTPUT);

  // set the pins to LOW
  digitalWrite(bt_power, LOW);
}

void loop()
{
  delay(20000); // bluetooth should be off while slave 1 is on
  delay(100);
  delay(20000); // reset time
  delay(20000); // off for slave 2
  delay(100);
  delay(20000); // reset
  // power on the BT
  digitalWrite(bt_power, HIGH);
  delay(20000); // on for some time

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
  delay(20000); // reset
}
