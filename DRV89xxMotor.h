/*
  DRV89xx.h - Configures a single brushed DC motor on a DRV89xx
  Currently written and tested for the ESP32 processor
  Created by Joseph Duchesne, September 23, 2020
  Licensed under BSD 3 Clause 
*/
#ifndef DRV89xxMotor_h
#define DRV89xxMotor_h

#include "Arduino.h"
#include "DRV89xxRegister.h"

// from https://stackoverflow.com/a/263738/346227
#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

#define HIGH_SIDE 1

typedef struct DRV89xxHalfBridge { 
    byte id;
    byte enable_register;
    byte pwm_map_register;
    byte pwm_ctrl_register;
    byte bitshift_1;  // used for pwm_ctrl
    byte bitshift_2;  // used for enable, and pwm_map
} DRV89xxHalfBridge;

class DRV89xxMotor
{
  public:
    DRV89xxMotor() : DRV89xxMotor(0, 0, 0, 0) {};
    DRV89xxMotor(byte hb1, byte hb2, byte pwm_channel, byte reverse_delay);

    // This function pre-calculates register IDs and bitshift offsets for the selected half-bridge configuration
    void populateHalfbridgeOffsets(byte offset, byte half_bridge);

     // Apply this motor's current state to the register buffer for the motor driver
    void applyConfig(byte *settings); 

    // Disable this motor
    void disable();
    // Set the speed of this motor
    void set(byte speed, byte direction);
    
  private:
    // Process variables
    int8_t _direction = 0;  // -1 = rev, 0 = brake, 1 = forward
    byte _speed = 0;  // 0 to 255
    bool _enabled = false;  // if disabled, motor is free spinning not braking mode, and PWM is not enabled
    bool _reverse_delay = 0;  // number of milliseconds to brake before reversing direction
    long int _last_forward = 0;  // last time forward was active
    long int _last_reverse = 0;  // last time reverse was active
    

    // configuration
    byte _pwm_channel = 0;
    DRV89xxHalfBridge _half_bridge[2];

    // internal functions
    void setBridgeLowsideDisablePWM(byte *settings, DRV89xxHalfBridge &bridge);
    void setBridgeHSPWM(byte *settings, DRV89xxHalfBridge &bridge);
    void setBridgeOpen(byte *settings, DRV89xxHalfBridge &bridge);
    
    void setPWMFrequency(byte *settings, byte _speed);
};

#endif
