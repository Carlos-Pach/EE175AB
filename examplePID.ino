const int desiredPosition = 90 ;
const int lowerBound = 0 ;
const int upperBound = 180 ;

const int kp = 500 ;
const int ki = 1 ;
const int kd = 3000 ;

void controllerPID(void) ;
int calculateError(void) ;
int calculateInteg(void) ;
int calculateDeriv(void) ; 

//void setup() {
//  // put your setup code here, to run once:
//
//}
//
//void loop() {
//  // put your main code here, to run repeatedly:
//
//}

void controllerPID(void){
  /* keep functions as void for now */
//  calculateError(void) ;
//  calculateInteg(void) ;
//  calculateDeriv(void) ;  
}

int calculateError(void){
    return 90 ;
}
