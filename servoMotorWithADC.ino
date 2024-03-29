/* 

    Program: servoMotorWithADC
    
    Objective: The program will control a servo motor with a potentiometer
               to mimic input recieved from the RPi
               
    Purpose: The program will use a servo motor to control the wheels of an RC
             car. The program will also use a linear search method from a look up table
             (LUT) to output the direction in which the wheels will point. A button will mimic
             acceptance/denial of the servo during waiting times.

    Notes/TO DO: (1) I2C works
                    (a) Wait for Kelly and Chad to finish their I2C transmission code
                 (2) Switch motor 1 pwm pin from 6 to 10?

*/

/* include libraries */
#include <Servo.h>
#include <i2c_t3.h>
#include <SoftwareSerial.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

// define macros
#define RxPin         0     // RX1 (pin 0/digital)
#define TxPin         1     // TX1 (pin 1/digital)
#define trigPin       2     // trigger pin (pin 2)
#define echoPin       3     // echo pin (pin 3)
#define HX711_DOUT    4     // HX711 dout pin (pin 4)
#define HX711_SCK     5     // HX711 sck pin (pin 5)
#define DIAGNOSE_LED  8     // startup code LED (pin 8)
#define pin10_PWM     10    // pin 10 (PWM)
#define ledPin        13    // pin 13 (built-in LED)
#define buttonPin     15    // pin 15 (analog 1) 
#define SQUIRT_MOTOR  16     // Motor for squirt gun (pin 16)
#define i2cPinSCL     19    // pin 19 (SCL0)
#define i2cPinSDA     18    // pin 18 (SDA0)
#define MOTOR_2_PWM   20    // PWM motor 2 pin (pin 20)
#define MOTOR_2B_PIN  21    // motor 2B pin (pin 21)
#define MOTOR_2A_PIN  22    // motor 2A pin (pin 22)
#define servoPin      23    // servo (pin 23)
// Pins not used:
// 6-7, 9, 11-12, 14, 16-17


#define NUM_PLANTS    3     // number of plants to water
#define ARR_SIZE      7     // size of look up table (LUT)
#define TASKS_NUM     8     // number of tasks
#define BAUD_RATE     38400  // baud rate for serial
#define BT_BAUD_RATE  38400  // baud rate for BT (HC-06 is 9600 by default) ... HC-05 38400 BAUD

Servo myServo ;
SoftwareSerial BT(RxPin, TxPin) ;
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK) ;


typedef enum {False, True} Bool ;    // 0 - false, 1 - true
typedef enum {Plant_0, Plant_1, Plant_2} PLANT_NUM ;    // number of plants to water
typedef enum {low, medium, high} PLANT_PRIORITY ; // plant priority


typedef struct PlantData{
  PLANT_NUM plantNum ;    // plant number
  uint16_t data ;   // humidity reading from plant via BT
  int desiredVal ;   // desired water level for respective plants
  PLANT_PRIORITY priority ;   // determines priority of plants
  Bool isWatered ;   // determine if plant has already been watered
} tPlantData ;

tPlantData plants[NUM_PLANTS] ; // number of plants to water


typedef struct task{
  int state ; // current state of task
  unsigned long period ;  // rate at which task should tick
  unsigned long elapsedTime ; // time since last function tick
  int (*TickFct)(int) ; // function to call for task's tick
  Bool isRunning ;  // 1/True indicates already running
} task ;

task tasks[TASKS_NUM] ; // number of tasks in struct

const unsigned long taskNum = TASKS_NUM ;
const unsigned long tasksPeriodGCD = 5 ; // 5 [ms]  5000 --> 5
const unsigned long periodUltraSonic = 30; // 30 [ms] 30000 --> 30
const unsigned long periodBTModule = 25 ; // 25 [ms]  25000 --> 25
const unsigned long periodIfPressed = 50 ; // 50 [ms] 50000 --> 50
const unsigned long periodServo = 20 ;    // 20 [ms]  20000 --> 20 ... SG-90 has a 20 [ms] period; change for different servos
const unsigned long periodOutputLED = 150 ; // 150 [ms] 250000 --> 250 --> 150
const unsigned long periodRPiData = 125 ;   // 125 [ms] 125000 --> 125
const unsigned long periodWaterGun = 75 ;   // 75 [ms]  75000 --> 75
const unsigned long periodWeightSensor = 35 ;

unsigned char runningTasks[TASKS_NUM] = {255} ; /* Track running tasks, [0] always idle */
unsigned char idleTask = 255 ;  /* 0 highest priority, 255 lowest */
unsigned char currentTask ; /* index of highest priority task in runningTask */

/* initialize state machines */
enum SM_checkButton{SM1_init, SM_isPressed } ;
int TickFct_checkIfPressed(int state) ;
enum SM_outputLED{SM2_init, SM_offLED, SM_onLED } ;
int TickFct_LEDs(int state) ;
enum SM_outputToServo{SM3_init, SM3_turnOffServo, SM3_turnOnServo } ;
int TickFct_servos(int state) ;
enum SM_bluetooth{SM4_init, SM4_wait, SM4_disconnect, SM4_connect } ;
int TickFct_HC05(int state) ;
enum SM_ultraSonic{SM5_init, SM5_measure, SM5_filter} ;
int TickFct_ultraSonic(int state) ;
enum SM_dataFromRPi{SM6_init, SM6_wait, SM6_getData } ;
int TickFct_dataFromRPi(int state) ;
//enum SM_waterGun{SM7_init, SM7_wait, SM7_activateGun, SM7_deactivateGun, SM7_measureWeight} ;
enum SM_waterGun{SM7_init, SM7_wait, SM7_waterPlant, SM7_shootObject, SM7_ERROR } ;
int TickFct_waterGun(int state) ;
enum SM_weightSensor{SM8_init, SM8_wait, SM8_measureWeight } ;
int TickFct_checkWeightSensor(int state) ;

void TimerISR(void){    /* Scheduler code */
  unsigned char i ;     /* adjust according to number of tasks */
  for(i = 0; i < taskNum; i++){
    if(tasks[i].elapsedTime >= tasks[i].period  /* Task ready */
       && (runningTasks[currentTask] > i)       /* Task priority > current task priority */ 
       && (!tasks[i].isRunning)                 /* Task not already running (no self-preemption) */
       )  {
        tasks[i].elapsedTime = 0 ;  /* Reset time since last running tasks */
        tasks[i].isRunning = True ; /* Mark as task is running */

        currentTask += 1 ;
        runningTasks[currentTask] = i ; /* Add to runningTasks */
        
        tasks[i].state = tasks[i].TickFct(tasks[i].state) ;  /* Execute tick */

        tasks[i].isRunning = False ; /* Mark as not running */
        runningTasks[currentTask] = idleTask ; 
        currentTask -= 1 ;
    }  
    tasks[i].elapsedTime += tasksPeriodGCD ;
  }
}
/* end task scheduler */



