
#include "MyWifiManager.h"

#define TRIGGER_PIN 0
#define RED_LED_PIN 4

void setup(){
   ConfigEsp(RED_LED_PIN) ;
}

void loop() {
    // is configuration portal requested?
  if ((digitalRead(TRIGGER_PIN) == LOW))
  {
    ResetEsp(4) ;
  }
  
  check_status();
}
