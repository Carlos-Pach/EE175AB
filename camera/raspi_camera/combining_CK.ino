/*
RECEIVE DATA FROM PI: NAME, DEGREES_G, C_DIST_G 

PROBLEMS AT line 62: arduino gets correct arr[2], after exiting for loop, and seeing a red object, it becomes a huge number?
PROBLEMS at line  119: not getting stuff to print correctly when I try to call our 3 pi variables as a fuction
*/
//https://dronebotworkshop.com/i2c-arduino-raspberry-pi/
// https://roboticsbackend.com/raspberry-pi-master-arduino-slave-i2c-communication-with-wiringpi/
#include <Wire.h>
#define SLAVE_ADDRESS 0x08

//Kelly: Serial.print for debugging, take out later.
//Final variables: 1. name , 2. degrees_g , 3. c_dist_g;
//1. object detected name 2. degree turn for water sprayer (0-180) 3. Distance from largest red object
unsigned char name[13];
volatile unsigned int degrees_g=0;
volatile unsigned int c_dist_g=0; //Chad's

unsigned char buf[8] = ""; //recieves pi info
unsigned int arr[] ={0};   //convert recieved info to int()
volatile unsigned int class_n=0;


void setup() 
{
  Serial.begin(9600);
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
 // Wire.onRequest(sendData);  //if need to send data to pi
}

void loop() {  delay(1000); }  //250
 
 void receiveData(volatile int byteCount) { 
   while(Wire.available()) {               //Wire.available() returns the number of bytes available for retrieval with Wire.read(). Or it returns TRUE for values >0.
      for(volatile int i=0; i<byteCount; i++){
         buf[i] = Wire.read();
         //Serial.print(buf[2], DEC);
         //Serial.print("  ");
        arr[i]=int(buf[i]);
        // arr[3] = arr[3] +256; }
       Serial.print(arr[i]);
       Serial.print(" ");
       Serial.print(i);
       Serial.print("\n ");
       }
      Serial.print(" \n\n");
     //SOMETHINGS WROG W ARR[2] AFTER CAMERA SEES RED OBJECT, IMPACTS MY DEGREES@arr[2] WHICH IS OUR LSB? SOMEHOW AFTER THE FOR LOOP , BEFORE BELOW
            //        Serial.print(arr[2]);

      
      //if if 1 in the first 4 bits of ex. 1000, then run script, else skip.
      if ((arr[0] & 8)==8 ){
          //2. Degrees for water sprayer to turn (0-180)
          degrees_g=0;
          degrees_g= 0x00 | arr[1]<<4;
          // Serial.print(" @b4fin: ");
          // Serial.print(degrees_g);
          Serial.print(" ored w/ arr[2]:");
          Serial.print(arr[2]);
          Serial.print("   ");
          degrees_g= degrees_g | arr[2];  
          Serial.print(" @CREATEdegrees: ");
          Serial.print(degrees_g);
          
          
         Serial.print(" \n");
         
         
         
      
        
        //on first 4 bits, bit mask last 3 bits, so ex. 0111, each number 0-7 corresponds to a hard-coded char string array name: ex. raccoon 
        class_n= arr[0] & 7; 
        //Serial.print(class_n);
        //Serial.print("= was that class num correct?\n");
          if (class_n==0) {
            strcpy(name, "zero");}
          else if (class_n==1){
            strcpy(name, "one");}
          else if (class_n==2){
            strcpy(name, "two");}
          else if (class_n==3){
            strcpy(name, "three");}
          else if (class_n==4){
            strcpy(name, "squirrel");}
          else if (class_n==5){
            strcpy(name, "raccoon");}
          else if (class_n==6){
            strcpy(name, "person");}
          else if (class_n==7){
            strcpy(name, "potted plant");}
          else { }
          //Serial.print("class name: ");//Serial.print(name);
          //Serial.print("\n");
          
      }
       
  else{ 
        //3. get chad's distance from big red object in cm
        /////////////////////////////////////////////////////////////////////////////////c_dist_g = arr[3]; 
        //Serial.print("nothing! nothing seen of interest, COMMENT OUT\n\n")  ;
      }    
   
      //3. get chad's distance from object in cm
      c_dist_g=0;
      c_dist_g = arr[3]; 
      Serial.print(": chads dist red: ");
      Serial.print(c_dist_g);
      
       Serial.print(" \n");
  
  
  
  // Serial.print(" True distance from red obj: ");
  // Serial.print(c_dist_g);
  
  // @CARLOS: want all 3 below, but it doesn't recieve name array, and degrees gives a weird number.
  //printPi(degrees_g); //, name ,c_dist_g); 
  Serial.print("\n");
     
  }
  
  
}


// void printPi( volatile int& degr) //, volatile int& c_di, char& na,)
// {
  
//   //char n[na.size()] = na;
//   int deg= degr;
//   //int c_d=c_di;
//   // Serial.print("detected obj: ");
//   // Serial.print(na);
//   Serial.print(" degrees_g to turn water sprayer: ");
//   Serial.print(deg);
//   // Serial.print(" Object Avoidence: distance from red obj: ");
//   // Serial.print(c_d);
//   Serial.print("\n\n");
// }


void sendData()
{
  //uneeded until we use to send data to pi?
  //Wire.write(data_to_echo);
  //Serial.print("send data to pi \n\n");
  }