/* Using uint16_t because max value of unsigned char is 255 and 10-bit PWM only goes up to 1023 */
const uint16_t arrPWM[ARR_SIZE] = {0, 170, 340, 510, 680, 850, 1023} ;
// Using uint16_t to decode compressed plant humidity value
const uint16_t plantDecoder[] = {    0x0,   // 0
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

// declare global variables
Bool buttonPressed_g = False;    // 0 - not pressed, 1 - pressed
Bool turnOnGun_g = False ;  // 0 - gun is turned off, 1 - gun is turned on
Bool waterMeasured_g = True ; // 0 - not measured yet, 1 - already measured
Bool stopSig_g = False ;    // determines when to stop water nozzle
Bool isPlant_g = False ;    // detects plant from RPi    False --> True (for testing)
Bool isObject_g = False ;   // detects object from RPi   False --> True (for testing)
Bool checkWater_g = False ; // determines if SM should check sensor or not
Bool jumpStart_g = True ;   // give RC car wheels a jump start to prevent stalling
//Bool wateredCurrentPlant_g = False ;  // True when current plant has been watered once
Bool carStops_g = False ;  // False when all data from plants have been gathered
Bool isPlantMEM_g = False ; // holds a true instance of a plant being seen ... reset once plant has been watered
//Bool plantsAlreadySorted_g = False ; // True when all plants have sent data and have been sorted

// data from RPi
//volatile unsigned char distRPi_g ;  // distance calculated by RPi
static unsigned long distTeensy_g = 16 ; // distance calculated by Teensy ... set to 16 in order to not reverse in start up
volatile unsigned char angleRPi_g = 10 ; // angle from RPi (start at 10)
volatile unsigned char prevAngleRPi_g = 10 ; // save previous angle for comparison in water gun SM
volatile unsigned char objectType_g = 0x00 ;  // classify object spotted from Rpi

// values for RC car
unsigned int valPWM_g ; // PWM output value
const int carVeloc_g = 380 ;  // velocity of car (can be changed later)

// values for weight sensor
const int calValAddrEEPROM_g = 0 ;  // EEPROM addr
long t ;  // time elapsed for LoadCell
const float expectedMass_g = 700.0 ;  // expected mass of weight sensor (half full, can be changed later)
const float minMass_g = 10.0 ; // minimum mass needed until refill is necessary (can be changed later)

// values for water gun
const unsigned char minDeg_g = 35 ; // minimum angle of servo motor
const unsigned char maxDeg_g = 155 ;  // maximum angle of servo motor

// i2c vars ... may change
//int degrees = 0 ;

#if 0
// values for PID system ... can be changed as testing continues
const int kp = 500 ;  // kp - proportional value for PID sys  (0.5)
const int ki = 1 ;  // ki - integral value for PID sys (0.001)
const int kd = 3000 ;  // kd - derivative value for PID sys (3.0)
#endif


// declare function prototypes
void outputPWM(unsigned int val, unsigned char pinNum) ;  /* added pinNum parameter */
uint16_t findNum(uint16_t arr[], unsigned int resistorVal, unsigned int arrSize) ;    // use this function until binary search is ironed out
unsigned long measureDistance(unsigned long *arr) ;
void filterEMA(unsigned long arr[], unsigned long arrEMA[], unsigned char MA) ;   // smooths out data using simple moving average with value K
void cutOffFilter(unsigned long arr[], unsigned long maxDist, unsigned char n) ;  // function caps array values to max distance
int32_t calculateError(int desiredVal, int approxVal) ;  // function that calculates error for PI sys
int32_t calculateInteg(int32_t errorVal, int32_t integralVal) ;  // function that calculates integral for PI sys
void event(int byteCount) ;  // i2c event when getting data
void calibration(void) ;
uint16_t decodePlantVal(uint16_t arrCompressedData[], uint16_t arrPlantVal[], uint8_t n, uint8_t decodeChar) ;  // returns decoded plant val (in dec)
void swap(unsigned long *p1, unsigned long *p2) ; // swaps vals between two addresses
void ascendSort(unsigned long arr[], unsigned char n) ; // sort arr in ascending order
void bubbleSortPlant(void) ;
float measureMass(void) ;  // measures current mass of weight sensor
// end function prototypes



/* begin SM functions */

/* State Machine 1 */
int TickFct_checkIfPressed(int state){
  switch(state){ // state transitions
    case SM1_init:
      //Serial.println("SM1_init") ;
      state = SM_isPressed ;
      break ;
    case SM_isPressed:
      //Serial.println("SM_isPressed") ;
      state = SM_isPressed ;
      break ; 
    default:
      Serial.println("DEBUG STATEMENT: in default SM1") ;
      state = SM1_init ;
      break ;
  }
  switch(state){ // state actions
    case SM1_init:
      break ;
    case SM_isPressed:
      // If digitalRead returns 0, assign False to buttonPressed_g , otherwise assign True 
      buttonPressed_g = (digitalRead(buttonPin) == 0 ? False : True) ;
      //Serial.print("buttonPressed_g: "); Serial.println(buttonPressed_g) ;
      break ;
    default:
      break ;
  }
  return state ;
}

/* State Machine 2 */
int TickFct_LEDs(int state){
  switch(state){  // state transitions
    case SM2_init:
      Serial.println("SM2_init") ;
      if(buttonPressed_g){
        state = SM_onLED ;  
      } else{
        state = SM_offLED ;  
      }
      break ;
    case SM_onLED:
      //Serial.println("SM_onLED") ;
      if(buttonPressed_g){
        state = SM_onLED ;  
      } else{
        state = SM_offLED ;  
      }
      break ;
    case SM_offLED:
      //Serial.println("SM_offLED") ;
      //delay(250) ;
      if(buttonPressed_g){
        state = SM_onLED ;  
      } else{
        state = SM_offLED ;  
      }
      break ;
    default:
      Serial.println("DEBUG STATEMENT: in default SM2") ;
      state = SM2_init ;
      break ;
  }
  switch(state){  // state actions
    case SM2_init:
      break ;
    case SM_onLED:
      //valPWM_g = analogRead(pinA0) ;
      //Serial.print("DEBUG STATEMENT: valPWM_g = ") ; Serial.println(valPWM_g) ;
      outputPWM(valPWM_g, pin10_PWM) ; // turn LED on
      break ;
    case SM_offLED:
      //Serial.println("DEBUG STATEMENT: offLED action") ;
      // remove bottom statement so binSearch wont be called every time
      //outputPWM(0, pin10_PWM) ;           // turn off LED (pin 10)
      //valPWM_g = analogRead(pinA0) ;  // update valPWM_g to test motor speed changes
      analogWrite(pin10_PWM, 0) ; // turn off LED (pin 10)
      break ;
    default:
      outputPWM(0, pin10_PWM) ;     // temporary adding second output statement
      break ;
  } 
  return state ;
}

/* State Machine 3 */
int TickFct_servos(int state){
  static unsigned short i = 0 ; ; // used to control motor for a set duration
  static unsigned int waitCnt = 0 ; // used to reverse out of stuck position
  const unsigned int maxWaitCnt = 50 ; // max time to wait in stuck position
  static volatile Bool objectDetected = False ;  // Assume false, True when isPlant_g OR isObject_g is True
  const unsigned short n = 20 ;   // time it takes to go backwards ~3 [s] ... can be changed after testing
  const unsigned short maxN = 500 ; // used when the car is directly in front of a wall
  const unsigned short boostCarVeloc = 500 ;  // when in the stuck state, car needs higher starting velocity to speed up
  static unsigned int stuckInObjDetected = 0 ;
  
  switch(state){  // state transitions
    case SM3_init:
      Serial.println("DEBUG STATEMENT: SM3_init") ;
      state = SM3_turnOffServo ;
      
      // ready up motors
      for(i = 0; i < 50; i++){  // turn on motor 1A and 2B to go forward
        //digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
        digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW) ;
        //analogWrite(MOTOR_2_PWM, carVeloc_g) ; delay(5) ;
        analogWrite(MOTOR_2_PWM, boostCarVeloc); delay(1) ;
      }
      
      jumpStart_g = False ; // motors already turning
      break ;
    case SM3_turnOffServo:  // always in this state
      state = SM3_turnOffServo ;
      break ;
    case SM3_turnOnServo:
      state = SM3_turnOffServo ;
      break ;
    default:
      Serial.println("DEBUG STATEMENT: default case") ;
      state = SM3_init ;
      break ;
  }
  switch (state){ // state actions
    case SM3_init:
      break ;
    case SM3_turnOffServo:
      // determine if an object was detected
      objectDetected = ((isPlant_g == True) || (isObject_g == True)) ? True : False ;
      // determine if an object is straight ahead
      //withinRange = compareDistance() ;

      if(!carStops_g){  // car has received all data from plants
      Serial.println("!carStops") ; delay(5) ;
      #if 1

      switch(objectDetected){
        case True:
          // stop car, object straight ahead

          //comment these out when not testing/debugging
          //Serial.println("objectDetected"); //delay(10) ;
          //Serial.println("Still"); //delay(10) ;
          if((stuckInObjDetected % 20) == 0){ // stuck on plant for too long ... fix count after testing
            
            if(isPlant_g){  // we are stuck on a plant
              for(i = 0; i < 100; i++){ // go forwards
                digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW) ;
                analogWrite(MOTOR_2_PWM, boostCarVeloc) ;  
              }
              for(i = 0; i < 100; i++){
                digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW) ;
                analogWrite(MOTOR_2_PWM, carVeloc_g) ;
              }
              stuckInObjDetected++ ;
            } else if(isObject_g){  // we are stuck on an animal
              for(i = 0; i < 100; i++){ // go backwards
                digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
                analogWrite(MOTOR_2_PWM, boostCarVeloc) ;  
              }
              for(i = 0; i < 100; i++){ // go backwards
                digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
                analogWrite(MOTOR_2_PWM, carVeloc_g) ;
              }
              stuckInObjDetected++ ;
            }
            
          }
          stuckInObjDetected = 0 ;
          
          digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW) ;
          outputPWM(0, MOTOR_2_PWM); //delay(10) ;
          stuckInObjDetected++ ;
          break ; // end case True
        
        case False:
          // object or obstacle not detected, determine distance with teensy sensor
          //Serial.println("!objectDetected");  // comment out when not debugging
          if(distTeensy_g > 50){ // more than 50 [cm] of free space
            //Serial.println("Forwards"); //delay(10) ; // comment out when not debugging
            digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW); delay(5) ;
            analogWrite(MOTOR_2_PWM, carVeloc_g) ;
          } else if(distTeensy_g < 30){ // less than 30 cm from an object/obstacle
            // move backwards
            //Serial.println("Backwards"); //delay(10) ;  // comment out when not debugging
            digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH);
            
            if(distTeensy_g <= 15){ // overshot the acceleration and we are right next to wall ... give a jump start
              //Serial.println("distTeensy <= 15") ;
              for(i = 0; i < n; i++){
                  //analogWrite(boostCarVeloc, MOTOR_2_PWM) ;
                  analogWrite(MOTOR_2_PWM, boostCarVeloc) ;
              }
            }
            if(waitCnt >= maxWaitCnt){  // if we are stuck in backwards state, give car a boost
              //Serial.println("waitCnt exceeded") ;
              for(i = 0; i < n; i++){
                //analogWrite(maxVeloc, MOTOR_2_PWM);
                analogWrite(MOTOR_2_PWM, boostCarVeloc) ;
              }
              waitCnt = 0 ; // reset waitCnt if we entered this portion 
            }
            
            for(i = 0; i < n; i++){ // boost the car for a bit ... change the value around when comparing variable i
              //digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH);
              //analogWrite(carVeloc_g, MOTOR_2_PWM); //delay(5) ;    
              analogWrite(MOTOR_2_PWM, carVeloc_g) ;
            }
            digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH); //delay(10) ;
            analogWrite(MOTOR_2_PWM, carVeloc_g) ;
            waitCnt++ ;
          } else{ // unknown object ahead ... stay still
            //Serial.println("Still") ; // comment out when not debugging
            if(waitCnt >= maxWaitCnt){  // if we stayed still for too long, go backwards for a few seconds
                //Serial.println("waitCnt exceeded"); delay(10) ; // comment out when not debugging
                digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
                for(i = 0; i < (maxN << 1); i++){ // move backwards
                    //digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
                    //analogWrite(maxVeloc, MOTOR_2_PWM) ; //delay(5) ;
                    analogWrite(MOTOR_2_PWM, boostCarVeloc) ;
                }
                waitCnt = 0 ;
            } else{
              outputPWM(0, MOTOR_2_PWM); delay(5) ;  
              digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW); delay(5) ;
            }
            waitCnt++ ;
          }
          break ; // end case False
        
        default:
          Serial.println("DEBUG: motor default case"); delay(10) ;
          digitalWrite(DIAGNOSE_LED, HIGH) ; delay(1000) ;
          digitalWrite(DIAGNOSE_LED, LOW) ; delay(1000) ;
          break ; // end case default
      }
      
      #endif
      }
      else{ // car has not received all data yet
        Serial.println("carStops") ;  delay(5) ;
        digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW) ;
        outputPWM(0, MOTOR_2_PWM) ; 
      }
      
      break ;
    case SM3_turnOnServo:
      Serial.println("DEBUG STATEMENT: turnOnServo") ;
      /* equation for ADC_max to degrees: ADC_value/180 [degrees], so 
         using fixed point arithmetic, do (ADC_value*10)/(57) to get ~180 (max) */
      #if 0
        for(i = 0; i < 100; i++){ // control motor (backwards)
          //digitalWrite(MOTOR_1A_PIN, HIGH); digitalWrite(MOTOR_1B_PIN, LOW) ;
          //myServo.write((int)((10 * valPWM_g)/57)) ; delay(5) ; // controls front motor
          //myServo.write(90) ;
          digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, HIGH) ;
          //outputPWM(valPWM_g, MOTOR_1_PWM) ; delay(5) ;
          outputPWM(valPWM_g, MOTOR_2_PWM) ; delay(5) ;
        } delay(10) ;
        
        for(i = 0; i < 100; i++){  // control motor (forwards)
          //digitalWrite(MOTOR_1A_PIN, LOW); digitalWrite(MOTOR_1B_PIN, HIGH) ;
          //myServo.write((int)((10 * valPWM_g)/57)) ; delay(5) ; // controls front motor
          //myServo.write(90) ;
          //outputPWM(valPWM_g, MOTOR_1_PWM) ; delay(5) ;
  
          digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW) ;
          outputPWM(valPWM_g, MOTOR_2_PWM) ; delay(5) ;
        } delay(10) ;
        
        for(i = 0; i < 250; i++){  // steering direction
          //digitalWrite(MOTOR_1A_PIN, HIGH) ; digitalWrite(MOTOR_1B_PIN, LOW) ;
          digitalWrite(MOTOR_2A_PIN, LOW) ; digitalWrite(MOTOR_2B_PIN, HIGH) ;
          //outputPWM(90, MOTOR_1_PWM) ; delay(10) ;  // 1023 --> 90
          outputPWM(0, MOTOR_2_PWM) ; delay(10) ;
        } delay(10) ;
      #endif

      #if 0
        // control steering
        for(i = 0; i < 10; i++){
          //digitalWrite(MOTOR_1A_PIN, HIGH); digitalWrite(MOTOR_1B_PIN, LOW) ;
          //outputPWM(512, MOTOR_1_PWM); delay(5) ; // 750 --> 900 (goes left)
          digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW) ;
          outputPWM(0, MOTOR_2_PWM); delay(5) ;
        }
        for(i = 0; i < 10; i++){
          //digitalWrite(MOTOR_1A_PIN, LOW); digitalWrite(MOTOR_1B_PIN, HIGH) ;
          //outputPWM(512, MOTOR_1_PWM); delay(5) ; // 750 --> 900 (goes left)
          digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW) ;
          outputPWM(0, MOTOR_2_PWM); delay(5) ;
        }
      #endif
      
      break ;
    default:
      break ;
  }
  return state ;
}

