#include "Arduino.h"
#include "DRV89xx.h"
#include "DRV89xxRegister.h"

DRV89xx::DRV89xx(int cs_pin, int fault_pin, int sleep_pin) : _cs_pin(cs_pin), _fault_pin(fault_pin), _sleep_pin(sleep_pin), _spi_settings(4000000, MSBFIRST, SPI_MODE1) {}

void DRV89xx::begin() {
  // Setup SPI
  SPI.begin(SCK, MISO, MOSI, _cs_pin);
  SPI.setHwCs(true);
  

  // Setup pins
  pinMode(_cs_pin, OUTPUT);
  if(_sleep_pin) {
    pinMode(_sleep_pin, OUTPUT);
    digitalWrite(_sleep_pin, HIGH);  // enable chip
  }
  if(_fault_pin) pinMode(_fault_pin, INPUT);

  // Configure device
  _config_cache[(int)DRV89xxRegister::OLD_CTRL_1] = 0b11111111; // Disable open load detection on channels 1-8
  _config_cache[(int)DRV89xxRegister::OLD_CTRL_2] = 0b11001111; // Disable errors from open load detection, open load detect on channels 9-12
  _config_cache[(int)DRV89xxRegister::PWM_FREQ_CTRL] = 0xFF;  // Set all 4 PWM channels to 2kHz (max speed), TODO: Possibly make this a configurable option?
  // TODO: Possibly make FW_CTRL_1+ an option?
  writeConfig();
}

void DRV89xx::configMotor(byte motor_id, byte hb1, byte hb2, byte pwm_channel) {
  _motor[motor_id] = DRV89xxMotor(hb1, hb2, pwm_channel);
}

byte DRV89xx::writeRegister(byte address, byte value) {
  uint16_t ret = SPI.transfer16((address << 8) | value);
  delayMicroseconds(1);  // Give the chip a chance to write
}

byte DRV89xx::readRegister(byte address) {
  uint16_t ret = SPI.transfer16(DRV89xx_REGISTER_READ | (address << 8));
  return ret & 0xFF;
}


void DRV89xx::readErrorStatus() {
  SPI.beginTransaction(_spi_settings);
  if (digitalRead(_fault_pin) == 0) Serial.println("Error detected");
  else Serial.println("No errors");
  Serial.print("Status: ");
  Serial.println(readRegister(0x00), HEX);
  Serial.print("Overcurrent: ");
  Serial.print(readRegister(0x03), HEX);
  Serial.print(readRegister(0x02), HEX);
  Serial.println(readRegister(0x01), HEX);
  Serial.print("Open Load: ");
  Serial.print(readRegister(0x06), HEX);
  Serial.print(readRegister(0x05), HEX);
  Serial.print(readRegister(0x04), HEX);
  SPI.endTransaction();
}

void DRV89xx::writeConfig() {
  // Flush the 28 bytes of cache
  SPI.beginTransaction(_spi_settings);
  for(byte i=DRV89xx_CONFIG_WRITE_START; i<DRV89xx_CONFIG_BYTES; i++) {
    DRV89xx::writeRegister(i, _config_cache[i]);
  }
  SPI.endTransaction();
}

void DRV89xx::updateConfig() {
  byte i;
  for(i=0;i<DRV89xx_MAX_MOTORS; i++) {
    _motor[i].applyConfig(_config_cache);
  }
  SPI.beginTransaction(_spi_settings);
  for(i=DRV89xx_UPDATE_START; i<=DRV89xx_UPDATE_END; i++) {
    DRV89xx::writeRegister(i, _config_cache[i]);
  }
  SPI.endTransaction();
}
