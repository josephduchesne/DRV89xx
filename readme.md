# DRV89xx Arduino Library

This library supports operating a DRV89xx ([DRV8912](https://www.ti.com/product/DRV8912-Q1) tested) as a set of full-bridge DC motor drivers.

Although the chip can also drive up to 12 independant half bridges for things like solenoid control, this library only has methods to control pairs of half-bridges as full-bridge 2kHz PWM DC motor drivers.

In the case of the DRV8912, this means 6x current limited to 1A, with a bunch of handy protections (overvoltage, overcurrent, reverse voltage, overtemp, etc.)

# Example
```C++
#include <DRV89xx.h>

const int chipSelectPin = 5;
const int nFaultPin = 34;  // optional, leave 0 if not connected
const int nSleepPin = 14;  // optional, leave 0 if not connected and hardwired
DRV89xx motor_driver(chipSelectPin, nFaultPin, nSleepPin);

void setup() {
  motor_driver.configMotor(0, 1, 2, 0);  // set up motor 0 to use half bridges 1,2 and pwm channel 0
  motor_driver.begin(); // init motor driver
}

void loop() {
    const byte speed=0;
    motor_driver.setMotor(0, speed++, DRV89xx_FORWARD);   // ramp up to full speed, then wrap around
    motor_driver.updateConfig();  // send configuration values to driver (for all motors)
    delay(25);                    // wait for a 25ms
}

```

# Supported Platforms
This has only been tested on an ESP32, however it may work on other arduino platforms.
Likewise this has only been tested with the DRV8912. 
The DRV8910 has virtually identical registers and should work as-is, however the lower DRV89xx models may require some additional logic to work properly.

# Version History

* Release 0.9: 
  * Initial release with basic DC motor driver support.
  * Open circuit protection is disabled since it was triggering false positives very frequently during testing with an N20 motor

# License

[![License](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)
