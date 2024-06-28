#include <stdint.h>
#include "RAKUnifiedApi.h"
#include "paramStore.h"

//This could have been done with operator overloading, but i think this makes it clearer.


void paramStore::writeInt16(uint16_t value, int address) {
  uint8_t flash_value[2] = { 0 };
  flash_value[0] = (uint8_t)(value >> 0);
  flash_value[1] = (uint8_t)(value >> 8);
  api.system.flash.set(address, flash_value, 2);
}

void paramStore::writeInt32(uint32_t value, int address) {
  uint8_t flash_value[4] = { 0 };
  flash_value[0] = (uint8_t)(value >> 0);
  flash_value[1] = (uint8_t)(value >> 8);
  flash_value[2] = (uint8_t)(value >> 16);
  flash_value[3] = (uint8_t)(value >> 24);
  api.system.flash.set(address, flash_value, 4);
}

void paramStore::writeFloat(float value, int address) {
  uint8_t flash_value[4] = { 0 };
  uint32_t intValue = *(uint32_t*)&value; // Interpret float value as an unsigned integer
  flash_value[0] = (uint8_t)(intValue >> 0);
  flash_value[1] = (uint8_t)(intValue >> 8);
  flash_value[2] = (uint8_t)(intValue >> 16);
  flash_value[3] = (uint8_t)(intValue >> 24);
  api.system.flash.set(address, flash_value, 4);
}

uint16_t paramStore::readInt16(int address) {
  uint16_t value = 0;
  uint8_t flash_read[2] = { 0 };
  api.system.flash.get(address, flash_read, 2);
  value |= flash_read[0] << 0;
  value |= flash_read[1] << 8;
  return value;
}

uint32_t paramStore::readInt32(int address) {
  uint32_t value = 0;
  uint8_t flash_read[4] = { 0 };
  api.system.flash.get(address, flash_read, 4);
  value |= flash_read[0] << 0;
  value |= flash_read[1] << 8;
  value |= flash_read[2] << 16;
  value |= flash_read[3] << 24;
  return value;
}

float paramStore::readFloat(int address) {
  uint32_t value = 0;
  uint8_t flash_read[4] = { 0 };
  api.system.flash.get(address, flash_read, 4);
  value |= flash_read[0] << 0;
  value |= flash_read[1] << 8;
  value |= flash_read[2] << 16;
  value |= flash_read[3] << 24;
  return *((float*)&value);
}
