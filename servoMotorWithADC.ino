/* 
    Program: servoMotorWithADC
    
    Objective: The program will control a servo motor with a potentiometer
               to mimic input recieved from the RPi
               
    Purpose: The program will use a servo motor to control the wheels of an RC
             car. The program will also use a linear search method from a look up table
             (LUT) to output the direction in which the wheels will point. A button will mimic
             acceptance/denial of the servo during waiting times.
    Notes/TO DO: (1) Added priority to task struct
                 (2) Test PI controller code
                    (a) Wait for Vivian to finish her portions
                 (3) finish water gun function/SM
                    (a) Finish start-up/calibration code, and water measuring functions
                      (i) Look at github code line 55 to port into SM code
                 (4) Create new filter for ultra sonic sensor to return 1 single value
                    (a) look at HR monitor algo code(?) 
*/

/* include libraries */
#include <Servo.h>
#include <Snooze.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <HX711_ADC.h>
#include <EEPROM.h>

/* define macros */
#define pinA0         0     // analog  (pin 14)
#define RxPin         0     // pin 0 (digital 0)
#define TxPin         1     // pin 1 (digital 1)
#define trigPin       2     // trigger pin (pin 2)
#define echoPin       3     // echo pin (pin 3)
#define HX711_DOUT    4     // HX711 dout pin (pin 4)
#define HX711_SCK     5     // HX711 sck pin (pin 5)
#define servoPin      9     // pin 9
#define pin10_PWM     10    // pin 10 (PWM)
#define ledPin        13    // pin 13 (built-in LED)
#define buttonPin     15    // pin 15 (analog 1) 
#define i2cPinSCL     19    // pin 19 (SCL0)
#define i2cPinSDA     18    // pin 18 (SDA0)

#define NUM_PLANTS    3     // number of plants to water
#define ARR_SIZE      7     // size of look up table (LUT)
#define TASKS_NUM     7     // number of tasks
#define BAUD_RATE     9600  // baud rate for serial
#define BT_BAUD_RATE  9600  // baud rate for BT (HC-06 is 9600 by default)

Servo myServo ;
SoftwareSerial BT(RxPin, TxPin) ;
HX711_ADC LoadCell(HX711_DOUT, HX711_SCK) ;


typedef enum {False, True} Bool ;    // 0 - false, 1 - true
typedef enum {Plant_0, Plant_1, Plant_2} PLANT_NUM ;    // number of plants to water

// change data from char --> uint16_t
// remove sumOfData ... no longer needed
typedef struct PlantData{
  PLANT_NUM plantNum ;    // plant number
  //char data[3] ; // humidity reading from plant via BT
  uint16_t data ;   // humidity reading from plant via BT
  //int sumOfData ;    // sum of plant watering data from data array
  int desiredVal ;   // desired water level for respective plants
  Bool isWatered ;   // determine if plant has already been watered
} tPlantData ;

tPlantData plants[NUM_PLANTS] ; // number of plants to water


/* Task scheduler runs in [us] */
/* create task scheduler */
IntervalTimer myTimer ;

typedef struct task{
  int state ; // current state of task
  unsigned long period ;  // rate at which task should tick
  unsigned long elapsedTime ; // time since last function tick
  int (*TickFct)(int) ; // function to call for task's tick
  Bool isRunning ;  // 1 indicates already running
} task ;

task tasks[TASKS_NUM] ; // number of tasks in struct

const unsigned long taskNum = TASKS_NUM ;
const unsigned long tasksPeriodGCD = 5000/1000 ; // 5 [ms]
const unsigned long periodUltraSonic = 30000/1000; // 30 [ms]
const unsigned long periodBTModule = 25000/1000 ; // 25 [ms] 
const unsigned long periodIfPressed = 50000/1000 ; // 50 [ms]
const unsigned long periodServo = 20000/1000 ;    // 20 [ms] /* SG-90 has a 20 [ms] period; change for different servos */
const unsigned long periodOutputLED = 250000/1000 ; // 250 [ms]
const unsigned long periodRPiData = 125000/1000 ;   // 125 [ms]
const unsigned long periodWaterGun = 75000/1000 ;   // 75 [ms]

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
enum SM_waterGun{SM7_init, SM7_wait, SM7_activateGun, SM7_deactivateGun, SM7_measureWeight} ;
int TickFct_waterGun(int state) ;

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
                                416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800,
                                832, 864, 896, 928, 960, 992, 1023
                              } ;

