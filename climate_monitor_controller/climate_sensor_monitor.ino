#include "paramStore.h"
#include "batteryLvl.h"
#include <ArduinoJson.h>

#define FAN_PIN PA9

enum Param {
  SAMPLTIME,
  TEMPLOW,
  TEMPHIGH,
  PMLOW,
  PMHIGH,
  CO2LOW,
  CO2HIGH,
  HUMLOW,
  HUMHIGH,
  UNKNOWN
};

// map string parameters to enumeration values
Param getParamEnum(const char *param) {
  if (strcmp(param, "samplTime") == 0) return SAMPLTIME;
  if (strcmp(param, "tempLow") == 0) return TEMPLOW;
  if (strcmp(param, "tempHigh") == 0) return TEMPHIGH;
  if (strcmp(param, "pmLow") == 0) return PMLOW;
  if (strcmp(param, "pmHigh") == 0) return PMHIGH;
  if (strcmp(param, "co2Low") == 0) return CO2LOW;
  if (strcmp(param, "co2High") == 0) return CO2HIGH;
  if (strcmp(param, "humLow") == 0) return HUMLOW;
  if (strcmp(param, "humHigh") == 0) return HUMHIGH;
  return UNKNOWN;
}

struct msg {
  uint16_t co2ppm;
  uint16_t pm25;
  float temp;
  uint8_t hum;
};

msg receivedData;
uint8_t MsgBuffer[sizeof(receivedData)];
paramStore paramStore;
JsonDocument doc;  //use one global JsonDocument, to avoid information loss between lora cycles (e.g fan status update after parameter change)
batteryLvl lvl;

uint8_t appeui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t deui[8] = { 0xAC, 0x1F, 0x09, 0xFF, 0xFE, 0x11, 0xD4, 0xBC };  // Update in final
uint8_t appkey[16] = { 0xF6, 0xF8, 0xCF, 0x8F, 0xED, 0x87, 0x43, 0xA5, 0x4D, 0xFB, 0xE9, 0x55, 0xD4, 0xEE, 0x01, 0xD5 };

uint32_t samplTime;
float tempLow;
float tempHigh;
uint16_t pmLow;
uint16_t pmHigh;
uint16_t co2Low;
uint16_t co2High;
uint16_t humLow;
uint16_t humHigh;

bool fanState = false;

void populateParams() {
  samplTime = paramStore.readInt32(a_samplTime);
  tempLow = paramStore.readFloat(a_tempLow);
  tempHigh = paramStore.readFloat(a_tempHigh);
  pmLow = paramStore.readInt16(a_pmLow);
  pmHigh = paramStore.readInt16(a_pmHigh);
  co2Low = paramStore.readInt16(a_co2Low);
  co2High = paramStore.readInt16(a_co2High);
  humLow = paramStore.readInt16(a_humLow);
  humHigh = paramStore.readInt16(a_humHigh);
}

void setup() {
  api.system.lpm.set(1);
  populateParams();
  Serial.begin(115200, RAK_CUSTOM_MODE);

  if (api.lorawan.nwm.get() != 1) {
    api.lorawan.nwm.set();
    api.system.reboot();
  }

  api.lorawan.appeui.set(appeui, 8);
  api.lorawan.appkey.set(appkey, 16);
  api.lorawan.deui.set(deui, 8);

  api.lorawan.band.set(RAK_REGION_AS923);
  api.lorawan.deviceClass.set(RAK_LORA_CLASS_A);
  api.lorawan.njm.set(RAK_LORA_OTAA);

  api.lorawan.adr.set(1);

  while (api.lorawan.njs.get() == 0) {
    api.lorawan.join();
    delay(10000);
  }

  api.lorawan.registerRecvCallback(receiveCallback);

  api.system.timer.create(RAK_TIMER_0, (RAK_TIMER_HANDLER)sendUpdData, RAK_TIMER_PERIODIC);
  api.system.timer.create(RAK_TIMER_1, (RAK_TIMER_HANDLER)monitor, RAK_TIMER_PERIODIC);

  api.system.timer.start(RAK_TIMER_0, samplTime, nullptr);
  api.system.timer.start(RAK_TIMER_1, 1000, nullptr);


  pinMode(FAN_PIN, OUTPUT);
}