/* State Machine 4 */
int TickFct_HC05(int state){
  unsigned char i, numBytes;
  static unsigned char plantDecodeID = 0x00 ;
  unsigned char chRecv = 0x00 ; // init char for Rx buffer
  const static unsigned char n = sizeof(plantDecoder)/sizeof(plantDecoder[0]) ;

  //const unsigned char testCase = 0x00 ; // basic unit test
  
  switch(state){  // state transitions
    case SM4_init:
      Serial.println("DEBUG STATEMENT: SM4_init") ;
      BT.listen() ;
      state = SM4_wait ;
      break ;
    case SM4_wait:
      //Serial.println("DEBUG STATEMENT: SM4_wait") ;
      delay(20) ;
      numBytes = BT.available() ;
      Serial.print("numBytes: "); Serial.println(numBytes) ;
      
      if(BT.available() > 0){ /* if buffer has message, read it */
        state = SM4_connect ;  
      } else if(!(BT.available() > 0)){ /* if there is no message, wait until one is received */
        state = SM4_wait ;
      } else{
        state = SM4_wait ;  
      }
      break ;
    case SM4_disconnect:
      Serial.println("DEBUG STATEMENT: SM4_disconnect\n") ;
      break ;
    case SM4_connect:
      Serial.println("DEBUG STATEMENT: SM4_connect") ;
      numBytes = BT.available() ;
      Serial.print("numBytes: "); Serial.println(numBytes) ;
      delay(50) ;
 
      if(numBytes > 0){ /* if buffer has message, read it */
        state = SM4_connect ;  
      } else if(!(numBytes > 0)){ /* if there is no message, wait until one is received */
        state = SM4_wait ;
      } else{
        state = SM4_wait ;  
      }
      break ;
    default:
      Serial.println("DEBUG STATEMENT: SM4 default") ;
      state = SM4_init ;
      break ;
  }
  
  switch(state){  // state actions
    case SM4_init:
      numBytes = BT.available() ;
      digitalWrite(ledPin, LOW) ;  /* turn off built-in LED if in state */
      break ;
    case SM4_wait:
      //numBytes = BT.available() ;
      //BT.print("DEBUG STATEMENT: test action\n") ;
      //delay(20) ;
      digitalWrite(ledPin, HIGH) ;  /* turn off built-in LED if in state */
      break ;
    case SM4_disconnect:
      break ;
    case SM4_connect:
      digitalWrite(ledPin, LOW) ;
      //delay(100) ;

      //i = 0 ;
      //Serial.print("DEBUG STATEMENT: BT.read(): ") ;
      // get string from BT buffer
      #if 0
        while(numBytes > 0){  // count chars from buffer
            arr[i] = BT.read() ;
            Serial.print(arr[i]) ;  // debug statement, print to serial
            BT.print(arr[i]) ;      // debug statement, print to BT terminal
            i++ ;
            numBytes-- ;
        }
      #endif
      
      // read from Rx buffer
      while(numBytes > 0){
        chRecv = BT.read() ;  // read from buffer
        // check what hex value we have
        Serial.print("\nchRecv = "); Serial.println(chRecv, HEX) ;
        numBytes-- ;          // decrease count by 1
        
        plantDecodeID = chRecv & 0x03 ;  // find first 2 bits for plant ID
        //Serial.print("chRecv = "); Serial.println(chRecv, HEX) ;  // print char in serial monitor as hex val
        
        switch(plantDecodeID){  // enter plantID info
          case 0x00:  // plant 0
            plants[0].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[0].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;
            
            #if 0
              Serial.println("DEBUG STATEMENT: Plant_0") ;
              Serial.print("Plant_0 isWatered = "); Serial.println(plants[0].isWatered) ;
              Serial.print("Plant_0 data = "); Serial.println(plants[0].data) ;
            #endif
            
            break ;
          case 0x01:  // plant 1
            plants[1].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[1].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;
            
            #if 0
              Serial.println("DEBUG STATEMENT: Plant_1") ;
              Serial.print("Plant_1 isWatered = "); Serial.println(plants[1].isWatered) ;
              Serial.print("Plant_1 data = "); Serial.println(plants[1].data) ;
            #endif
            
            break ;
          case 0x02:  // plant 2
            plants[2].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[2].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;

            #if 0
              Serial.println("DEBUG STATEMENT: Plant_2") ;
              Serial.print("Plant_2 isWatered = "); Serial.println(plants[2].isWatered) ;
              Serial.print("Plant_2 data = "); Serial.println(plants[2].data) ;
            #endif
            
            break ;
          default:  // plant not found
            Serial.println("DEBUG STATEMENT: PLANT_ID NOT FOUND") ;
            //digitalWrite(ledPin, LOW) ;
            delay(100) ;
            digitalWrite(ledPin, HIGH) ; // light up built-in LED if in default
            delay(100) ;
            break ;
        }
      }
      Serial.print("Numbytes: "); Serial.println(numBytes) ;
      //delay(3000) ;   // 2000 [ms] --> 200 [ms] ... delay so plant controller can catch up
      
      /* print BT buffer to serial monitor */
      #if 0
        Serial.print("DEBUG STATEMENT: sizeof arrBT "); Serial.println(sizeof(arr)/sizeof(arr[0])) ;  // outputs 0
        for(i = 0; i < 4; i++){
          Serial.print(arr[i]) ;
          BT.print(arr[i]) ;
        }
        delay(2000) ;
      #endif

      //only enter when all plants have sent data at least once
      if((plants[0].data > 0) && (plants[1].data > 0) && (plants[0].data > 0)){
        // sort plant priority
        bubbleSortPlant() ;
        // start the RC car
        carStops_g = False ;
      } else{
        carStops_g = True ;  
      }
      
      delay(100) ;
      digitalWrite(ledPin, HIGH) ; // light up built-in LED if in state
      break ;
    default:
      break ;
  }
  return state ;  
}

