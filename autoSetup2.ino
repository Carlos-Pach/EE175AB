#include <SoftwareSerial.h>

#define bt_power 12 //5v
#define pot A7
int potVal = 0;
int plantNum = 0x02;
int desiredVal = 562;
bool isWatered = true;
unsigned char myVal = 0x00;

//#define led 8
//#define button 5
//int buttonState = 0;
//int flag = 0;



SoftwareSerial mySerial(2,1); // RX | TX

void setup()
{
//  pinMode(button, INPUT);
//  pinMode(led, OUTPUT);

  // set the pins to LOW
//  digitalWrite(led, LOW);

    mySerial.begin(38400);
    pinMode(pot, INPUT);
    pinMode(bt_power, OUTPUT);
    
    digitalWrite(bt_power, LOW);
}

void loop()
{
  delay(20000); // bluetooth should be off while slave 1 is on
  delay(10000);
  delay(20000); // reset time
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
  
  delay(10000);
  
 /* buttonState = digitalRead(button);
  if(buttonState == HIGH){
    flag = 2;
    digitalWrite(led, HIGH);
  }
  else{
    flag = 0;
    digitalWrite(led, LOW);
  }
  mySerial.println(flag);
  delay(10000);*/

  digitalWrite(bt_power, LOW);
  delay(20000); // reset
  delay(20000); // off for slave 3
  delay(10000);
  delay(20000); // reset
}
