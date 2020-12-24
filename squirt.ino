// This is the video I saw that uses the laod cell. https://www.youtube.com/watch?v=LIuf2egMioA I also scrolled down and saw a comment to help save the callibrations.
// I tried combining my code to spray the water and the Read_1x_load_cell code in the HX711 library. Since for some reason, I can't edit that without creating a new file.
#include <Servo.h>
#include <HX711_ADC.h>
#include <EEPROM.h>
int motorPin1 = 3;
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAdress = 0;
long t;
int stopsig = 0; // this stopstig will stop the nozzle and tell the car that there is not enough water.
int Servo spin;
int pos = 0;
int sign1 = 0; // This is the signal that determine which plant to water. 
int sign2 = 0;
int sign3 = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = 696.0; // uncomment this if you want to set the calibration value in the sketch
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }
spin.attach(6); // pin 6
pinMode(motorPin1, OUTPUT);
digitalWrite(motorPin1, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      newDataReady = 0;
      t = millis();
      if (i <= x) { //i is the weight of the water container. If weight is less than required amount, then the machine will stop.
        stopsig = 1;
      }
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
  if (stopsig == 0) {
   while (sign1 == 1) {
   spin.write(45); // plant #1 is 45 degrees from the car
   delay (1000)
   digitalWrite(motorPin1, HIGH);
   delay(1000);
   digitalWrite(motorPin1, LOW);
   delay (1000);
   }
   while (sign2 == 1) { //For motorpin1, I'm leaving it at 1000 delay for now, but once we're testing the distance, then I'll change it.
    spin.write(90);
    delay (1000)
   digitalWrite(motorPin1, HIGH);
   delay(1000);
   digitalWrite(motorPin1, LOW);
   delay (1000);
   }
   while (sign3 == 1) {
   spin.write(135);
   delay (1000)
   digitalWrite(motorPin1, HIGH);
   delay(1000);
   digitalWrite(motorPin1, LOW);
   delay (1000);
   }
   spin.write(0);
  }
  else {
    spin.write(0); //I wan to try and reset the position of the nozzle when not in use.
  }
}
