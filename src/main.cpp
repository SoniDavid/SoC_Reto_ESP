#include <Arduino.h>


extern "C" { 
#include "EngTrModel.h" 
}


// put function declarations here:
//int myFunction(int, int);

void setup() {
  Serial.begin(115200);
  EngTrModel_initialize();
  // put your setup code here, to run once:
  //int result = myFunction(2, 3);
}

void loop() {
  EngTrModel_U.Throttle = 50.0;
  EngTrModel_U.BrakeTorque = 0.0;
  EngTrModel_step( );
  Serial.print("Speed: ");
  Serial.println(EngTrModel_Y.VehicleSpeed);
  Serial.print("RPM: ");
  Serial.println(EngTrModel_Y.EngineSpeed);
  Serial.print("Gear: ");
  Serial.println(EngTrModel_Y.Gear);
  Serial.println("------");
  delay(100);
  //for( volatile uint32_t i = 0; i < 200000; i++ );
  // put your main code here, to run repeatedly:
}

// put function definitions here:
//int myFunction(int x, int y) {
//  return x + y;
//}