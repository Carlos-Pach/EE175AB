//reads from serial and outputs to led

#include <SoftwareSerial.h>

SoftwareSerial mySerial(0,1); //rx | tx
#define ledPin 15
int state = 0;
unsigned char buttonState;
unsigned char i = 0;
char arr[] = {};

void setup() {
  mySerial.begin(38400);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  if(mySerial.available()>0){
    state = mySerial.read();
    //Serial.println(state);
  }

  if (state == '1'){
    digitalWrite(ledPin, HIGH);
    Serial.println('1');
    state = 0;
  }
  else if(state == '0'){
    digitalWrite(ledPin, LOW);
    Serial.println('0');
    state = 0;
  }

 /* buttonState = mySerial.available();

    if (mySerial.available() > 0){
      while(buttonState > 0){  // count chars from buffer
          arr[i] = mySerial.read() ;
          Serial.print(arr[i]) ;  // debug statement, print to serial
          //mySerial.print(arr[i]) ;      // debug statement, print to BT terminal

          if (arr[i] == '1'){
            digitalWrite(ledPin, HIGH);
            state = 0;
          }
          else if(arr[i] == '0'){
            digitalWrite(ledPin, LOW);
            state = 0;
          }

          i++ ;
          buttonState-- ;
      }
    }*/
    
    
}