/* State Machine 5 */
int TickFct_ultraSonic(int state){
  unsigned int i = 0 ;  /* fill up distance array */
  const unsigned char K_MA = 4 ;  /* moving average value */
  static unsigned char j = 1 ; /* count until EMA is called */
  
  static unsigned long distanceArr[100] = { 0 } ; /* measure distance */  // 100 --> 200
  const unsigned char n = sizeof(distanceArr)/sizeof(distanceArr[0]) ;  /* size of arr */
  static unsigned long distanceArrEMA[n - K_MA + 1] = { 0 } ; /* smoothed out array, 4 is the MA value */ 
  const unsigned char n_EMA = sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0]) ;  // size of EMA filtered array
  
  const unsigned char maxDist = 150 ;   // maximum desired distance measured in [cm]
  
  switch(state){
    case SM5_init:
      state = SM5_measure ;
      break ;
    case SM5_measure:
      if(j < 100){
        state = SM5_measure ;
      } else{ // j >= 100
        state = SM5_filter ;  
      }
      break ;
    case SM5_filter:
      if(j < 100){
        state = SM5_measure ;  
      } else{
        state = SM5_filter ;
      }
      break ;
    default:
      state = SM5_measure ;
      break ;
  }
  switch(state){
    case SM5_init:
      Serial.println("DEBUG STATEMENT: SM5_init") ;
      break ;
    case SM5_measure:
      //Serial.println("DEBUG STATEMENT: SM5_measure") ;
      for(i = 0; i < n ; i++){ // measure distance function call
        distanceArr[i] = measureDistance(distanceArr) ;
        j++ ;
      }
      break ;
    case SM5_filter:
      
      #if 0 // if statement used to print distanceArr values for analysis
        Serial.println("distanceArr: ") ;
        for(i = 0; i < n; i++){
          Serial.println(distanceArr[i]) ;  
        }
        delay(5000) ;
      #endif
      
      //Serial.println("DEBUG STATEMENT: SM5_filter") ;
      // call EMA filter function
      filterEMA(distanceArr, distanceArrEMA, K_MA) ;
      
      #if 0 // use #if 0 to ignore statement during runtime
        Serial.print("DEBUG STATEMENT: distanceArrEMA ") ;
        for(i = 0; i < n_EMA; i++){  // EMA function call
          Serial.println(distanceArrEMA[i]) ;
        }
        delay(5000) ; // remove after testing in excel
      #endif
      
      // call cut off filter function
      cutOffFilter(distanceArrEMA, maxDist, n_EMA) ;
      
      // print data array to serial monitor
      #if 0     // change to #if 0 to ignore the printing loop or #if 1 to go thru printing loop
        Serial.println("DEBUG STATEMENT: cutOffFilter array") ;
        for(i = 0; i < n_EMA; i++){
          Serial.println(distanceArrEMA[i]) ;
        }
        delay(5000) ; // remove delay after testing data in excel
      #endif

      // call ascending values algo
      ascendSort(distanceArrEMA, n_EMA) ;

      // print ascending array
      #if 0
        Serial.println("DEBUG STATEMENT: ascendSort") ;
        Serial.print("sorted distance = \n") ;
        for(i = 0; i < n_EMA; i++){
          Serial.println(distanceArrEMA[i]) ;  
        }
        delay(5000) ;
      #endif

      // print median value
      #if 0
        Serial.print("DEBUG STATEMENT: sorted arr median = ") ;
        Serial.println(distanceArrEMA[(n_EMA-1)/2]) ;
        delay(5000) ;
      #endif
      distTeensy_g = distanceArrEMA[(n_EMA-1)/2] ;  // return approx distance from teensy
      
      j = 0 ;
      break ;
    default:
      Serial.println("DEBUG STATEMENT: SM5_default") ;
      break ;  
  }  
  return state ;  
}

