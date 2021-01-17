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
                 (3) create water gun function/SM
                 (4) Alter ultrasonic sensor code to cap distance if no value
                     is returned within a certain time 

*/

/* include libraries */
#include <Servo.h>
#include <Snooze.h>
#include <Wire.h>
#include <SoftwareSerial.h>

/* define macros */
#define pinA0         0     // analog  (pin 14)
#define RxPin         0     // pin 0 (digital 0)
#define TxPin         1     // pin 1 (digital 1)
#define trigPin       2     // trigger pin (pin 2)
#define echoPin       3     // echo pin (pin 3)
#define servoPin      9     // pin 9
#define pin10_PWM     10    // pin 10 (PWM)
#define ledPin        13    // pin 13 (built-in LED)
#define buttonPin     15    // pin 15 (analog 1) 
#define i2cPinSCL     16    // pin 16 (SCL0)
#define i2cPinSDA     17    // pin 17 (SDA0)

#define NUM_PLANTS    3     // number of plants to water
#define ARR_SIZE      7     // size of look up table (LUT)
#define TASKS_NUM     6     // number of tasks
#define BAUD_RATE     9600  // baud rate for serial
#define BT_BAUD_RATE  9600  // baud rate for BT (HC-06 is 9600 by default)

Servo myServo ;
SoftwareSerial BT(RxPin, TxPin) ;

//char getChar ;    /* get char from BT device */
//unsigned char str[50] = { 0 };    /* char array for string */

typedef enum {False, True} Bool ;    // 0 - false, 1 - true
typedef enum {Plant_0, Plant_1, Plant_2} PLANT_NUM ;    // number of plants to water

typedef struct PlantData{
  PLANT_NUM plantNum ;    // plant number
  char data[3] ; // humidity reading from plant via BT
  int sumOfData ;    // sum of plant watering data from data array
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

/* declare global variables */
Bool buttonPressed_g ;    // 0 - not pressed, 1 - pressed
Bool turnOnGun = False ;  // 0 - gun is turned off, 1 - gun is turned on
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
  char arr[] = {} ;  // create empty array buffer
  
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
      delay(500) ;
      digitalWrite(ledPin, HIGH) ; /* light up built-in LED if in state */
      delay(500) ;

      i = 0 ;
      //Serial.print("DEBUG STATEMENT: BT.read(): ") ;
      /* get string from BT buffer */
      while(numBytes > 0){  // count chars from buffer
          arr[i] = BT.read() ;
          Serial.print(arr[i]) ;  // debug statement, print to serial
          BT.print(arr[i]) ;      // debug statement, print to BT terminal
          i++ ;
          numBytes-- ;
      }
      delay(20) ;   // 2000 [ms] --> 20 [ms]
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
      /*
      Serial.print("DEBUG STATEMENT: distanceArrEMA ") ;
      for(i = 0; i < sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0]); i++){
        Serial.print(distanceArrEMA[i]) ;  Serial.print(" ") ;
      }
      */
      // call cut off filter function
      cutOffFilter(distanceArrEMA, maxDist, sizeof(distanceArrEMA)/sizeof(distanceArrEMA[0])) ;
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

  Wire.begin(0x08) ;    /* I2C address */
  Wire.setSDA(i2cPinSDA) ;  /* sets up SDA pin */
  Wire.setSCL(i2cPinSCL) ;  /* sets up SCL pin */
  Wire.onReceive(event) ; /* call function when i2c gets data */
  
  
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
  unsigned char j = 0 ;
  
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

  /* clear trig pin */
  digitalWrite(trigPin, LOW) ;
  /* measure trig pin */
  digitalWrite(trigPin, HIGH) ;
  delayMicroseconds(10) ;
  digitalWrite(trigPin, LOW) ;
  /* calculate distance */
  /* formula: duration * 0.034 / 2 */
  return (distanceConst * pulseIn(echoPin, HIGH))/2000 ;  /* fixed point arithmetic used */
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
      if(arr[i] > maxDist){ // determine if index's value exceeds max distance
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
  return (desiredDist - approxDist) ;  
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

/* end functions */
