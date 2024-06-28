#include "stm32wlxx.h"
#include "stm32wle5xx.h"
#include "batteryLvl.h"

void batteryLvl::Error_Handler(void) {
  // User can add their own implementation to report the HAL error return state
  while (1) {
    // Blink LED or log error for debugging
    NVIC_SystemReset();
  }
}

void batteryLvl::ADC_Init(void) {
  // Enable ADC clock
  __HAL_RCC_ADC_CLK_ENABLE();

  hadc.Instance = ADC;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;  // Adjust as needed
  hadc.Init.Resolution = ADC_RESOLUTION_12B;            // 12-bit resolution
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;            // Right-aligned data
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;            // Single channel scan mode
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;         // End of conversion selection
  hadc.Init.LowPowerAutoWait = ENABLE;                  // Enable Auto wait mode
  hadc.Init.LowPowerAutoPowerOff = DISABLE;             // Disable Auto power-off
  hadc.Init.ContinuousConvMode = DISABLE;               // Single conversion mode
  hadc.Init.NbrOfConversion = 1;                        // Number of conversions
  hadc.Init.DiscontinuousConvMode = DISABLE;            // Disable discontinuous mode
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;      // Software start conversion
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; // No external trigger edge
  hadc.Init.DMAContinuousRequests = DISABLE;            // Disable DMA continuous requests
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;           // Preserve data on overrun
  hadc.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5; // Sampling time
  hadc.Init.OversamplingMode = DISABLE;                 // Disable oversampling
  hadc.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH; // Trigger frequency

  if (HAL_ADC_Init(&hadc) != HAL_OK) {
    Error_Handler();
  }

  // Run the ADC calibration in single-ended mode
  if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) {
    // Calibration Error
    Error_Handler();
  }

  // Configure ADC regular channel
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK) {
    // Channel Configuration Error
    Error_Handler();
  }
}

uint16_t batteryLvl::Read_ADC() {
  ADC_COMMON->CCR |= ADC_CCR_VREFEN;
  HAL_ADC_Start(&hadc); // Ensure ADC is started
  HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
  uint16_t value = HAL_ADC_GetValue(&hadc);
  HAL_ADC_Stop(&hadc); // Stop ADC after reading
  return value;
}

// Function to calculate VDDA
float batteryLvl::Calculate_VDDA(void) {
  ADC_Init();
  uint16_t vrefint_cal = *VREFINT_CAL_ADDR;  // Read VREFINT_CAL
  const int num_samples = 10;
  uint32_t sum_vrefint = 0;

  for (int i = 0; i < num_samples; i++) {
    sum_vrefint += Read_ADC();  // Read VREFINT from ADC multiple times
  }
  uint16_t vrefint = sum_vrefint / num_samples;  // Average the readings

  // Calculate VDDA
  float vdda = 3.3f * ((float)vrefint_cal / (float)vrefint);
  return vdda;
}

int batteryLvl::calculateBatteryPercentage(float vdda) {
  // Define actual voltage ranges for 100% and 0% battery levels
  const float maxVoltage = 3.0f;  // Corresponds to 100% battery level
  const float minVoltage = 2.0f;  // Corresponds to 0% battery level

  // Ensure VDDA value is within the expected range
  if (vdda > maxVoltage) vdda = maxVoltage;
  if (vdda < minVoltage) vdda = minVoltage;

  // Perform the conversion
  float percentage = 100.0f * (vdda - minVoltage) / (maxVoltage - minVoltage);
  return static_cast<int>(percentage);
}

batteryLvl::batteryLvl() {
}

float batteryLvl::getLvlPercentage() {
  return calculateBatteryPercentage(Calculate_VDDA());
}