/* State Machine 6 */
int TickFct_dataFromRPi(int state){
  static unsigned char dataRPi = 0 ;
  
  switch(state){  // state transitions
    case SM6_init:
      state = SM6_wait ;
      break ;
    case SM6_wait:
      state = SM6_wait ;
      break ;
    default:
      state = SM6_init ;
      break ;  
  }
  switch(state){  // state actions
    case SM6_init:
      break ;
    case SM6_wait:
      Serial.println("DEBUG STATEMENT: SM6_wait") ;
      
      while(Wire.available()){  // read I2C data
          dataRPi = Wire.read() ;
      }
      Serial.print("DEBUG STATEMENT: dataRPi = ") ; Serial.println(dataRPi, DEC) ;
      delayMicroseconds(10) ;
      break ;
    default:
      break ;  
  }
  return state ;
}

/* State Machine 7 */
int TickFct_waterGun(int state){
  static unsigned char cnt = 0 ;
  static unsigned char i = 0 ;
  static unsigned int j = 0 ;
  static unsigned int k = 0 ;
  static unsigned char waterCnt = 0 ;
  static int theta = 90 ;
  static unsigned char errCnt = 0 ;

  #if 0   // test variables
    static Bool testTurnOnGun = True ;
  #endif
  
  switch(state){  // state transitions
    case SM7_init:
      state = SM7_wait ;
      
      // test squirt gun and servo motor
      myServo.write(maxDeg_g) ; // write to max angle
      delay(20) ;
      digitalWrite(SQUIRT_MOTOR, LOW) ;
      for(i = 0; i < 10; i++){
          digitalWrite(SQUIRT_MOTOR, HIGH) ;
          delay(20) ;
      }
      digitalWrite(SQUIRT_MOTOR, LOW) ;
      delay(200) ;
      break ;
    case SM7_wait:
      Serial.println("DEBUG STATEMENT: SM7_wait") ;
      
      #if 1
      if(!turnOnGun_g){ // gun cannot be used
        state = SM7_wait ;
        if(isObject_g){
          theta = angleRPi_g;  // angle the gun anyways to obj
        } else if(isPlant_g){
          theta = minDeg_g ;  // angle gun anyways to plant  
        }
      } else{ // gun can be used
        // animal or object takes greater priority
        if(!isPlant_g && isObject_g){ // decide if we shoot object
          if(objectType_g == 0x06){ // if its a person, dont shoot
            state = SM7_wait ;
          } else{ // shoot object
            state = SM7_shootObject ;
            theta = angleRPi_g ;
          }
        } else if(isPlant_g && !isObject_g){  // water plant
          state = SM7_waterPlant ;
          theta = 0 ; // move servo motor
        } else if(isPlant_g && isObject_g){ // plant and object in frame
          if(objectType_g == 0x06){ // if its is a human, dont shoot
            state = SM7_wait ;  
          } else{ // shoot the object
            state = SM7_shootObject ; 
            theta = angleRPi_g ; 
          }
        } else{ // ERROR
          state = SM7_ERROR ;
        }
      }
      #endif
      
      break ;
    case SM7_waterPlant:
      //Serial.println("DEBUG STATEMENT: SM7_waterPlant") ;
      if(cnt < 5){
        state = SM7_waterPlant ;
        cnt++ ;
      } else{
        state = SM7_wait ;
        cnt = 0 ; // reset cnt
        waterCnt++ ;  // increase count to read weight sensor
        theta = 90 ;
        //wateredCurrentPlant_g = True ;  // plant has been watered ... keep driving
      }
      break ;
    case SM7_shootObject:
      //Serial.println("DEBUG STATEMENT: SM7_shootObject") ;
      if(cnt < 5){
        state = SM7_shootObject ;
        cnt++ ;
      } else {
        state = SM7_wait ;
        cnt = 0 ; // reset cnt
        waterCnt++ ;  // increase count to read weight sensor
        theta = 90 ;
      }
      break ;
    case SM7_ERROR: // used for debugging
      //Serial.println("DEBUG STATEMENT: SM7_ERROR") ;
      if(errCnt < 20){
        state = SM7_ERROR ;
      } else{
        state = SM7_wait ;
        digitalWrite(DIAGNOSE_LED, LOW) ;
      }
      break ;
    default:
      Serial.println("DEBUG STATEMENT: SM7 default") ;
      state = SM7_init ;
      break ;
  }
  switch(state){  // state actions
    case SM7_init:
      break ;
    case SM7_wait:
      myServo.write(theta) ;
      checkWater_g = (waterCnt == 200 ? True : False) ; // 5 --> 200
      waterCnt = (checkWater_g == True ? 0 : waterCnt) ;
      digitalWrite(SQUIRT_MOTOR, LOW) ;
      break ;
    case SM7_waterPlant:
      myServo.write(minDeg_g) ;
      for(i = 0; i < 3; i++){  // squirt the gun
        digitalWrite(SQUIRT_MOTOR, HIGH) ;
        delay(10) ;
        for(j = 0; j < 10; j++){
          for(k = 0; k < 50; k++){
            digitalWrite(MOTOR_2A_PIN, HIGH); digitalWrite(MOTOR_2B_PIN, LOW) ;
            analogWrite(MOTOR_2_PWM, 500) ;  delay(1) ;
          }  
        }
        digitalWrite(MOTOR_2A_PIN, LOW); digitalWrite(MOTOR_2B_PIN, LOW) ;
        isPlantMEM_g = False ;
      }
      break ;
    case SM7_shootObject:
      myServo.write(theta) ;
      for(i = 0; i < 3; i++){  // squirt the gun
        digitalWrite(SQUIRT_MOTOR, HIGH) ;
        delay(10) ;
      }
      break ;
    case SM7_ERROR:
      digitalWrite(DIAGNOSE_LED, HIGH) ;
      errCnt++ ;
      break ;
    default:
      state = SM7_init ;
      break ;
  }
  return state ;
}

// State Machine 8
int TickFct_checkWeightSensor(int state){
  // vars here
  const uint16_t minMass = 50 ; // 50 [g]
  const uint16_t expectedMass = 850 ; // ~2 [lbs] or 850 [g]
  static float mass = 0.0 ;

  switch(state){  // state transitions
    case SM8_init:
      calibration() ;
      state = SM8_wait ;
      break ;
    case SM8_wait:
      //Serial.println("DEBUG STATEMENT: SM8_wait") ;
      if(checkWater_g){
        state = SM8_measureWeight ;  
      } else{
        state = SM8_wait ;  
      }
      break ;
    case SM8_measureWeight:
      //Serial.println("DEBUG STATEMENT: SM8_measureWeight") ;
      state = SM8_wait ;
      break ;
    default:
      Serial.println("DEBUG STATEMENT: SM8 default") ;
      state = SM8_init ;
      break ;
  }
  switch(state){  // state actions
    case SM8_init:
      break ;
    case SM8_wait:
      mass = measureMass() ;
      __asm("nop") ;
      break ;
    case SM8_measureWeight:
      mass = measureMass() ;
      break ;
    default:
      break ;  
  }

  return state ;
}
/* end SM functions */



