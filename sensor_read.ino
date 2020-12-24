#include <SoftwareSerial.h>

SoftwareSerial BTserial(0, 1); // RX | TX
int state = 0;  //set value to read from serial port
#define led 15  //set led pin

void setup() {
  BTserial.begin(38400);  //start serial communication at 38400
  pinMode(led,OUTPUT);  //set led pin as output
}

void loop() {
  if(BTserial.available()>0){ //check data 
    state = BTserial.read(); // reads the data from serial
    BTserial.write(state);
  }
  if (state == '1'){  //read water state and output to led
    digitalWrite(led, HIGH);
    state = 0;
  }
  else if (state == '0'){
    digitalWrite(led, LOW);
    state = 0;
  }
    
 

}
