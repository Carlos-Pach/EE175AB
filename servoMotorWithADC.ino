/* 

    Program: servoMotorWithADC
    
    Objective: The program will control a servo motor with a potentiometer
               to mimic input recieved from the RPi
               
    Purpose: The program will use a servo motor to control the wheels of an RC
             car. The program will also use a linear search method from a look up table
             (LUT) to output the direction in which the wheels will point. A button will mimic
             acceptance/denial of the servo during waiting times.

    Notes/TO DO: (1) Finish servo code
                    (a) Fix voltage source b/c servo is oscillating too much (LPF?)
                 (2) Added priority to task struct
                 (3) Finish BT module code
                    (a) Fix Serial monitor outputting 25 repeatedly
                       (i) Problem with loop (?)
                 (4) fix ino files not being included (e.g. funName not declared in scope)
*/

/* include libraries */
#include <Servo.h>
#include <Snooze.h>
#include <SoftwareSerial.h>
// #include <NewSoftSerial.h>

/* define macros */
#define pinA0         0     // analog  (pin 14)
#define servoPin      9     // pin 9
#define pin10_PWM     10    // pin 10 (PWM)
#define buttonPin     15    // pin 15 (analog 1)
#define RxPin         0     // pin 0 (digital 0)
#define TxPin         1     // pin 1 (digital 1) 

#define ARR_SIZE      7     // size of look up table (LUT)
#define TASKS_NUM     4     // number of tasks
#define BAUD_RATE     38400 // baud rate for serial

Servo myServo ;
SoftwareSerial BT(RxPin, TxPin) ;

char getChar ;    /* get char from BT device */
unsigned char str[50] = { 0 };    /* char array for string */

typedef enum {False, True} Bool;    // 0 - false, 1 - true

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
const unsigned long tasksPeriodGCD = 5000 ; // 5 [ms]
const unsigned long periodBTModule = 25000 ; // 25 [ms] 
const unsigned long periodIfPressed = 50000 ; // 50 [ms]
const unsigned long periodServo = 20000 ;    // 20 [ms] /* SG-90 has a 20 [ms] period; change for different servos */
const unsigned long periodOutputLED = 250000 ; // 250 [ms]

unsigned char runningTasks[3] = {255} ; /* Track running tasks, [0] always idle */
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
unsigned int valPWM_g ;



/* declare function prototypes */
void outputPWM(unsigned int val, unsigned char pinNum) ;  /* added pinNum parameter */
uint16_t findNum(uint16_t arr[], unsigned int resistorVal, unsigned int arrSize) ;    // use this function until binary search is ironed out
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
  unsigned char i ;
  unsigned char numBytes ; 
  
  switch(state){  // state transitions
    case SM4_init:
      Serial.println("DEBUG STATEMENT: SM4_init") ;
      BT.listen() ;
      state = SM4_wait ;
      break ;
    case SM4_wait:
      Serial.println("DEBUG STATEMENT: SM4_wait") ;
      Serial.print("numBytes: "); Serial.println(numBytes) ;
      
      if(numBytes > 0){ /* if buffer has message, read it */
        state = SM4_connect ;  
      } else if(!(numBytes > 0)){ /* if there is no message, wait until one is received */
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
      Serial.print("numBytes: "); Serial.println(numBytes) ;
 
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
  switch(state){  // state actions ;
    case SM4_init:
      numBytes = BT.available() ;
      digitalWrite(LED_BUILTIN, LOW) ;  /* turn off built-in LED if in state */
      break ;
    case SM4_wait:
      numBytes = BT.available() ;
      digitalWrite(LED_BUILTIN, LOW) ;  /* turn off built-in LED if in state */
      break ;
    case SM4_disconnect:
      __asm("nop") ;
      break ;
    case SM4_connect:
      digitalWrite(LED_BUILTIN, HIGH) ; /* light up built-in LED if in state */
      
      for(i = 0; i < numBytes; i++){  /* copy msg from trasmitter */
        str[i] = BT.read() ;
      }
      for(i = 0 ; i < numBytes; i++){ /* echo msg to console and transmitter */
        if(i == (numBytes - 1)){
          Serial.println(str[i]) ;
          BT.write(str[i]) ; BT.write("\n") ;
        } else {
          BT.write(str[i]) ;
        }
      }
      i = 0 ;
      delay(3000) ;  
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
  
  myServo.attach(servoPin) ;  // attaches the servo on pin 9 to servo object
  myServo.write(0) ;         // initialize position to 0 degrees
  delay(20) ; /* delay for 20 [ms] to complete period */
  
  // myTimer.begin(TimerISR, tasksPeriodGCD) ; /* task scheduler set up */

  // mySerial.begin(38400) ;   /* UART serial */
  // mySerial.println("test") ;
  pinMode(LED_BUILTIN, OUTPUT) ;
  BT.begin(BAUD_RATE) ;
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
/* end servo functions */
