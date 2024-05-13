//basic example 
// by Spencer Chen 
// Created @date 2019-11-04
//
#include <Arduino.h>
/*
  BQ25895(M) dynamic test
*/

#include <bq2589x.h>
#include <Wire.h>

unsigned long last_change = 0;
unsigned long now = 0;

bq2589x mycharger;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Start BQ25895(M) dynamic test..."));
  
  Wire.begin();
  mycharger.begin(&Wire);
  delay(10);
}

String vbusType() {
  String s = "?";
    switch (mycharger.get_vbus_type()) {
      case 0: s = "NONE";       break;
      case 1: s = "USB_SDP";    break;
      case 2: s = "CDP(1,5A)";  break;
      case 3: s = "DCP(3,25A)"; break;
      case 4: s = "MAXC";       break;
      case 5: s = "UNKNOWN";    break;
      case 6: s = "NONSTAND";   break;
      case 7: s = "OTG";        break;
    }
  return s;
}

void loop() {
  now = millis();

  if (now - last_change > 5000) {
    last_change = now;

    mycharger.reset_watchdog_timer();

    mycharger.adc_start(false);
    delay(10);

    Serial.print(F(" TYPE:")); Serial.print(vbusType());
    Serial.print(F(" VBUS:")); Serial.print(mycharger.adc_read_vbus_volt());
    Serial.print(F(" BAT:"));  Serial.print(mycharger.adc_read_battery_volt());
    Serial.print(F(" SYS:"));  Serial.print(mycharger.adc_read_sys_volt());
    Serial.print(F(" TS:"));  Serial.print(mycharger.adc_read_temperature());

    Serial.print(F(" Charging:"));
    switch (mycharger.get_charging_status()) {
      case 0: Serial.print(F("Not"));  break;
      case 1: Serial.print(F("Pre"));  break;
      case 2: Serial.print(F("Fast")); break;
      case 3: Serial.print(F("Done")); break;
    }
    
    Serial.print(F(" ChargerCurrent:"));  Serial.print(mycharger.adc_read_charge_current());
    Serial.print(F(" IdmpLimit:"));       Serial.print(mycharger.read_idpm_limit());
    Serial.println();
  }
}
