int motorPin1 = 3;

void setup() {
  // put your setup code here, to run once:
pinMode(motorPin1, OUTPUT);
digitalWrite(motorPin1, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
   digitalWrite(motorPin1, HIGH);
   delay(500);
   digitalWrite(motorPin1, LOW);
}