/* main code */
void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE) ;     // serial monitor set up
  pinMode(pin10_PWM, OUTPUT) ;  // potentiometer pin
  analogWriteResolution(10) ;   // 10 bit PWM
  pinMode(buttonPin, INPUT) ;   // button pin
  pinMode(DIAGNOSE_LED, OUTPUT) ;  // LED used for diagnosing code
  pinMode(SQUIRT_MOTOR, OUTPUT) ; // motor pin used for squirt gun

  // light up LED for diagnostics
  digitalWrite(DIAGNOSE_LED, HIGH) ;
  
  // set up motors
  //pinMode(MOTOR_1A_PIN, OUTPUT) ; // controls motor 1A
  //pinMode(MOTOR_1B_PIN, OUTPUT) ; // controls motor 1B
  pinMode(MOTOR_2A_PIN, OUTPUT) ; // controls motor 2A
  pinMode(MOTOR_2B_PIN, OUTPUT) ; // controls motor 2B
  
  //pinMode(MOTOR_1_PWM, OUTPUT) ;  // sets up front motor PWM
  pinMode(MOTOR_2_PWM, OUTPUT) ;  // sets up rear motor PWM
  

  // set up ultrasonic sensor
  pinMode(echoPin, INPUT) ;   // echo pin
  pinMode(trigPin, OUTPUT) ;  // trig pin
  
  
  // set up I2C
  Wire.setSDA(i2cPinSDA) ;  // sets up SDA pin
  Wire.setSCL(i2cPinSCL) ;  // sets up SCL pin
  Wire.begin(0x8) ;    // I2C address
  Wire.onReceive(event) ; // call function when i2c gets data
  
  
  // set up HX711
  Serial.println("Begin loadcell") ; // ------- debug only statement -------
  LoadCell.begin() ;  // load cell set up
  
  // set up vars for HX711
  Bool tareHX711 = True ;  // set to false if tare should not be conducted next step
  float calibrationValue ;  // calibration value
  calibrationValue = 696.0 ;  // uncomment if calibrationValue is set in file
  const unsigned int stabilizingTime = 2000 ; // time to stabilize
  
  EEPROM.get(calValAddrEEPROM_g, calibrationValue) ;  // parameters: addr of EEPROM, calibration value
  Serial.print("EEPROM val = "); Serial.println(calibrationValue) ; // -------- debug only statement --------
  
  Serial.println("Starting loadcell") ;   // -------- debug only statement ------
  LoadCell.start(stabilizingTime, tareHX711) ;  // check if correct pins were used
  Serial.println("Starting calibration") ;  // -------- debug only statement -------
  //LoadCell.setCalFactor(calibrationValue) ; // set calibration
  
  if (LoadCell.getTareTimeoutFlag()) { // check if pins were correct
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations") ;
    //while(1) ;  // stay in loop until error is corrected
  } else {  // set up pins were correct 
    //LoadCell.setCalFactor(calibrationValue); // user set calibration value (float), initial value 1.0 may be used for this sketch
    LoadCell.setCalFactor(calibrationValue) ;
    Serial.println("Startup is complete");
  }
  
  while (!LoadCell.update()); // wait until load cell updates
  // finished HX711
  
  
  // set up servo
  myServo.attach(servoPin) ;  // attaches the servo on pin 9 to servo object
  myServo.write(0) ;         // initialize position to 0 degrees
  delay(500) ; // test servo motor movement
  myServo.write(minDeg_g) ; // test minimum angle
  delay(500) ;
  myServo.write(maxDeg_g) ; // test maximum angle ;
  delay(500) ;
  
  
  // set up BT module pins
  pinMode(ledPin, OUTPUT) ;
  pinMode(RxPin, INPUT) ;
  pinMode(TxPin, OUTPUT) ;
  BT.begin(BT_BAUD_RATE) ;
  BT.println("Connection established") ;

  
  // initialize scheduler
  unsigned char i = 0 ;
  
  tasks[i].state = SM1_init ;
  tasks[i].period = periodIfPressed ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_checkIfPressed ;
  tasks[i].isRunning = False ;
  i++ ;
  
  tasks[i].state = SM2_init ;
  tasks[i].period = periodOutputLED ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_LEDs ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM3_init ;
  tasks[i].period = periodServo ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_servos ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM4_init ;
  tasks[i].period = periodBTModule ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_HC05 ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM5_init ;
  tasks[i].period = periodUltraSonic ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_ultraSonic ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM6_init ;
  tasks[i].period = periodRPiData ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_dataFromRPi ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM7_init ;
  tasks[i].period = periodWaterGun ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_waterGun ;
  tasks[i].isRunning = False ;
  i++ ;

  tasks[i].state = SM8_init ;
  tasks[i].period = periodWeightSensor ;
  tasks[i].elapsedTime = tasks[i].period ;
  tasks[i].TickFct = &TickFct_checkWeightSensor ;
  tasks[i].isRunning = False ;
  i++ ;

  // initialize plants
  plants[0].plantNum = Plant_0 ;
  plants[0].data = 0 ;
  plants[0].desiredVal = 450 ;  // (0.75 * 1023) --> 630 --> 450
  plants[0].isWatered = True ;
  plants[0].priority = low ;

  plants[1].plantNum = Plant_1 ;
  plants[1].data = 0 ;
  plants[1].desiredVal = 500 ; // (0.5 * 1023) --> 500
  plants[1].isWatered = True ;
  plants[1].priority = medium ;

  plants[2].plantNum = Plant_2 ;
  plants[2].data = 0 ;
  plants[2].desiredVal = 600 ; // (0.3 * 1023) --> 700 --> 600
  plants[2].isWatered = True ;
  plants[2].priority = high ;

  // big delay to allow for time to ready script
  delay(5000) ;
  
  // turn off diagnostic LED
  digitalWrite(DIAGNOSE_LED, LOW) ;  

  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  // Put sleep function here
  TimerISR() ;

}
/* end main code */



/* begin servo functions */
/* 
    Function name: outputPWM
    Description: Function acts as intermediate between main() and findNum()
    Purpose: The function is used to get a return value from findNum() and 
             output the value into analogWrite()
    
    Parameters:
              Inputs: val, pinNum
              Outputs: none
    TODO: N/A
*/
void outputPWM(unsigned int val, unsigned char pinNum){
  uint16_t valPWM ;   // gets return value from findNum
  unsigned int arrSize = sizeof(arrPWM)/sizeof(arrPWM[0]) ; // size of array
  
  // Serial.println("DEBUG STATEMENT: Entering findNum function") ;
  valPWM = findNum(arrPWM, val, arrSize) ;   // arguments for findNum are: (array, inputValue, n)
  
  analogWrite(pinNum, (int)valPWM) ;
}

/* 
    Function name: findNum
    Description: Function finds nearest value of an input in a sorted array
    Purpose: The function is used to quickly find and return the nearest value of an input in 
             a pre-made array of sorted values. Linear is used to shorten search time.
    
    Parameters:
             Inputs: arr[], actualVal, low, high
             Outputs: arr[mid]
    TODO: 
          1. Solve binary search algo
            a. When using binary search, function is not called
    Tests:
          1. [Add test notes here. i.e. changes in function type, parameters, etc]
*/
uint16_t findNum(uint16_t arrPWM[], unsigned int actualVal, unsigned int sizeofArr){
  unsigned int i ;
  
  for(i = 0; i < sizeofArr; i++){
    if(actualVal <= arrPWM[i]){
      // Serial.print("DEBUG STATEMENT: arr[i]: "); Serial.println(arrPWM[i]) ;
      return arrPWM[i] ;
    }  
  }
  // if it was not found, return error message
  Serial.println("ERROR: value not found in array") ;
  return 0 ;
}

