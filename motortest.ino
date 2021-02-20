#include <Servo.h> 
#include <Wire.h>
 
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created 
 
int pos = 0;    // variable to store the servo position 
int c = 0;
void setup() 
{ 
  myservo.attach(3);  // attaches the servo on pin 20 
  Wire.begin(0x8);
  Wire.onReceive(receiveEvent);
} 

void receiveEvent(int howMany) {
  while(Wire.available()){
    int c = Wire.read();
    Serial.print(c); 
    if (pos < c) {   
      for(pos;pos<c;pos++){
        Serial.print(pos);
        myservo.write(pos);
        delay(15);
      }
    }
    if (pos > c) {
      for(pos;pos>c;pos--){
        Serial.print(pos);
        myservo.write(pos);
        delay(15);
      }
    }
    delay(1000);
  }
}

void loop() 
{ 
delay(1000);
} 
