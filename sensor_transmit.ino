#include <SoftwareSerial.h>

SoftwareSerial BTserial(2, 1); // RX | TX
#define led 8
int soilMoistureValue = 0;  //initialize variable to hold analog value

void setup() {
  BTserial.begin(38400);
  pinMode(led,OUTPUT);  //set led pin as output
}

void loop() {
soilMoistureValue = analogRead(A7);  //read analog value from the sensor

//BTserial.println(soilMoistureValue); //sends value to serial
//delay(500);

if (soilMoistureValue < 400){ // water level starts greater than 400. if above 400, level is ok, sends '0'
  BTserial.println('0');
  digitalWrite(led, LOW);
  delay(100);
}
else{ //once the level is too low, send high signal '1'
  BTserial.println('1');
  digitalWrite(led, HIGH);
  delay(100);
}

}
