#ifndef _DS18B20_H_
#define _DS18B20_H_

#include <stdint.h>

#define OW_CONVERT 0x44
#define OW_READ_SCRATCHPAD 0xBE
#define OW_WRITE_SCRATCHPAD 0x4E
#define OW_COPY_SCRATCHPAD 0x48
#define OW_RECALL_E2 0xB8
#define OW_READ_POWER_SUPPLY 0xB4

void OW_Start_Conversion(uint8_t block);
int16_t OW_GetTemperature(uint8_t *aID);

#endif
