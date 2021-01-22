//reads from serial and outputs to led

#include <SoftwareSerial.h>

SoftwareSerial mySerial(0,1); //rx | tx
//#define ledPin 15
int state = 0;  // var to read bluetooth
int arrayIndex = 0; // hold index for charArray
bool flag = 0;  // flag to indicate when to store into charArray
char mychar = 0;  // var to read dec values into char and print to serial
char charArray[25]; // array for all characters following 
char storeVal[5];  // store digits from charArray to save into variables
int j = 0;  // var to use as index

// plant information
char potVal[5];
char plantNum;
char desiredVal[5];
char isWatered;

void setup() {
  mySerial.begin(38400);
//  pinMode(ledPin, OUTPUT);
}

void loop() {
  if(mySerial.available()>0){
    state = mySerial.read();
  }
  
  if (state == 10){
    for (int i=0; i<20; i++){
      charArray[i] = 0;
    }
    arrayIndex = 0;
    j = 0;
    for(int i=0; i<5; i++){
          storeVal[i] = 0;
    }
  }
  else{
    charArray[arrayIndex] = state;
    arrayIndex++;

    if ((state > 47) && (state < 58)){
      storeVal[j] = state;
      j++;
    }
    for(int i=0; i<arrayIndex+1; i++){
      if (charArray[i] == 35){
        plantNum = storeVal[0];
      }
      if (charArray[i] == 37){
        for (int i=0; i<5; i++){
          potVal[i] = storeVal[i];
        }
      }
      if ((charArray[i] == 108) && (charArray[i+1] == 58)){
        for (int i=0; i<5; i++){
          desiredVal[i] = storeVal[i];
        }
      }
      if ((charArray[i] == 101) && (charArray[i+1] == 58)){
        isWatered = storeVal[0];
      }
    }
  }
    Serial.println(charArray);
    Serial.print("val: ");
    Serial.println(storeVal);
    Serial.print("plant number: ");
    Serial.println(plantNum);
    Serial.print("potentiometer %: ");
    Serial.println(potVal);
    Serial.print("desired water level: ");
    Serial.println(desiredVal);
    Serial.print("water state: ");
    Serial.println(isWatered);
    delay(50);



// ~~~~~~~~~~~~ button test ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  /*if(mySerial.available()>0){
    state = mySerial.read();
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
  }*/
    
}
