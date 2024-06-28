#ifndef BATTERYLVL_H
#define BATTERYLVL_H
#include "stm32wlxx_hal.h"


// Define the address of VREFINT_CAL
#define VREFINT_CAL_ADDR ((uint16_t*)0x1FFF75AA)

class batteryLvl {
  ADC_HandleTypeDef hadc;
  void ADC_Init();
  uint16_t Read_ADC();
  void  Error_Handler();
  float Calculate_VDDA();
  int calculateBatteryPercentage(float vdda);
public:
  batteryLvl();
  float getLvlPercentage();
};
#endif