#include <Servo.h>


/* 

  Goal: Plant priority is pre-computed. From there, we want to check the plant
        numbers from the Pi's camera. Use plantNumSeen_g as an example of what
        number the camera saw. Only water plants that have isWatered == False.
        
        If the number matches with the plant with the highest priority, 
        then water the plant. Once it is watered, change plants[n].isWatered = True. 

        If the number DOES NOT match with the plant with the highest priority,
        then ignore it (by going into SM_wait state?). 

        


  Ideas: Use switch statement for numPlantSeen_g. Since it is between 0-2,
         only 3 cases will be made (plus 1 default that functions as an ignore
         or error). Maybe implement this in the SM_waterPlant state? 

*/
#define DIAGNOSE_LED  (8)   // pin 8
#define SQUIRT_MOTOR  (16)  // pin 16
#define servoPin      (23)  // pin 23

#define BAUD_RATE     (9600)  // baud for serial monitor

Servo myServo ; // create class

typedef enum{False, True} Bool ;
typedef enum{plant_0, plant_1, plant_2} PLANT_NUM ;
typedef enum{low, medium, high} PLANT_PRIORITY ;

typedef struct PlantData{
  PLANT_NUM plantNum ;
  uint16_t data ;
  int desiredVal ;
  PLANT_PRIORITY priority ;
  Bool isWatered ;  
} tPlantData ;

tPlantData plants[3] ;



// change these during testing
Bool isPlant_g = False ;    // determines if a plant was spotted
Bool isObject_g = False ;   // determines if an object was spotted
Bool turnOnGun_g = True ;  // determines whether we can turn on gun or not
Bool checkWater_g = False ; // value from weight sensor code to see if we need to check water amount

// RPi data
unsigned char angleRPi_g = 70 ; // change when testing/debugging
unsigned char minAngle_g = 35 ; // can be changed later during testing
unsigned char maxAngle_g = 165 ;  // can be changed later during testing
PLANT_NUM plantNumSeen_g ;  // plant num read from RPi

int state ;
int n;

enum SM_waterGun{SM_init, SM_wait, SM_checkNumber, SM_waterPlant, SM_shootObject, SM_ERROR} ;
int TickFct_waterGun(state) ;


void testFun(int state){
  // local vars
  static unsigned char cnt = 0 ;
  static unsigned char i = 0 ;
  static unsigned char waterCnt = 0 ;
  static unsigned char errCnt = 0 ;
  static int theta = 90 ;
  
  switch(state){  // state transitions
    case SM_init:
      Serial.println("SM_init") ;
      state = SM_wait ;
      break ;
      
    case SM_wait:
      Serial.println("SM_wait") ;
      if(!turnOnGun_g){ // gun cannot be used
        state = SM_wait ;  
      } else{ // gun is allowed to be used ... see what we are shooting
        if(!isPlant_g && isObject_g){ // we are shooting object
          state = SM_shootObject ;
        } else if(isPlant_g && !isObject_g){  // we are watering plant
          state = SM_waterPlant ;  
        } else{ // go into error for debugging
          state = SM_ERROR ;
        }
      }
      delay(500) ;
      break ;
      
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    case SM_checkNumber:
      Serial.println("SM_checkNumber");
      for (i=0;i<3;i++){  // find index of plantNum == plantNumSeen
        if(plants[i].plantNum == plantNumSeen_g){
          n = i;
          break;
        }
      }
      return n;
      if (plants[n].priority == high){  // check priority
        state = SM_waterPlant;
      }
      else{
        state = SM_wait;
      }
      delay(500);
    break;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    case SM_waterPlant:
      Serial.println("SM_waterPlant") ;
      if(cnt < 5){  // keep watering
        state = SM_waterPlant ;
        cnt++ ;
      } else{
        state = SM_wait ;
        cnt = 0 ;
        waterCnt++ ;  // increase value to read from weight sensor
        Serial.println("theta = 90 deg") ;
      }
      delay(500) ;
      break ;

    case SM_shootObject:
      Serial.println("SM_shootObject") ;
      if(cnt < 5){
        state = SM_shootObject ;  
        cnt++ ;
      } else{
        state = SM_wait ;
        cnt = 0 ;
        waterCnt++ ;
        Serial.println("theta = 90 deg") ;
      }
      break ;
      
    case SM_ERROR:
      Serial.println("SM_ERROR") ;
      for(errCnt = 0; errCnt < 50; errCnt++){ // stay here for debugging
        __asm("nop") ;  
      }
      delay(500) ;
      state = SM_wait ;
      break ;
      
    default:
      state = SM_init ;
      Serial.println("SM default") ;
      break ;  
  }
  switch(state){  // state actions
    case SM_init:
      break ;
      
    case SM_wait:
      Serial.println("theta (wait) = 90 deg") ;
      // if waterCnt is 5 or exceeds 5, check water in its SM
      checkWater_g = (waterCnt == 5 ? True : False) ;
      // reset waterCnt if we need to check water in its SM
      waterCnt = (checkWater_g == True ? 0 : waterCnt) ;
      delay(500) ;
      break ;
      
    case SM_waterPlant:
      Serial.println("theta (waterPlant) = angleRPi_g deg") ;
      for(i = 0; i < 3; i++){ // pretend we are watering the plant
        __asm("nop") ;  
      }
      delay(500) ;
      break ;
      
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    case SM_checkNumber:
      Serial.println("theta (checkNumber) = angleRPi_g deg") ;
      // if plant and priority were correct
        plants[n].isWatered = True;
      // else
    break;
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    case SM_shootObject:
      Serial.println("theta (shootObject) = angleRPi_g deg") ;
      for(i = 0; i < 3; i++){ // pretend we are shooting object
        __asm("nop") ;  
      }
      delay(500) ;
      break ;
      
    case SM_ERROR:
      Serial.println("SM_ERROR") ;
      delay(1000) ;
      
    default:
      break ;
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE) ;
  
  myServo.attach(servoPin) ;

  // init plants
  plants[0].plantNum = 0 ;
  plants[0].data = 600 ;
  plants[0].desiredVal = 700 ;
  plants[0].priority = medium ;
  plants[0].isWatered = False ;

  plants[1].plantNum = 1 ;
  plants[1].data = 500 ;
  plants[1].desiredVal = 400 ;
  plants[1].priority = low ;
  plants[1].isWatered = True ;

  plants[2].plantNum = 2 ;
  plants[2].data = 400 ;
  plants[2].desiredVal = 800 ;
  plants[2].priority = high ;
  plants[2].isWatered = False ;

  // init state
  state = SM_init ;
}

void loop() {
  // put your main code here, to run repeatedly:
  testFun(state) ;
}
