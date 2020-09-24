#include <DRV89xx.h>

const int chipSelectPin = 5;
const int nFaultPin = 34;
const int nSleepPin = 14;
DRV89xx motor_driver(chipSelectPin, nFaultPin, nSleepPin);

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);  // init serial
  pinMode(LED_BUILTIN, OUTPUT);   // initialize digital pin LED_BUILTIN as an output.

  motor_driver.configMotor(0, 1, 5, 0);  // set up motor 0 to use half bridges 1,5 and pwm channel 0
  motor_driver.begin(); // init motor driver
}

// the loop function runs over and over again forever
void loop() {
  
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on
  for (byte i=0;i<255;i++) {
    motor_driver.setMotor(0, i, DRV89xx_FORWARD);   // ramp up speed to full, then wrap around
    motor_driver.updateConfig();  // writes new motor values to driver
    delay(25);                    // wait for a 25ms
  }

  // apply brakes for 1sec
  motor_driver.setMotor(0, 0, DRV89xx_BRAKE);   
  motor_driver.updateConfig();  // writes new motor values to driver
  delay(1000);                    // wait for 1sec
  
  for (byte i=0;i<255;i++) {
    motor_driver.setMotor(0, i, DRV89xx_REVERSE);   // ramp up speed to full, then wrap around
    motor_driver.updateConfig();  // writes new motor values to driver
    delay(25);                    // wait for a 25ms
  }
  //motor_driver.debugConfig();

  // Disable motors and Sleep for 1sec
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off
  motor_driver.disableMotor(0);   // disconnect bridge
  motor_driver.updateConfig();  // writes new motor values to driver
  delay(1000);                    // wait for a 25ms
 
}
