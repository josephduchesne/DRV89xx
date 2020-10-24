#include "Arduino.h"
#include "DRV89xxMotor.h"

// #define DEBUG_DRV89xx_MOTORS true

DRV89xxMotor::DRV89xxMotor(byte hb1, byte hb2, byte pwm_channel, byte reverse_delay) : _reverse_delay(reverse_delay), _pwm_channel(pwm_channel) {
  // populate the half bridge configurations with register and offset settings
  populateHalfbridgeOffsets(0, hb1);
  populateHalfbridgeOffsets(1, hb2);
}

// This function pre-calculates register IDs and bitshift offsets for the selected half-bridge configuration
void DRV89xxMotor::populateHalfbridgeOffsets(byte offset, byte half_bridge) {
  
  _half_bridge[offset].id = half_bridge;
  if (half_bridge==0) return;
  _half_bridge[offset].enable_register = (byte)DRV89xxRegister::OP_CTRL_1 + (half_bridge - 1) / 4; // cache the register address. HB1-4 are OP_CTRL_1, 5-8 are _2, and 9-12 are _3
  _half_bridge[offset].pwm_map_register = (byte)DRV89xxRegister::PWM_MAP_CTRL_1 + (half_bridge - 1) / 4; // Same as before, 2 bits per PWM map entry
  _half_bridge[offset].pwm_ctrl_register = (byte)DRV89xxRegister::PWM_CTRL_1 + (half_bridge - 1) / 8;  // PWM_CTRL_2 if hb 9+

  // This bitshift is used for PWM ctrl (1 bytes per hb)
  _half_bridge[offset].bitshift_1 = ((half_bridge - 1) % 8); // hb1 = bitshift 0, hb2 = bitshift 1, ..., hb12 = bitshift 4

  // This bitshift is used for PWM map, and ctrl (2 bytes per hb)
  _half_bridge[offset].bitshift_2 = ((half_bridge - 1) % 4) * 2; // hb1 = bitshift 0, hb2 = bitshift 2, ..., hb12 = bitshift 6
}

void DRV89xxMotor::applyConfig(byte *settings) {
  if (_enabled) {
    switch (_direction) { // this section is verbose verbose, but much clearer than the more line-efficient logic would be
      case 1:  // forward
        // Serial.println("Motor forward");
        if (millis()-_last_reverse > _reverse_delay) {
          setBridgeLowsideDisablePWM(settings, _half_bridge[0]); // hb1 LOW, no PWM
          setBridgeHSPWM(settings, _half_bridge[1]); // hb2 PWM, high
          setPWMFrequency(settings, _speed); // set PWM duty cycle
          _last_forward = millis();
        } else {  // brake for _reverse_delay upon direction reversing
          setBridgeLowsideDisablePWM(settings, _half_bridge[0]); // hb1 LOW, no PWM
          setBridgeLowsideDisablePWM(settings, _half_bridge[1]); // hb2 LOW, no PWM
        }
        

        break;
      case -1: // reverse
        //Serial.println("Motor reverse");
        if (millis()-_last_forward > _reverse_delay) {
          setBridgeHSPWM(settings, _half_bridge[0]); // hb1 PWM, high
          setBridgeLowsideDisablePWM(settings, _half_bridge[1]); // hb2 LOW, no PWM
          setPWMFrequency(settings, _speed); // set PWM duty cycle
          _last_reverse = millis();
        } else {  // brake for _reverse_delay upon direction reversing
          setBridgeLowsideDisablePWM(settings, _half_bridge[0]); // hb1 LOW, no PWM
          setBridgeLowsideDisablePWM(settings, _half_bridge[1]); // hb2 LOW, no PWM
        }

        break;

      default: // brake
        //Serial.println("Motor braking");
        setBridgeLowsideDisablePWM(settings, _half_bridge[0]); // hb1 LOW, no PWM
        setBridgeLowsideDisablePWM(settings, _half_bridge[1]); // hb2 LOW, no PWM
        break; // lol
    }
  } else { // motor is disabled, set gates to ZZ (free spinning), disable PWM channel for bridges
    setBridgeOpen(settings, _half_bridge[0]);
    setBridgeOpen(settings, _half_bridge[1]);
    //Serial.println("Motor disabled");
  }
}
void DRV89xxMotor::disable() {
  _enabled = false;
}

