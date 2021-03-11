#include <SoftwareSerial.h>

#define pot A0
#define ledPin D4 // built in LED

// Using uint16_t to decode compressed plant humidity value
const uint16_t plantEncoder[] = {    0x0,   // 0
                                     0x1,   // 1
                                     0x2,   // 2
                                     0x3,   // 3
                                     0x4,   // 4
                                     0x5,   // 5
                                     0x6,   // 6
                                     0x7,   // 7
                                     0x8,   // 8
                                     0x9,   // 9
                                     0xA,   // 10
                                     0xB,   // 11
                                     0xC,   // 12
                                     0xD,   // 13
                                     0xE,   // 14
                                     0xF,   // 15
                                     0x10,   // 16
                                     0x11,   // 17
                                     0x12,   // 18
                                     0x13,   // 19
                                     0x14,   // 20
                                     0x15,   // 21
                                     0x16,   // 22
                                     0x17,   // 23
                                     0x18,   // 24
                                     0x19,   // 25
                                     0x1A,   // 26
                                     0x1B,   // 27
                                     0x1C,   // 28
                                     0x1D,   // 29
                                     0x1E,   // 30
                                     0x1F,   // 31
                                } ;
const uint16_t plantVals[] = {  32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384,
                                416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 
                                800, 832, 864, 896, 928, 960, 992, 1023
                              } ;

// args: arr[] is the plant value you want to compare
//       hexArr[] is the associated hex value of compressed data
//       n is the size of the arr (both arr[] and hexArr[])
//       val is the original ADC reading
uint16_t linSearch(const uint16_t arr[], const uint16_t hexArr[], uint8_t n, uint16_t val){
  static unsigned char i ;

  for(i = 0; i < n; i++){
    if(val <= plantVals[i]){
      Serial.print("\nval in hex: "); Serial.print(plantEncoder[i], HEX) ;
      return plantEncoder[i] ;
    }  
  }
  Serial.println("ERROR: val not found") ;
  return 0 ;
}


uint16_t potValHex = 0 ;
uint8_t n = sizeof(plantEncoder)/sizeof(plantEncoder[0]) ; // size of both arrs
int potVal = 0;
int plantNum = 0x02 ;
int desiredVal = 600;  // pot reading between 300 and 600
bool isWatered = true;
unsigned char myVal = 0x00;

SoftwareSerial mySerial(D2, D3); // RX | TX

void setup()
{
  Serial.begin(38400) ;
  mySerial.begin(9600);
  pinMode(pot, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // set the pins to LOW
  digitalWrite(ledPin,LOW);
}

void loop()
{
  potVal = analogRead(pot);
  
  if (potVal < desiredVal){
    isWatered = bitWrite(myVal,2,0);
  }
  else{
    isWatered = bitWrite(myVal,2,1);
  }

  potValHex = linSearch(plantEncoder, plantVals, n, potVal) ;
  Serial.print("\npotVal = "); Serial.print(potVal, DEC) ;
  Serial.print("\npotValHex = "); Serial.print(potValHex, DEC) ;
  myVal = myVal | plantNum;
  //myVal = myVal | (potVal/32) << 3;
  myVal = myVal | (potValHex << 3) ;
  mySerial.write(myVal);
  Serial.print("\nmyVal = "); Serial.print(myVal, HEX);
  myVal = 0x00;
  
  digitalWrite(ledPin,HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);

  delay(4000);
}
