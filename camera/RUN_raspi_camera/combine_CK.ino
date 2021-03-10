
//https://dronebotworkshop.com/i2c-arduino-raspberry-pi/
// https://roboticsbackend.com/raspberry-pi-master-arduino-slave-i2c-communication-with-wiringpi/
/*
RECEIVE DATA FROM PI: once_flag, obj_num_g, degrees_g (call in outside function ex. printPi)
-two bytes received:
ex.
 1111 1111 | 1111 1111
b15         b7       b0
*/
#include <Wire.h>
#define SLAVE_ADDRESS 0x08

//Kelly: Serial.print for debugging, take out later.
//Final variables: 
//1. once_flag , 2.obj_num_g , 3. degrees_g ;
//1. true means it is the first nondetected instance (will send nothing until it detects again)
//2. object detected number
//3. degree turn for water sprayer (0-180) (from pi, constrained to within 35-165) 


volatile unsigned char buf[4] = "";     //recieves pi info
volatile unsigned int arr[2]; // ={0};   //convert buf array to int() IMP! TO ASSIGN TO EMPTY ARRAY OR COULD GET RANDOM NUMBERS FOR DIFF INDEXES
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

void loop() {  delay(1000); }  //1 sec delay in case of clock stretching or other errors
 
 void receiveData(volatile int byteCount) { 
    volatile unsigned int obj_num_g=0;
    volatile unsigned int degrees_g=0;
    volatile bool once_flag=false;
   while(Wire.available()) {  //Wire.available() returns the number of bytes available for retrieval with Wire.read(). Or it returns TRUE for values >0.
      digitalWrite(ledPin, LOW);
      for(volatile int i=0; i<byteCount; i++){
         buf[i] = Wire.read();
         arr[i]=int(buf[i]);
       }
      
      if ((arr[0] & 0xF0)==0xF0 ){ //check leftmost bits[12:15]
        once_flag=true ; }
        
      //if bit[11]==1, then run script, else skip.
      else if ((arr[0] & 8)==8 ){
        once_flag=false;
        //2. Kelly's Degrees for water sprayer to turn (0-180)
        //bits[0:7]
          degrees_g=arr[1];        
        
    //1. Kelly's Object Number
    //bits[8:10] each number 0-7 corresponds to a hard-coded char string array name: ex. raccoon 
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
      degrees_g, obj_num_g = 0; once_flag=0;}  
    
// @CARLOS: Refer obj_num_g to corresp. name of object commented up above 
  printPi(once_flag, degrees_g, obj_num_g);
  Serial.print("\n");
  } 
}


void printPi( volatile bool& onc_flag, volatile unsigned int& degr, volatile unsigned int& obj_num_g)
{
  //IMP:
  //If true, This is first non obj detected instance, will send nothing afterwards.
  //and will skip to the end and print nothing.
  if (onc_flag==true){
  Serial.print("Once_flag detected: ");
  Serial.print(onc_flag);
  Serial.print("\n");
  }
  
  //Something detected!
  //Prints out only when detected threshold >=65%, and pi's flag is raised stating something is detected 
  else{                 //if (onc_flag==false) 
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
  }
  Serial.print("\n\n");
}


void sendData()
{
  //uneeded until we use to send data to pi?
  //Wire.write(data_to_echo);
  //Serial.print("send data to pi \n\n");
  }

