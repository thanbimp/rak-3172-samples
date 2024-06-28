#define a_samplTime 0x00  //32bit integer
#define a_tempLow 0x04    //16bit integers
#define a_tempHigh 0x06
#define a_pmLow 0x08
#define a_pmHigh 0x0A
#define a_co2Low 0x0C
#define a_co2High 0x0E
#define a_humLow 0x10
#define a_humHigh 0x12

//If you want to manually edit the parameters using stm32cubeprogrammer, the flash memory user partition starts at 0x08037800

#ifndef PARAM_STORE_H
#define PARAM_STORE_H

class paramStore {
public:
    void writeInt16(uint16_t value, int address);
    void writeInt32(uint32_t value, int address);
    void writeFloat(float value, int address);
    uint16_t readInt16(int address);
    uint32_t readInt32(int address);
    float readFloat(int address);
};

#endif