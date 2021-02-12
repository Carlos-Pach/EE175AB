//checks recived values from the slave bluetooth and determines plant priority based on sensor level
//recives 8-bit data as two bytes in hex
//checks byte with the 4 leftmost bits and bit 3 on the second byte


#include <SoftwareSerial.h>

SoftwareSerial mySerial(0,1); //rx | tx
//#define NUM_PLANTS 3  //number of plants to water
int state = 0;  // var to read bluetooth
int arrayIndex = 0; // hold index for charArray
bool flag = 0;  // flag to indicate when to store into charArray
char charArray[25]; // array for all characters following 
int j = 0;  // var to use as index
int i=0;  // array index
char myChar;
char storeVal[2];
int sortPlants[3];  // array to set plant priority
int plantMask;   // bin 0011 to check the first 2 bits
int plantNum;   // hold value of byte 1
int sensorNum;  // hold value of byte 0
int sensorMask;
int sortSensor[3];  // array for corresponding sensor values
int holdVal;  // hold value before placing into array
int Flag;

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
}

void loop() {
  if(mySerial.available()>0){
    state = mySerial.read();
  }
  storeVal[i] = state;
  i++;

  if (i == 2){    // once the array is filled, clear index, clear array
    i = 0;
    Serial.println(storeVal); // prints recived value
    delay(50);
    
    if (storeVal[1] & 4){
      // set values = to corresponding hex values, otherwise values above 9 are saved as decimal
      if (storeVal[0] == 56){ // hex A
        sensorNum = 8;
      }
      else if(storeVal[0] == 57){
        sensorNum = 9;
      }
      else if (storeVal[0] == 65){ // hex A
        sensorNum = 10;
      }
      else if(storeVal[0] == 66){
        sensorNum = 11;
      }
      else if(storeVal[0] == 67){ 
        sensorNum = 12;
      }
      else if(storeVal[0] == 68){
        sensorNum = 13;
      }
      else if(storeVal[0] == 69){
        sensorNum = 14;
      }
      else if(storeVal[0] == 70){
        sensorNum = 15;
      }
      else{
        sensorNum = storeVal[0];
      }
      
      if (storeVal[1] == 65){ // hex A
        plantNum = 10;
      }
      else if(storeVal[1] == 66){
        plantNum = 11;
      }
      else if(storeVal[1] == 67){ 
        plantNum = 12;
      }
      else if(storeVal[1] == 68){
        plantNum = 13;
      }
      else if(storeVal[1] == 69){
        plantNum = 14;
      }
      else if(storeVal[1] == 70){
        plantNum = 15;
      }
      else{
        plantNum = storeVal[1];
      }
        
      plantMask = 3 & plantNum;
      sensorMask = 8 & plantNum;
      holdVal = sensorNum << 1;   // should hold the val of byte 0 shifted 1

      if (sensorMask == 8){
          holdVal = holdVal | 1;
      }
      else{
        holdVal = holdVal | 0;
      }
      
      for (int a=0; a<3; a++){
        if (sortPlants[a] == plantMask){
          Flag = a;
          break;
        }
        else{
          Flag = 4;
        }
      }

      if (Flag != 4){
          if ((holdVal > 0x11) && (holdVal < 0x1F)){
            sortSensor[Flag] = holdVal;
          }
        }
      else{
          if ((holdVal > 0x11) && (holdVal < 0x1F)){
              sortSensor[2] = holdVal;
              sortPlants[2] = plantMask;
          } 
      }

      
          /*if ((holdVal > 0x11) && (holdVal < 0x1F)){
            sortSensor[2] = holdVal;
            sortPlants[2] = plantMask;
          }*/
      

      for (int j=0; j<2; j++){
        int tempSensor;
        int tempPlant;
        for (int i=0; i<2; i++){
          if (sortSensor[i] < sortSensor[i+1]){
            tempSensor = sortSensor[i];
            sortSensor[i] = sortSensor[i+1];
            sortSensor[i+1] = tempSensor;
  
            tempPlant = sortPlants[i];
            sortPlants[i] = sortPlants[i+1];
            sortPlants[i+1] = tempPlant; 
          }
        }
      }
      for (j = 0; j<2; j++){  // clear array
        storeVal[j] = 0;
      }
      holdVal = 0x00;
    }
    /*else if (~(storeVal[1] & 4)){ // if water level is 0, remove plant from array
        int flag;
        for (int i=0; i<3; i++){
          if (sortPlants[i] == plantMask){
            flag = 1;
            break;
          }
          else{
            flag = 0;
          }
        }
        if (flag == 1){
            sortSensor[i] = 0;
        }
        for (j = 0; j<2; j++){  // clear array
            storeVal[j] = 0;
        }
    }*/

    Serial.println(sortPlants[0]);
    Serial.println(sortPlants[1]);
    Serial.println(sortPlants[2]);
    Serial.println(sortSensor[0], HEX);
    Serial.println(sortSensor[1], HEX);
    Serial.println(sortSensor[2], HEX);
    delay(100);
    
  }  
    
}