void DRV89xxMotor::set(byte speed, byte direction) {
  _enabled = true;
  _speed = speed;
  _direction = direction;
}

void DRV89xxMotor::setBridgeLowsideDisablePWM(byte *settings, DRV89xxHalfBridge &bridge) {
  if (bridge.id==0) return;  // unconfigured

  #ifdef DEBUG_DRV89xx_MOTORS
  Serial.print("HBridge "); Serial.println(bridge.id);
  Serial.print("Brake HBridge L: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" set bit "); Serial.println(bridge.bitshift_2);
  Serial.print("Brake HBridge H: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_2+HIGH_SIDE);
  Serial.print("Brake HBridge PWM: 0x"); Serial.print(bridge.pwm_ctrl_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_1);
  #endif

  BIT_SET(settings[bridge.enable_register], bridge.bitshift_2);  // enable lowside of half bridge
  BIT_CLEAR(settings[bridge.enable_register], bridge.bitshift_2 + HIGH_SIDE); // disable highside of half bridge
  BIT_CLEAR(settings[bridge.pwm_ctrl_register], bridge.bitshift_1);  // disable PWM on this half bridge
}

void DRV89xxMotor::setBridgeHSPWM(byte *settings, DRV89xxHalfBridge &bridge) {
  if (bridge.id==0) return;  // unconfigured

  #ifdef DEBUG_DRV89xx_MOTORS
  Serial.print("HBridge "); Serial.println(bridge.id);
  Serial.print("Brake HBridge L: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_2);
  Serial.print("Brake HBridge H: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" set bit "); Serial.println(bridge.bitshift_2+HIGH_SIDE);
  Serial.print("Brake HBridge PWM: 0x"); Serial.print(bridge.pwm_ctrl_register, HEX); Serial.print(" set bit "); Serial.println(bridge.bitshift_1);
  Serial.print("Setting PWM channel: 0x"); Serial.print(bridge.pwm_map_register, HEX); Serial.print(" bytes at " ); 
    Serial.print(bridge.bitshift_2); Serial.print(","); Serial.print(bridge.bitshift_2+1); Serial.print(": "); Serial.println(_pwm_channel);
  #endif

  BIT_SET(settings[bridge.enable_register], bridge.bitshift_2 + HIGH_SIDE); // enable highside of half bridge
  BIT_CLEAR(settings[bridge.enable_register], bridge.bitshift_2);  // disable low side of half bridge
  BIT_SET(settings[bridge.pwm_ctrl_register], bridge.bitshift_1);  // enable PWM on this half bridge
  bitWrite(settings[bridge.pwm_map_register], bridge.bitshift_2, _pwm_channel & 0b1);  // write the low bit
  bitWrite(settings[bridge.pwm_map_register], bridge.bitshift_2 + 1, (_pwm_channel & 0b10) >> 1); // write the high bit
}

void DRV89xxMotor::setBridgeOpen(byte *settings, DRV89xxHalfBridge &bridge) {
  if (bridge.id==0) return;  // unconfigured

  #ifdef DEBUG_DRV89xx_MOTORS
  Serial.print("HBridge "); Serial.println(bridge.id);
  Serial.print("Brake HBridge L: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_2);
  Serial.print("Brake HBridge H: 0x"); Serial.print(bridge.enable_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_2+HIGH_SIDE);
  Serial.print("Brake HBridge PWM: 0x"); Serial.print(bridge.pwm_ctrl_register, HEX); Serial.print(" clear bit "); Serial.println(bridge.bitshift_1);
  #endif

  BIT_CLEAR(settings[bridge.enable_register], bridge.bitshift_2);  // disable lowside of half bridge
  BIT_CLEAR(settings[bridge.enable_register], bridge.bitshift_2 + HIGH_SIDE); // disable highside of half bridge
  BIT_CLEAR(settings[bridge.pwm_ctrl_register], bridge.bitshift_1);  // disable PWM on this half bridge
}

void DRV89xxMotor::setPWMFrequency(byte *settings, byte _speed) {
  // Serial.print("Setting PWM channel[");
  // Serial.print(_pwm_channel);
  // Serial.print("]: ");
  // Serial.println(_speed);
  settings[(int)DRV89xxRegister::PWM_DUTY_CTRL_1 + _pwm_channel] = _speed;
}
