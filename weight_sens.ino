int weight_sens() {
  // This is the video I saw that uses the laod cell. https://www.youtube.com/watch?v=LIuf2egMioA I also scrolled down and saw a comment to help save the callibrations.
  // I tried combining my code to spray the water and the Read_1x_load_cell code in the HX711 library. Since for some reason, I can't edit that without creating a new file.
  #include <HX711_ADC.h>
  #include <EEPROM.h>
  // Any libraries or codes the is needed to be used for this one is put here. What is needed is a signal when the car is at the specified place and a signal that tells when a plant needs watering. Maybe also something else if we still do the pest control.
  const int HX711_dout = 4; //mcu > HX711 dout pin
  const int HX711_sck = 5; //mcu > HX711 sck pin
  //HX711 constructor:
  HX711_ADC LoadCell(HX711_dout, HX711_sck);
  const int calVal_eepromAdress = 0;
  long t;
  int stopsig = 0; // this stopstig will stop the nozzle and tell the car that there is not enough water.





  void setup() {
    // put your setup code here, to run once:
    Serial.begin(57600); delay(10);

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
    }
  }



I believe this is all you need for the function and everything on top would need to be placed in setup or as global variables.

//----------------------------------------------------------------------------------------------------
  // put your main code here, to run repeatedly:
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.println(i);
      newDataReady = 0;
      t = millis();
      if (i >= x) { //i is the weight of the water container. If weight is less than required amount, then the machine will stop. X will be changed when we got a specified value.
        stopsig = 1;
      }
      else {
        stopsig = 0;
      }
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  return stopsig; // stopsig will be the return value to the main file
}
//------------------------------------------------------------------------------------------