/* declare global variables */
Bool buttonPressed_g ;    // 0 - not pressed, 1 - pressed
Bool turnOnGun = False ;  // 0 - gun is turned off, 1 - gun is turned on
Bool waterMeasured = True ; // 0 - not measured yet, 1 - already measured
Bool stopSig = True ;    // 0 - 
unsigned int valPWM_g ;
/* values for PID system ... can be changed as testing continues */
const int kp = 500 ;  // kp - proportional value for PID sys  (0.5)
const int ki = 1 ;  // ki - integral value for PID sys (0.001)
const int kd = 3000 ;  // kd - derivative value for PID sys (3.0)



/* declare function prototypes */
void outputPWM(unsigned int val, unsigned char pinNum) ;  /* added pinNum parameter */
uint16_t findNum(uint16_t arr[], unsigned int resistorVal, unsigned int arrSize) ;    // use this function until binary search is ironed out
unsigned long measureDistance(unsigned long *arr) ;
void filterEMA(unsigned long arr[], unsigned long arrEMA[], unsigned char MA) ;   // smooths out data using simple moving average with value K (MA)
void cutOffFilter(unsigned long arr[], unsigned long maxDist, unsigned char n) ;  // function caps array values to max distance
int32_t calculateError(int desiredVal, int approxVal) ;  // function that calculates error for PI sys
int32_t calculateInteg(int32_t errorVal, int32_t integralVal) ;  // function that calculates integral for PI sys
void event(void) ;  // i2c event when getting data
void calibrate(void) ;
uint16_t decodePlantVal(uint16_t arrCompressedData[], uint16_t arrPlantVal[], unsigned char n, unsigned char decodeChar) ;  // returns decoded plant val (in dec)
/* end function prototypes */



/* begin SM functions */

/* State Machine 1 */
int TickFct_checkIfPressed(int state){
  switch(state){ // state transitions
    case SM1_init:
      Serial.println("SM1_init") ;
      state = SM_isPressed ;
      break ;
    case SM_isPressed:
      Serial.println("SM_isPressed") ;
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
      /* If digitalRead returns 0, assign False to buttonPressed_g , otherwise assign True */
      buttonPressed_g = (digitalRead(buttonPin) == 0 ? False : True) ;
      Serial.print("buttonPressed_g: "); Serial.println(buttonPressed_g) ;
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
      Serial.println("SM_onLED") ;
      if(buttonPressed_g){
        state = SM_onLED ;  
      } else{
        state = SM_offLED ;  
      }
      break ;
    case SM_offLED:
      Serial.println("SM_offLED") ;
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
      valPWM_g = analogRead(pinA0) ;
      //Serial.println(valPWM_g) ;  /* test out serial plotter */
      Serial.print("DEBUG STATEMENT: valPWM_g = ") ; Serial.println(valPWM_g) ;
      outputPWM(valPWM_g, pin10_PWM) ; // turn LED on
      break ;
    case SM_offLED:
      Serial.println("DEBUG STATEMENT: offLED action") ;
      outputPWM(0, pin10_PWM) ;           // turn off LED (pin 10)
      break ;
    default:
      outputPWM(0, pin10_PWM) ;     // temporary adding second output statement
      break ;
  } 
  return state ;
}