void receiveCallback(SERVICE_LORA_RECEIVE_T *data) {
  updateParam(data->Buffer, data->BufferSize);
}

void updateParam(uint8_t *buffer, int32_t size) {
  DeserializationError error = deserializeJson(doc, buffer, size);
  if (error) {
    return;
  }

  if (doc.containsKey("param")) {
    switch (getParamEnum(doc["param"])) {
      case SAMPLTIME:
        samplTime = doc["value"];
        paramStore.writeInt32(samplTime, a_samplTime);
        api.system.timer.stop(RAK_TIMER_0);
        api.system.timer.start(RAK_TIMER_0, samplTime, nullptr);
        break;
      case TEMPLOW:
        tempLow = doc["value"];
        paramStore.writeFloat(tempLow, a_tempLow);
        break;
      case TEMPHIGH:
        tempHigh = doc["value"];
        paramStore.writeFloat(tempHigh, a_tempHigh);
        break;
      case PMLOW:
        pmLow = doc["value"];
        paramStore.writeInt16(pmLow, a_pmLow);
        break;
      case PMHIGH:
        pmHigh = doc["value"];
        paramStore.writeInt16(pmHigh, a_pmHigh);
        break;
      case CO2LOW:
        co2Low = doc["value"];
        paramStore.writeInt16(co2Low, a_co2Low);
        break;
      case CO2HIGH:
        co2High = doc["value"];
        paramStore.writeInt16(co2High, a_co2High);
        break;
      case HUMLOW:
        humLow = doc["value"];
        paramStore.writeInt16(humLow, a_humLow);
        break;
      case HUMHIGH:
        humHigh = doc["value"];
        paramStore.writeInt16(humHigh, a_humHigh);
        break;
      default:
        break;
    }
  }
}

void sendUpdData() {

  doc["co2"] = receivedData.co2ppm;
  doc["pm2"] = receivedData.pm25;
  doc["temp"] = receivedData.temp;
  doc["hum"] = receivedData.hum;
  doc["batLvl"] = lvl.getLvlPercentage();


  if (doc["hum"] > 100 | doc["pm2"] > 250 | doc["co2"] > 9000) {
    api.system.reboot();  //something has gone wrong, reboot!
  }

  String output;
  serializeJson(doc, output);
  doc.clear();

  api.lorawan.send(output.length(), (uint8_t *)output.c_str(), 129, true, 1);
}


void monitor(void *data) {
  Serial.write('R');  // Request Data from climate monitor node

  // Clear MsgBuffer before reading new data
  memset(MsgBuffer, 0, sizeof(MsgBuffer));

  size_t bytesRead = Serial.readBytes(MsgBuffer, sizeof(MsgBuffer));
  if (bytesRead == sizeof(MsgBuffer)) {
    memcpy(&receivedData, MsgBuffer, sizeof(receivedData));
    co2Logic(receivedData.co2ppm);
  } else {
    api.system.reboot();
  }
}


void co2Logic(int curCo2) {
  bool newFanState = (curCo2 > co2High);

  if (newFanState != fanState) {
    fanState = newFanState;

    if (newFanState) {
      digitalWrite(FAN_PIN, HIGH);
    } else {
      digitalWrite(FAN_PIN, LOW);
    }
    sendFanStateChange();
  }
}

void sendFanStateChange() {
  doc["fanState"] = fanState ? "ON" : "OFF";

  String output;
  serializeJson(doc, output);
  api.lorawan.send(output.length(), (uint8_t *)output.c_str(), 129, true, 10);
}

void loop() {
}
