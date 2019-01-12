#include <util/atomic.h>
#include <util/delay.h>
#include "1wire_basic.h"
#include "1wire_defines.h"
#include "funkcje.h"
#include "control_bits.h"


uint8_t Error;

void OW_SendBit(bool bit) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		SET(DDR, OW_PIN);

		_delay_us(3);
		if (bit) {
			CLR(DDR, OW_PIN);

		}
		_delay_us(60);
		CLR(DDR, OW_PIN);

	}
}

bool OW_ReadBit() {
	unsigned char tmp;
	unsigned char counter = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		SET(DDR, OW_PIN);

		_delay_us(3);
		CLR(DDR, OW_PIN);

		_delay_us(15);
		tmp = GET(OW_PIN);
	}
	while ((counter < 0xFF) && (GET(OW_PIN) == 0)) {
		_delay_us(2);
		counter++;
	}
	if (counter == 0xFF)
		Error = OW_BusShorted;
	return tmp;
}

void OW_ResetPulse() {
	SET(DDR, OW_PIN);
	_delay_us(480);
	CLR(DDR, OW_PIN);
}

bool OW_WaitForPresencePulse() {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		OW_ResetPulse();
		_delay_us(30);
		unsigned char counter = 0;
		while ((counter < 0xFF) && (GET(OW_PIN))) {
			_delay_us(1);
			counter++;
		}
		if (counter == 0xFF) {
			Error = OW_NoPresencePulse;
			return false;
		}
		counter = 0;
		while ((counter < 0xFF) && (GET(OW_PIN) == 0)) {
			_delay_us(2);
			counter++;
		}
		if (counter == 0xFF) {
			Error = OW_BusShorted;
			return false;
		}
		Error = OW_OK;
		return true;
	}
}

void OW_init() {
	CLR(DDR, OW_PIN);
	CLR(PORT, OW_PIN);
}
