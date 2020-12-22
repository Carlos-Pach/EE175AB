#include <SoftwareSerial.h>

SoftwareSerial mySerial(0, 1); // RX, TX

void setup() {

Serial.begin(9600);

pinMode(15,OUTPUT); digitalWrite(15,HIGH);

Serial.println("Enter AT commands:");

mySerial.begin(38400);

}

void loop()

{

if (mySerial.available())

Serial.write(mySerial.read());

if (Serial.available())

mySerial.write(Serial.read());

}
