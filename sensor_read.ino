#include <SoftwareSerial.h>

SoftwareSerial BTserial(0, 1); // RX | TX
int state = 0;  //set value to read from serial8 port
#define led 15  //set led pin

void setup() {
  BTserial.begin(38400);  //start serial communication at 38400
  pinMode(led,OUTPUT);  //set led pin as output
}

//the leds all work accordingly so it can read the high or low values. serial monitor is blank
//to read from the serial monitor, I've tried changing baud rates and line endings to all the possible choices. both bluetooth devices are set to run at 38400
//double checked the wiring, and made sure the tx and rx are not flipped
//the port works fine when it's not through bluetooth, when using bluetooth I am unable to see what the sent data is
//i tried using print instead of write to see if that would change anything. the code is based off an example I found where values should be able to be seen from both monitors
//i used an led to see what the range of sensor values might be. from the nano, the sensor values should be ranging between 300 to 600. when read, it seems like they're below 50. i tried sending them as decimal to make sure it wouldnt be an issue with whether different values are sent
//https://www.youtube.com/watch?v=uOUSW00n838
//im trying to just test a simple code from the video where it seems he can just read and write to the serial monitor without an issue

void loop() {
  if(BTserial.available()>0){ //check data 
    state = BTserial.read(); // reads the data from serial
    BTserial.write(state);  //this should just write either 0 or 1 to the serial monitor. if it can be written to, we should be able to directly send the sensor value
  }
  if (state == '1'){  //if state is high, moisture level is low. led is on
    digitalWrite(led, HIGH);
    state = 0;
  }
  else if (state == '0'){ //if 0, moisture level is ok
    digitalWrite(led, LOW);
    state = 0;
  }
    
 

}