/* State Machine 3 */
int TickFct_servos(int state){
  int tempVal ; /* using to test PID file (ino) */
  
  switch(state){  // state transitions
    case SM3_init:
      Serial.println("DEBUG STATEMENT: SM3_init") ;
      if(!buttonPressed_g){ /* if button is not pressed */
        state = SM3_turnOffServo ;
      } else{
        state = SM3_turnOnServo ;  
      }
      break ;
    case SM3_turnOffServo:
      if(!buttonPressed_g){ /* if button is not pressed */
        state = SM3_turnOffServo ;  
      } else{
        state = SM3_turnOnServo ;  
      }
      break ;
    case SM3_turnOnServo:
      if(!buttonPressed_g){ /* if button is not pressed */
        state = SM3_turnOffServo ;  
      } else{
        state = SM3_turnOnServo ;  
      }
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
      /* do nothing */
      __asm("nop") ;
      /* test out PID response here */
      break ;
    case SM3_turnOnServo:
      Serial.println("DEBUG STATEMENT: turnOnServo") ;
      /* equation for ADC_max to degrees: ADC_value/180 [degrees], so 
         using fixed point arithmetic, do (ADC_value*10)/(57) to get ~180 (max) */
      myServo.write((int)((10 * valPWM_g)/57)) ; delay(20) ;
      break ;
    default:
      break ;  
  }
  return state ;  
}

