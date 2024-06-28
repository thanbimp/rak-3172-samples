#include <stdlib.h>
#include <ArduinoJson.h>
#include "ModbusMaster.h"


#define RX_ENA_P PB4
#define TX_ENA_P PB5

unsigned char *msgBuffer = NULL;  // Initialize array pointer to NULL
int currentSize = 0;              // Current size of the array
bool joinResult;
uint8_t result;

uint8_t appeui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t deui[8] = { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x11, 0xD4, 0xBC };
uint8_t appkey[16] = { 0xF6, 0xF8, 0xCF, 0x8F, 0xED, 0x87, 0x43, 0xA5, 0x4D, 0xFB, 0xE9, 0x55, 0xD4, 0xEE, 0x01, 0xD5 };
uint16_t registers[5];


JsonDocument doc;

ModbusMaster node;

void main_action(void *) {
  api.system.lpm.set(0);
  result = node.readInputRegisters(0, 5);  //Read 30000 to 30009
  if (result == node.ku8MBSuccess)         //Read success
  {
    doc["co2"] = node.getResponseBuffer(0);
    doc["hum"] = node.getResponseBuffer(1) / 100.0f;
    doc["temp"] = node.getResponseBuffer(2) / 100.0f;
    doc["dewpoint"] = node.getResponseBuffer(4) / 100.0f;
    String jsonOutput = doc.as<String>();
    api.lorawan.send(strlen(jsonOutput.c_str()), (uint8_t *)jsonOutput.c_str(), 129, true, 1);
  }
  api.system.lpm.set(1);
}

void setup() {
  service_nvm_set_baudrate_to_nvm(115200);
  node.begin(1, Serial);  //slave ID node
  if (api.lorawan.nwm.get() != 1) {
    api.lorawan.nwm.set(1);
    api.system.reboot();
  }
  api.system.timer.create(RAK_TIMER_0, main_action, RAK_TIMER_PERIODIC);
  api.system.timer.start(RAK_TIMER_0, 10000, NULL);
  pinMode(RX_ENA_P, OUTPUT);
  pinMode(TX_ENA_P, OUTPUT);
  digitalWrite(RX_ENA_P, 1);
  digitalWrite(TX_ENA_P, 1);
  api.lorawan.appeui.set(appeui, 8);
  api.lorawan.appkey.set(appkey, 16);
  api.lorawan.deui.set(deui, 8);

  api.lorawan.band.set(RAK_REGION_AS923);
  api.lorawan.deviceClass.set(RAK_LORA_CLASS_A);
  api.lorawan.njm.set(RAK_LORA_OTAA);

  api.lorawan.adr.set(1);


  /** Wait for Join success */
  while (api.lorawan.njs.get() == 0) {
    //Serial.print("Wait for LoRaWAN join...");
    api.lorawan.join();
    delay(10000);
  }
  api.system.lpm.set(1);
}

void loop() {
}
