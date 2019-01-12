#include "DS18B20.h"
#include "1wire_basic.h"
#include "1wire.h"
#include "1wire_defines.h"

#include <avr/io.h>
#include <util/delay.h>

#define PULLUP_ON   DDRC |= (1 << PC2);PORTC |= (1 << PC2);
#define DELAY 		_delay_us(10);
#define PULLUP_OFF  DDRC &= ~(1 << PC2);PORTC &= ~(1 << PC2);

void OW_Start_Conversion(uint8_t block) {
	if (OW_WaitForPresencePulse() == 0)
		return;
	OW_Write(OW_SkipROM);
	OW_Write(OW_CONVERT);
//	PULLUP_ON
//	DELAY
//	PULLUP_OFF
	if (block) {
		while (!OW_ReadBit());
	}

}

int16_t OW_GetTemperature(uint8_t *aID) {
	OW_SelectDevice(aID);
	if (Error != OW_OK)
		return 0xFFFF;

	OW_Write(OW_READ_SCRATCHPAD);
	uint16_t temp = OW_Read();
	temp = temp + (OW_Read() << 8);
	if (temp & 0x8000)
		temp = 1 - (temp ^ 0xFFFF); //Konwersja kod�w
	return temp;
}