/* State Machine 4 */
int TickFct_HC05(int state){
  unsigned char i, numBytes;
  char numOfPlant ;
  static unsigned char plantDecodeID = 0x00 ;
  char arr[] = {} ;  // create empty array buffer
  unsigned char chRecv = 0x00 ;
  const static unsigned char n = sizeof(plantDecoder)/sizeof(plantDecoder[0]) ;

  //const unsigned char testCase = 0x00 ; // basic unit test
  
  switch(state){  // state transitions
    case SM4_init:
      Serial.println("DEBUG STATEMENT: SM4_init") ;
      BT.listen() ;
      state = SM4_wait ;
      break ;
    case SM4_wait:
      Serial.println("DEBUG STATEMENT: SM4_wait") ;
      delay(200) ;
      numBytes = BT.available() ;
      Serial.println("numBytes: "); Serial.println(numBytes) ;
      
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
      delay(500) ;
 
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
      delay(20) ;
      digitalWrite(ledPin, HIGH) ;  /* turn off built-in LED if in state */
      break ;
    case SM4_disconnect:
      break ;
    case SM4_connect:
      digitalWrite(ledPin, LOW) ;
      delay(100) ;
      digitalWrite(ledPin, HIGH) ; /* light up built-in LED if in state */
      delay(100) ;

      //i = 0 ;
      //Serial.print("DEBUG STATEMENT: BT.read(): ") ;
      /* get string from BT buffer */
      #if 0
        while(numBytes > 0){  // count chars from buffer
            arr[i] = BT.read() ;
            Serial.print(arr[i]) ;  // debug statement, print to serial
            BT.print(arr[i]) ;      // debug statement, print to BT terminal
            i++ ;
            numBytes-- ;
        }
      #endif
      
      // get first byte (plant number)
      while(numBytes > 0){
        chRecv = BT.read() ;  // read from buffer
        numBytes-- ;          // decrease count by 1
        
        plantDecodeID = chRecv & 0x03 ;  // find first 2 bits for plant ID
        Serial.print("chRecv = "); Serial.println(chRecv, HEX) ;
        
        switch(plantDecodeID){  // enter plantID info
          case 0x00:
            plants[0].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[0].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;
            
            Serial.println("DEBUG STATEMENT: Plant_0") ;
            Serial.print("Plant_0 isWatered = "); Serial.println(plants[0].isWatered) ;
            Serial.print("Plant_0 data = "); Serial.println(plants[0].data) ;
            break ;
          case 0x01:
            plants[1].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[1].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;
            
            Serial.println("DEBUG STATEMENT: Plant_1") ;
            Serial.print("Plant_1 isWatered = "); Serial.println(plants[1].isWatered) ;
            Serial.print("Plant_1 data = "); Serial.println(plants[1].data) ;
            break ;
          case 0x02:
            plants[2].data = decodePlantVal(plantDecoder, plantVals, n, (chRecv & 0xF8) >> 3) ;
            plants[2].isWatered = (((chRecv & 0x04) >> 2) == 0x01) ? True: False ;
            
            Serial.println("DEBUG STATEMENT: Plant_2") ;
            Serial.print("Plant_2 isWatered = "); Serial.println(plants[2].isWatered) ;
            Serial.print("Plant_2 data = "); Serial.println(plants[2].data) ;
            break ;
          default:
            Serial.println("DEBUG STATEMENT: PLANT_ID NOT FOUND") ;
            digitalWrite(ledPin, LOW) ;
            delay(100) ;
            digitalWrite(ledPin, HIGH) ; // light up built-in LED if in default
            delay(100) ;
            break ;
        }
        delay(3000) ;
      }
      Serial.print("Numbytes: "); Serial.println(numBytes) ;
      delay(200) ;   // 2000 [ms] --> 200 [ms] ... delay so plant controller can catch up
      /* print BT buffer to serial monitor */
      /*
      Serial.print("DEBUG STATEMENT: sizeof arrBT "); Serial.println(sizeof(arr)/sizeof(arr[0])) ;  // outputs 0
      for(i = 0; i < 4; i++){
        Serial.print(arr[i]) ;
        BT.print(arr[i]) ;
      }
      delay(2000) ;
      */
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
  static unsigned long distanceArr[100] = { 0 } ; /* measure distance */
  const unsigned char n = sizeof(distanceArr)/sizeof(distanceArr[0]) ;  /* size of arr */
  static unsigned long distanceArrEMA[n - K_MA + 1] = { 0 } ; /* smoothed out array, 4 is the MA value */ 
  const unsigned char maxDist = 100 ;   /* maximum distance measured in [cm] */
  
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
      Serial.println("DEBUG STATEMENT: SM5_measure") ;
      for(i = 0; i < sizeof(distanceArr)/sizeof(distanceArr[0]) ; i++){ // measure distance function call
        distanceArr[i] = measureDistance(distanceArr) ;
        j++ ;
      }
      break ;
    case SM5_filter:
      Serial.println("DEBUG STATEMENT: SM5_filter") ;
      // call EMA filter function
      filterEMA(distanceArr, distanceArrEMA, K_MA) ;
      
      #if 0
        Serial.print("DEBUG STATEMENT: distanceArrEMA ") ;
        for(i = 0; i < sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0]); i++){
          Serial.print(distanceArrEMA[i]) ;  Serial.print(" ") ;
        }
      #endif
      
      // call cut off filter function
      cutOffFilter(distanceArrEMA, maxDist, sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0])) ;
      // print data array to serial monitor
      #if 0     // change to #if 0 to ignore the printing loop or #if 1 to go thru printing loop
        for(i = 0; i < sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0]); i++){
          Serial.print(distanceArrEMA[i]) ;  Serial.print(" ") ;
        }
      #endif
      
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
  char dataRPi = 0 ;
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
      Wire.requestFrom(0x8, 1) ;
      while(Wire.available()){
          dataRPi = Wire.read() ;
      }
      Serial.println("DEBUG STATEMENT: dataRPi = ") ;
      Serial.print(dataRPi, DEC) ;
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
  
  switch(state){  // state transitions
    case SM7_init:
      state = SM7_wait ;
      break ;
    case SM7_wait:
//      if(turnOnGun == False){ // stay in wait state
//        state = SM7_wait ;  
//      } else if(!(turnOnGun == False)){ // go to activate gun state
//        state = SM7_activateGun ;  
//      }
      // use this statement until weight sensor is finished
      if(1){
        state = SM7_wait ;
      }
      break ;
    case SM7_activateGun:
      if(cnt < 5){  // stay in activate gun state
        state = SM7_activateGun ;  
      } else if(!(cnt < 5)){  // go to deactivateGun state
        state = SM7_deactivateGun ;  
      }
      break ;
    case SM7_deactivateGun:
      if(cnt < 20){       // stay in deactive state
        state = SM7_deactivateGun ;  
      } else if(!(cnt < 20)){ // go to water measuring state
        state = SM7_measureWeight ;  
      }
      break ;
    case SM7_measureWeight:
      if(waterMeasured == False){ // stay in measuring state
        state = SM7_measureWeight ;  
      } else{           // go to wait state
        state = SM7_wait ;  
      }
      break ;
    default:
      state = SM7_init;
      break ;
  }
  switch(state){  // state actions
    case SM7_init:
      waterMeasured = False ;
      cnt = 0 ;
      break ;
    case SM7_wait:
      Serial.println("DEBUG STATEMENT: SM7_wait") ;
      cnt = 0 ;
      break ;
    case SM7_activateGun:
      Serial.println("DEBUG STATEMENT: SM7_activateGun") ;
      // function to squirt the water gun
      
      cnt++ ; // increase cnt
      
      break ;
    case SM7_deactivateGun:
      Serial.println("DEBUG STATEMENT: SM7_deactivateGun") ;
      cnt++ ;
      __asm("nop") ;
    case SM7_measureWeight:
      Serial.println("DEBUG STATEMENT: SM7_measureWeight") ;
      // function to measure water tank capacity
      waterMeasured = True ;
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
  Serial.begin(BAUD_RATE) ;     /* serial monitor set up*/
  pinMode(pin10_PWM, OUTPUT) ; /* potentiometer pin */
  analogWriteResolution(10) ; /* 10 bit PWM */
  pinMode(buttonPin, INPUT) ; /* button pin */

  pinMode(echoPin, INPUT) ;   /* echo pin */
  pinMode(trigPin, OUTPUT) ;  /* trig pin */

  Wire.begin(0x8) ;    /* I2C address */
  Wire.setSDA(i2cPinSDA) ;  /* sets up SDA pin */
  Wire.setSCL(i2cPinSCL) ;  /* sets up SCL pin */
  Wire.onReceive(event) ; /* call function when i2c gets data */

  // comment out until weight sensor is finished
//  /* set up HX711 */ The weight value became hard to change no matter how hard I pressed down.
//  LoadCell.begin() ;  /* load cell set up */
//  float calibrationValue ;  // calibration value
//  calibrationValue = 696.0 ;  // uncomment if calibrationValue is set in file
//#if defined(ESP8266)|| defined(ESP32)
//  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
//#endif
//  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

//  long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
//  boolean tareHX711 = true; //set this to false if you don't want tare to be performed in the next step
//  LoadCell.start(stabilizingtime, tareHX711);
//  LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
//  /* finish HX711 */
  
  myServo.attach(servoPin) ;  // attaches the servo on pin 9 to servo object
  myServo.write(0) ;         // initialize position to 0 degrees
  delay(20) ; /* delay for 20 [ms] to complete period */
  
  // myTimer.begin(TimerISR, tasksPeriodGCD) ; /* task scheduler set up */
  
  pinMode(ledPin, OUTPUT) ;
  pinMode(RxPin, INPUT) ;
  pinMode(TxPin, OUTPUT) ;
  BT.begin(BT_BAUD_RATE) ;
  BT.println("Connection established") ;

  
  /* initialize scheduler */
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

  // initialize plants
  plants[0].plantNum = Plant_0 ;
  plants[0].data = 0 ;
  plants[0].desiredVal = 0.75 * 1023 ;
  plants[0].isWatered = True ;

  plants[1].plantNum = Plant_1 ;
  plants[1].data = 0 ;
  plants[1].desiredVal = 0.5 * 1023 ;
  plants[1].isWatered = True ;

  plants[2].plantNum = Plant_2 ;
  plants[2].data = 0 ;
  plants[2].desiredVal = 0.3 * 1023 ;
  plants[2].isWatered = True ;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  /* Put sleep function here */
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
  unsigned int arrSize = sizeof(arrPWM)/sizeof(arrPWM[0]) ; // gets size of array
  
//  Serial.println("DEBUG STATEMENT: Entering findNum function") ;
  valPWM = findNum(arrPWM, val, arrSize) ;   // arguments for findNum are: (array, inputValue, lowestIndex, highestIndex)
  analogWrite(pinNum, (int)valPWM) ; // pin10_PWM --> pinNum
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
uint16_t decodePlantVal(uint16_t arrCompressedData[], uint16_t arrPlantVal[], unsigned char n, unsigned char decodeChar){
  unsigned int i ;
  Serial.println("DEBUG STATEMENT: decodePlantVal") ;
  delay(2000) ;
  Serial.print("decodeChar(HEX) = "); Serial.println(decodeChar, HEX);
  
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
  TODO: N/A
*/
unsigned long measureDistance(unsigned long *arr){
  const unsigned char distanceConst = 34 ;  /* (duration * 34 / 1000) / 2*/
  const int maxTime = 3100 ;    /* max time for ultrasonic sensor to retrieve data past 100 [cm] */

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
  unsigned long *ptr = arrEMA ;

  Serial.println("DEBUG STATEMENT: filterEMA") ;
//  Serial.print("DEBUG STATEMENT: sizeof arr ") ; Serial.println(sizeof(arr)/sizeof(arr[0])) ;

//  Serial.print("DEBUG STATEMENT: arr[] ") ;
//  for(i = 0; i < MA; i++){
//    Serial.print(arr[i]) ; Serial.print(" ");
//  }
  arrEMA[0] = (arr[0] + arr[1] + arr[2] + arr[3])/4 ;
    
  //Serial.print("DEBUG STATEMENT: arrEMA: "); Serial.println(arrEMA[0]) ;
  //delay(3000) ;
  
  for(i = MA; i < 100; i++){  // start off at MA value as index
    arrEMA[i - 3] = (arr[i] + arr[i - 1] + arr[i -2] + arr[i - 3])/4 ;
  }
  
//  Serial.print("DEBUG STATEMENT: arrEMA = ") ;
//  for(i = 0; i < 97; i++){
//    Serial.print(arrEMA[i]); Serial.print(" ") ;  
//  }
//  delay(5000) ;

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
        //Serial.print("DEBUG STATEMENT: value exceeds max distance ") ;
        //Serial.println(arr[i]) ;
        //delay(1000) ; // 1000 [ms]
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

void event(void){
  Serial.println("DEBUG STATEMENT: i2c event") ;
  digitalWrite(ledPin, LOW) ;
  delay(250) ;
  digitalWrite(ledPin, HIGH) ;
  delay(250) ;
}

/* 
  Function name: calibrate
  Purpose: Sets up weight sensor with known weight
  Details: FINISH LATER
  Parameters:
      FINISH LATER
  Return value: 
    FINISH LATER
  TODO:
    FINISH LATER
*/
void calibrate(void){
  // local vars
  static Bool resumeCal = False ;
  static long t ; // time in [ms]
  float knownMass = 0 ;   // known mass of object ... may change?
  const int intervalTime = 0 ;
  static const float minimumMass ;  // minimum weight for water tank
  
//  while(resumeCal == False){  // get status of tare
//    LoadCell.update() ;
//    if(LoadCell.getTareStatus()){
//      resumeCal = True ;  
//      Serial.println("DEBUG STATEMENT: tare complete") ;
//    }
//  }
//
//  resumeCal = False ;
//
//  while(resumeCal == False){
//    LoadCell.update() ;
//    // TODO: FINISH CODE (github: line 120)  
//  }
//
//  LoadCell.refreshDataSet() ;
//  float newCalibrationData = LoadCell.getNewCalibration(knownMass) ;

  if(LoadCell.update()){
    resumeCal = True ;  
  }
  if(resumeCal == True){
    if(millis() + t > intervalTime){
      float i = LoadCell.getData() ;  // var used to print to serial monitor
      Serial.print("DEBUG STATEMENT: weight = "); Serial.println(i) ; // comment out when no longer needed
      resumeCal = True ;
      t = millis() ;
      if(i > minimumMass){  // measured mass is above minimum water required
        stopSig = True ;    // do not send stop signal 
      } else{
        stopSig = False ;  
      }
    }
  }

  // more to finish (?)
}

/* end functions */
