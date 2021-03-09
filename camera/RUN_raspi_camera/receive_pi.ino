#include <Wire.h>
#define SLAVE_ADDRESS 0x08
int degrees=0;
//https://dronebotworkshop.com/i2c-arduino-raspberry-pi/
 // https://roboticsbackend.com/raspberry-pi-master-arduino-slave-i2c-communication-with-wiringpi/
//Kelly: Serial.print for debugging, take out later.
//Final variables: name , degrees; for object name and degree for water sprayer
// LED on pin 13
const int ledPin = 13; 
int arr[]={0};
int class_n=0;
char name[13];

void setup() 
{
  Serial.begin(9600);
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
 // Setup pin 13 as output and turn LED off
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {  delay(250); }
 
 void receiveData(int byteCount) { 
   while(Wire.available()) {               //Wire.available() returns the number of bytes available for retrieval with Wire.read(). Or it returns TRUE for values >0.
      for(int i=0; i<byteCount; i++){
        arr[i] = Wire.read();
       Serial.print(arr[i]);
       Serial.print(" ");
       }
       
       Serial.print(" \n");
       if ((arr[0] & 8)==8 ){
         //take info, do conversions, else, ignore
         
          degrees=0;
          degrees= degrees | arr[1]<<4;
          degrees= degrees | arr[2];  
          // degrees= degrees | arr[1]<<8;  //commented out for >3 sets of bits (12) sent thru bus
          // degrees= degrees | arr[2]<<4;  // this was for 4 sets (2 bytes) but the 4th set 
          // degrees= degrees | arr[3];     //and beyond inits to 0 so this can't be used
          
          Serial.print(degrees);
          Serial.print(" ");
          

      Serial.print("= degrees, our coordinate!");
       Serial.print(" \n");
       
        class_n= arr[0] & 7; //bit mask left-most number to keep last 3 bits >>1;
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
             strcpy(name, "raccoon");
             //digitalWrite(ledPin, HIGH);
             //delay(500);
             }
           else if (class_n==6){
             strcpy(name, "person");}
           else if (class_n==7){
             strcpy(name, "potted plant");}
           else { }
           Serial.print("class name: ");
           Serial.print(name);
           Serial.print("\n");
       }
       
   else{
        Serial.print("nothing! nothing seen of interest, COMMENT OUT\n\n")  ;
       }    
     
   }
}



void sendData()
{
  //uneeded until we use to send data to pi?
  //Wire.write(data_to_echo);
  Serial.print("send data to pi \n\n");}

