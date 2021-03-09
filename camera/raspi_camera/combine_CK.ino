
//https://dronebotworkshop.com/i2c-arduino-raspberry-pi/
// https://roboticsbackend.com/raspberry-pi-master-arduino-slave-i2c-communication-with-wiringpi/
/*
RECEIVE DATA FROM PI: obj_num_g, degrees_g, c_dist_g (call in outside function ex. printPi)
-three bytes received:
ex.
1111 1111 | 1111 1111 | 1111 1111
b23        b15         b7       b0
*/
#include <Wire.h>
#define SLAVE_ADDRESS 0x08

//Kelly: Serial.print for debugging, take out later.
//Final variables: 
//1. obj_num_g , 2. degrees_g , 3. c_dist_g;
//1. object detected number 2. degree turn for water sprayer (0-180) 3. Distance from largest red object

volatile unsigned char buf[6] = "";     //recieves pi info
volatile unsigned int arr[3]; // ={0};   //convert buf array to int() IMP! TO ASSIGN TO EMPTY ARRAY OR COULD GET RANDOM NUMBERS FOR DIFF INDEXES
const int ledPin = 13; 

void setup() {
  Serial.begin(9600);
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
// Setup pin 13 as output and turn LED off
 pinMode(ledPin, OUTPUT);
 digitalWrite(ledPin, LOW);
 // Wire.onRequest(sendData);  //if needed to send data to pi
}

void loop() {  delay(1000); }  //250
 
 void receiveData(volatile int byteCount) { 
    volatile unsigned int obj_num_g=0;
    volatile unsigned int degrees_g=0;
    volatile float c_dist_g=0; 
   while(Wire.available()) {  //Wire.available() returns the number of bytes available for retrieval with Wire.read(). Or it returns TRUE for values >0.
      digitalWrite(ledPin, LOW);
      for(volatile int i=0; i<byteCount; i++){
         buf[i] = Wire.read();
         arr[i]=int(buf[i]);
       }
      
      //if bit[19]==1, then run script, else skip.
      if ((arr[0] & 8)==8 ){
        //2. Kelly's Degrees for water sprayer to turn (0-180)
        //bits[8:15]
          degrees_g=arr[1];        
        
    //1. Kelly's Object Number
    //bits[16:18] each number 0-7 corresponds to a hard-coded char string array name: ex. raccoon 
        obj_num_g= arr[0] & 7; 
        
        if (obj_num_g==7){
            digitalWrite(ledPin, HIGH);
            delay(1000);
        }
        
          // if (obj_num_g==0) {
          //   strcpy(name, "zero");}
          // else if (obj_num_g==1){
          //   strcpy(name, "one");}
          // else if (obj_num_g==2){
          //   strcpy(name, "two");}
          // else if (obj_num_g==3){
          //   strcpy(name, "three");}
          // else if (obj_num_g==4){
          //   strcpy(name, "squirrel");}
          // else if (obj_num_g==5){
          //   strcpy(name, "raccoon");}
          // else if (obj_num_g==6){
          //   strcpy(name, "person");}
          // else if (obj_num_g==7){
          //   strcpy(name, "potted plant");}
          // else { } 
      }
  else{  //Serial.print("nothing! Does not detect one of 8 objects, COMMENT OUT\n\n")  ;
      degrees_g, obj_num_g = 0;  }  
      
      //3. Chad's Distance from red object inches, so convert to cm first
      //c_dist_g=0;
      c_dist_g = arr[2];
    
// @CARLOS: Refer obj_num_g to corresp. name of object commented up above 
  printPi(degrees_g, c_dist_g, obj_num_g);
  Serial.print("\n");
  } 
}


void printPi( volatile unsigned int& degr, volatile float& c_di, volatile unsigned int& obj_num_g)
{
  //IMP:
  // for obj_n_g ==0, AND degr==0,==> nothing detected, BUT:
  //if obj_n==0 && degrees==True (exists), then => 'zero' SOMETHING detected)
  Serial.print("detected obj: ");
  // Serial.print(obj_num_g);
          if (obj_num_g==0) {
            Serial.print(obj_num_g);
            Serial.print( " zero");}
          else if (obj_num_g==1){
            Serial.print(obj_num_g);
            Serial.print( " one");}
          else if (obj_num_g==2){
            Serial.print(obj_num_g);
            Serial.print( " two");}
          else if (obj_num_g==3){
            Serial.print(obj_num_g);
            Serial.print( " three");}
          else if (obj_num_g==4){
            Serial.print(obj_num_g);
            Serial.print( " squirrel");}
          else if (obj_num_g==5){
            Serial.print(obj_num_g);
            Serial.print( " raccoon");}
          else if (obj_num_g==6){
            Serial.print(obj_num_g);
            Serial.print( " person");}
          else if (obj_num_g==7){
            Serial.print(obj_num_g);
            Serial.print( " potted plant");}
          else { } 
  Serial.print("\n degrees_g to turn water sprayer: ");
  Serial.print(degr);
  Serial.print("\n Object Avoidence, aka distance from red obj: ");
  Serial.print(c_di);
  Serial.print("\n\n");
}


void sendData()
{
  //uneeded until we use to send data to pi?
  //Wire.write(data_to_echo);
  //Serial.print("send data to pi \n\n");
  }

