//reads from serial and outputs to led

#include <SoftwareSerial.h>

SoftwareSerial mySerial(0,1); //rx | tx
//#define ledPin 15
#define NUM_PLANTS 3  //number of plants to water
int state = 0;  // var to read bluetooth
int arrayIndex = 0; // hold index for charArray
bool flag = 0;  // flag to indicate when to store into charArray
char mychar = 0;  // var to read dec values into char and print to serial
char charArray[25]; // array for all characters following 
char storeVal[5];  // store digits from charArray to save into variables
int j = 0;  // var to use as index

// plant information
char data[4];
char plantNum;
char desiredVal[5];
char isWatered;

/*
typedef enum {False, True} Bool ;    // 0 - false, 1 - true
typedef enum {Plant_0, Plant_1, Plant_2} PLANT_NUM ;    // number of plants to water

typedef struct PlantData{
  PLANT_NUM plantNum ;    // plant number
  char data[4] ; // humidity reading from plant via BT
  int sumOfData ;    // sum of plant watering data from data array
  int desiredVal ;   // desired water level for respective plants
  Bool isWatered ;   // determine if plant has already been watered
} tPlantData ;

tPlantData plants[NUM_PLANTS] ; // number of plants to water*/

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
          data[i] = storeVal[i];
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

    Serial.print("plant number: ");
    //mySerial.write(plantNum);
    Serial.println(plantNum);
    Serial.print("potentiometer %: ");
    //mySerial.write(data);
    Serial.println(data);
    Serial.print("desired water level: ");
    //mySerial.write(desiredVal);
    Serial.println(desiredVal);
    Serial.print("water state: ");
    //mySerial.write(isWatered);
    Serial.println(isWatered);
    delay(10);



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
