#ifndef _1WIRE_BASIC_H_
#define _1WIRE_BASIC_H_

#include <stdbool.h>

#define OW_OK	0
#define OW_BusShorted	1
#define OW_NoPresencePulse	2
#define OW_SearchNoResponse 3

extern uint8_t Error;

void OW_SendBit(bool);
bool OW_ReadBit();
void OW_ResetPulse();
bool OW_WaitForPresencePulse();
void OW_init();

#endif

