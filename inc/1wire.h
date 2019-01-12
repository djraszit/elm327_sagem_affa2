#ifndef _1WIRE_H_
#define _1WIRE_H_

#include <stdint.h>

#define OW_MatchROM	0x55
#define OW_SearchROM 0xF0
#define OW_SkipROM 0xCC
#define OW_Alarm_Search 0xEC

void OW_Write(uint8_t);
uint8_t OW_Read();
void OW_SelectDevice(const uint8_t *aID);
uint8_t OWI_Search(uint8_t cmd, uint8_t *aID, uint8_t deviationpos);

#endif