/* 
  Function name: decodePlantVal
  Purpose: Decodes the plant's moisture sensor data
  Details: Uses 2 arrays to find the encoded data value from the plant.
           Returns the decoded value as a decimal.
  
  Parameters:
        Name                        Details
        [In] arrCompressedData      - encoded array
        [In] arrPlantVal            - decoded lant value
        [In] n                      - size of both arrays
        [In] decodeChar             - value to decode into decimal
  Return value:
        arrPlantVal[index]
*/
uint16_t decodePlantVal(const uint16_t arrCompressedData[], const uint16_t arrPlantVal[], uint8_t n, uint8_t decodeChar){
  unsigned int i ;
  //Serial.println("DEBUG STATEMENT: decodePlantVal") ;
  //delay(2000) ;
  //Serial.print("decodeChar(HEX) = "); Serial.println(decodeChar, HEX);
  
  for(i = 0; i < n; i++){
    if(decodeChar == arrCompressedData[i]){
      Serial.print("DEBUG STATEMENT: arrPlantVal = "); Serial.println(arrPlantVal[i]) ;
      return arrPlantVal[i] ;
    }  
  }
  Serial.println("DEBUG STATEMENT: value not found (decodePlantVal)") ;
  return 0 ;
}

/* 
  Function name: measureDistance
  Purpose: measures distance from HC-SR04 ultrasonic sensor
  Details: Calculates distance between an object and the 
           ultrasonic sensor. 
  
  Parameters:
      Name              Details
      [In] *arr         - fills an array with distance values
  Notes: 
    equation to find maxTime: x = maxDistance * (10^6)/(3.43*10^4) for maxDistance is user decided
*/
unsigned long measureDistance(unsigned long *arr){
  const unsigned char distanceConst = 34 ;  /* (duration * 34 / 1000) / 2*/
  const int maxTime = 4500 ;    /* max time for ultrasonic sensor to retrieve data past 150 [cm] */

  /* clear trig pin */
  digitalWrite(trigPin, LOW) ;
  /* measure trig pin */
  digitalWrite(trigPin, HIGH) ;
  delayMicroseconds(10) ;
  digitalWrite(trigPin, LOW) ;
  /* calculate distance */
  /* formula: duration * 0.034 / 2 */
  return (distanceConst * pulseIn(echoPin, HIGH, maxTime*2))/2000 ;  /* fixed point arithmetic used */
}

/* 
  Function name: filterEMA
  Purpose: Smooths out electrical signals from peripherals
  Details: Uses exponential moving average to smooth out noise in order to
           get more accurate data. N = 4
  
  Parameters:
      Name            Details
      [In] arr[]       - array with filled values
      [Out] arrEMA[]   - filtered EMA array
      [In] MA         - Moving average number

  TODO:
    N/A
*/
void filterEMA(unsigned long arr[], unsigned long arrEMA[], unsigned char MA){
  unsigned char i = 0 ; // count
  //unsigned long *ptr = arrEMA ;

  //Serial.println("DEBUG STATEMENT: filterEMA") ;
//  Serial.print("DEBUG STATEMENT: sizeof arr ") ; Serial.println(sizeof(arr)/sizeof(arr[0])) ;
  
  #if 0
    Serial.print("DEBUG STATEMENT: arr[] ") ;
    for(i = 0; i < MA; i++){
      Serial.print(arr[i]) ; Serial.print(" ");
    }
  #endif
  
  arrEMA[0] = (arr[0] + arr[1] + arr[2] + arr[3])/4 ; // MA4 first calculation
    
  //Serial.print("DEBUG STATEMENT: arrEMA: "); Serial.println(arrEMA[0]) ;
  //delay(3000) ;
  
  for(i = MA; i < 100; i++){  // start off at MA value as index   (100 --> 200)
    arrEMA[i - 3] = (arr[i] + arr[i - 1] + arr[i -2] + arr[i - 3])/4 ;
  }
  
  #if 0
    Serial.print("DEBUG STATEMENT: arrEMA = ") ;
    for(i = 0; i < 97; i++){
      Serial.print(arrEMA[i]); Serial.print(" ") ;  
    }
    delay(5000) ;
  #endif
}

/* 
  Function name: cutOffFilter
  Purpose: Caps array value to maximum distance
  Details: The function iterates through a filtered array to cap distance

  Parameters:
      Name                Details
      [In] arr[]          - address of array's first value
      [In] maxDist        - maximum distance for array
      [in] n              - size of array
  TODO:
    N/A
*/
void cutOffFilter(unsigned long arr[], unsigned long maxDist, unsigned char n){
    unsigned char i ;

    //Serial.println("DEBUG STATEMENT: cutOffFilter") ;
    for(i = 0; i < n; i++){ // begin loop
      if(arr[i] > maxDist || arr[i] == 0){ // determine if index's value exceeds max distance
        arr[i] = maxDist ;  
      }  
    }
}

/* 
  Function name: calculateError
  Purpose: Calculates error value for PID controller 
  Details: Uses a desired distance to calculate error value in PID system

  Parameters:
      Name                  Details
      [In] desiredDist      - desired distance of object
      [In] approxDist       - approximate distance of object

  Return value:
      errorVal
  TODO:
    N/A
*/
int32_t calculateError(int desiredVal, int approxVal){
  return (desiredVal - approxVal) ;  
}

/* 
  Function name: calculateInteg
  Purpose: Calculates integral value for PI controller
  Details: Calculates integral value by using the calculated error and previous
           integral value
  
  Parameters:
        Name                Details
        [In] errorVal       - error value from calculateError
        [In] integralVal    - previous integral value
        [Out] integVal      - sum of error and previous integral val
  
  Return value:
    integralVal
  TODO:
    N/A
*/
int32_t calculateInteg(int32_t errorVal, int32_t integralVal){
  return (integralVal + errorVal) ;
}

/* 
  Function name: event
  Purpose: Uses an interrupt to obtain data from the RPi
  Details: Instead of using the task scheduler, an interrupt is used
           to obtain all information via I2C.

  Parameters:
      Name            Details
      [In] byteCount    - Number of bytes available to read from I2C bus
  
  Return Value:
    None
  TODO:
    Wait for Kelly and Chad to continue working on their portions
*/

void event(int byteCount){
  volatile unsigned char buf[4] = "" ;  // [8] --> [4]
  volatile unsigned int arr[2] ;    // [4] --> [2]
  
  while(Wire.available()) {               //Wire.available() returns the number of bytes available for retrieval with Wire.read(). Or it returns TRUE for values >0.
      for(int i = 0; i < byteCount; i++){ // place read bytes into array
        buf[i] = Wire.read();
        arr[i] = (int)(buf[i]) ;
        //Serial.print(arr[i]);
        //Serial.print(" ");
      }
      //Serial.print(" \n");
       
     if ((arr[0] & 0x08) == 8){
        // get angle
        // check if previous angle is similar to new angle
        if(((prevAngleRPi_g - 5) < arr[1]) && (arr[1] < prevAngleRPi_g + 5)){ // arr[1] is similar to previous angle
          angleRPi_g = prevAngleRPi_g ;
        } else{ // arr[1] is not similar to previous angle
          // store angle into previous angle
          prevAngleRPi_g = angleRPi_g ;
          // give angleRPi_g the new angle
          angleRPi_g = arr[1] ;
        }

        Serial.print("angleRPi_g = "); Serial.println(angleRPi_g) ;

        // check to see if returned degrees goes above/under max/min angle
        if(angleRPi_g > maxDeg_g){
          angleRPi_g = maxDeg_g ;
        } else if(angleRPi_g < minDeg_g){
          angleRPi_g = minDeg_g ;
        }
     
        // objects are: numbers, animals, and human (not plants!)
        if(((arr[0] & 0x07) > 0) && ((arr[0] & 0x07) < 0x07)){  // determine if object is found
          isObject_g = True ;
        } else{
          isObject_g = False ;
        }

        if((arr[0] & 0x07) == 0x07){  // determine if a plant was found
          isPlant_g = True ;
          isPlantMEM_g = True ;
        } else{
          isPlant_g = False ;
        }

        objectType_g = arr[0] & 0x07 ;  // get info of first 3 bits
        
        //class_n = arr[0] & 0x07; //bit mask left-most number to keep last 3 bits >>1;
        //Serial.print(class_n);
        //Serial.print("= was that class num correct?\n");
           #if 0
             if (class_n==0) {  // 0x0  ... 0b0000
               strcpy(name, "zero") ;
             } else if (class_n==1){  // 0x1  ... 0b0001
               strcpy(name, "one") ;
             } else if (class_n==2){  // 0x2  ... 0b0010
               strcpy(name, "two") ;
             } else if (class_n==3){  // 0x3  .. 0b0011
               strcpy(name, "three") ;
             } else if (class_n==4){  // 0x4  ... 0b0100
               strcpy(name, "squirrel") ;
             } else if (class_n==5){  // 0x5  ... 0b0101
               strcpy(name, "raccoon") ;
             } else if (class_n==6){  // 0x6  ... 0b0110
               strcpy(name, "person") ;
             } else if (class_n==7){  // 0x7  ... 0b0111
               strcpy(name, "potted plant") ;
             } else{ 
                __asm("nop") ;
             }
             Serial.print("class name: ");
             Serial.print(name);
             Serial.print("\n");
           #endif
       } else{
         angleRPi_g = 0 ;
         isPlant_g = 0 ;
         isObject_g = 0 ;
       }
   }
   // get distance ... no longer needed
   //distRPi_g = (unsigned char)arr[3] ; // get distance value form I2C
}

/* 
  Function name: calibration
  Purpose: Sets up weight sensor with known weight
  Details: FINISH LATER

  Parameters:
      FINISH LATER

  Return value: 
    FINISH LATER

  TODO:
    FINISH LATER
*/
void calibration(void){
  // local vars
  static Bool newDataReady = False ;  // used to determine if weight sensor is active
  const unsigned char intervalTime = 0 ;
  volatile float massRead = 0 ;   // mass read from weight sensor
  static const float minimumMass = 0 ;  // minimum weight for water tank
  static const float expectedMass = 100 ; // mass needed with water tank

  if(LoadCell.update()){  // check for new data/start next conversion
    newDataReady = True ;
    Serial.println("LoadCell updated") ; 
  } else{
    newDataReady = False ;
    Serial.println("LoadCell not updated") ;
  }

  if(newDataReady == True){
    if(millis() > (t + (float)intervalTime)){
      massRead = LoadCell.getData() ;
      Serial.print("LoadCell data (g) = "); Serial.println(massRead) ;
      newDataReady = False ;
      t = millis() ;
      if(massRead >= minimumMass && massRead < expectedMass){
        stopSig_g = True ;  
      } else{
        stopSig_g = False ;  
      }
    }  
  } else{
    Serial.println("newDataReady is not ready") ;  
  }

  Serial.print("stopSig_g = "); Serial.println(stopSig_g) ;
  Serial.println("Exiting calibration") ;
}

/* 
  Function name: Swap
  Purpose: Swaps values of two address
  Details: Used in sorting algos to swap values using pointers

  Paramters:
    Name                Details
    [In] *p1            - pointer to array index to be swapped
    [In] *p2            - pointer to array index to be swapped
  
  Return value:
    None
*/
void swap(unsigned long *p1, unsigned long *p2){
  //Serial.println("DEBUG STATEMENT: swap") ;
  
  unsigned long temp = *p1 ;
  *p1 = *p2 ;
  *p2 = temp ;
}

/* 
  Function name: ascendSort
  Purpose: Sorts an array with ascending values
  Details: Sorts a non-sorted array from smallest values to greatest

  Parameters:
    Name                  Details
    [In] arr              - array to be sorted
    [In] n                - size of array
    
  Return value:
    None
*/
void ascendSort(unsigned long arr[], unsigned char n){
  //Serial.println("DEBUG STATEMENT: ascendSort") ;
  uint16_t i, j, indx ;

  for(i = 0; i < (n - 1); i++){
    indx = i ;
    
    for(j = (i + 1); j < n; j++){
      if(arr[indx] > arr[j]){  // check if arr[j] is smaller than arr[indx]
          indx = j ;  // indx now has smaller value from arr
      }
      swap(&arr[indx], &arr[i]) ;
    }   
  }
}

/* 
  Function name: bubbleSortPlant
  Purpose: Sorts plant priority for which plant has to be watered first
  Details: Compares the ratio of current plant data and its desired data
           to the next plant being compared

  Parameters:
    None
    
  Return value:
    None
*/
void bubbleSortPlant(void){
  static unsigned char i, j, tempVal ;
  Serial.println("DEBUG STATEMENT: bubbleSort") ;

  //if(!plantsAlreadySorted_g){
    for(i = 0; i < (NUM_PLANTS - 1); i++){
      for(j = 0; j < (NUM_PLANTS - i - 1); j++){
        // change sign as when plants are watered the ADC value decreases (< --> >)
        if(plants[j].data/(float)(plants[j].desiredVal) > plants[j+1].data/(float)(plants[j+1].desiredVal)){  // compare plant data/desiredVal ratio
            tempVal = plants[j].priority ;
            plants[j].priority = plants[j+1].priority ;
            plants[j+1].priority = tempVal ;  
        }  
      }
    }
  //}

  #if 1 // keep at 1 for testing purposes
    for(i = 0; i < NUM_PLANTS; i++){
      Serial.print("Plant: "); Serial.print(plants[i].plantNum) ;
      Serial.print("\nPlant priority: "); Serial.println(plants[i].priority) ;
    }
    delay(5000) ;
  #endif

  //plantsAlreadySorted_g = True ;
}

/* 
  Function name: measureMass
  Purpose: Measures current mass of weight sensor
  Details: Measures mass in order to determine if there is enough water in
           the water tank to continue spraying plants/animals
  
  Parameters: 
    None
  
  Return value:
    massToReturn
  
  TODO:
    N/A
*/
float measureMass(void){
  static Bool newDataReady = False ;
  volatile float massToReturn = 0.00 ;
  
  delay(1000) ; // delay used for waiting for load cell updating
  newDataReady = (LoadCell.update() == 1 ? True : False) ;  // check if LoadCell has updated data
  //Serial.print("newDataReady = "); Serial.println(newDataReady) ; // print result to serial monitor
  
  if(newDataReady){ // read data if it is ready
    massToReturn = LoadCell.getData() ;  
    digitalWrite(DIAGNOSE_LED, LOW) ;
    turnOnGun_g = True ;
    Serial.print("turnOnGun_g = "); Serial.println(turnOnGun_g) ;
  } else{ // otherwise return 0.00
    massToReturn = 0.00 ;
    Serial.println("DEBUG STATEMENT: data not ready") ;
    digitalWrite(DIAGNOSE_LED, HIGH) ;
    turnOnGun_g = False ;
    //Serial.print("turnOnGun = "); Serial.println(turnOnGun_g) ;
    return massToReturn ; // exit function
  }

  if(massToReturn < -1*(expectedMass_g - minMass_g)){ // mass values are expected to be neg. as water is used
    digitalWrite(DIAGNOSE_LED, HIGH) ;
    Serial.println("DEBUG STATEMENT: less than minimum mass") ;
    turnOnGun_g = False ;
    //Serial.print("turnOnGun = "); Serial.println(turnOnGun_g) ;
  } 
  delay(10000) ;  // 10 s delay
  return massToReturn ;
}

/* end functions */
