/*
 * one-wire.c
 *
 *  Created on: 3 lip 2017
 *      Author: root
 */
#include <avr/io.h>
#include <string.h>
#include "../inc/1wire_basic.h"
#include "../inc/1wire.h"
#include "../inc/DS18B20.h"
#include "../inc/1wire_defines.h"

extern uint8_t Error;

uint8_t pomiar[2] = { 0, 0 };
float temp[2][ILE_POMIAROW];

uint8_t aID[8];
uint8_t ID[4][7];
uint8_t i, pos = 0;

void OneWireScan() {
	for (i = 0; i < 4; i++) {
		Error = OW_OK;
		pos = OWI_Search(OW_SearchROM, aID, pos);
		if (Error != OW_OK) break;
		memcpy(&ID[i][0], aID, 7);  //Skopiuj zeskanowane ID

//		for (uint8_t ii = 0; ii < 8; ii++) {
//			sprintf(lcdbufor, "%02X", aID[ii]);
//			lcd_send_text(lcdbufor);
//		}
		if (pos == 0) break;
	}
}

float DS18B20_gettemp(uint8_t id) {
	uint8_t p;
	int16_t t;
	float tempret = 0;
	float ret = 0;
//	OW_Start_Conversion(1);
	t = OW_GetTemperature(&ID[id][0]);
	int16_t d = t >> 4;
	int16_t f = (t & 0b1111) * 625;
	tempret = (float) (d * 1.0) + (f / 10000.0);

	for (uint8_t s = 1; s < ILE_POMIAROW; s++) {
		temp[id][ILE_POMIAROW - s] = temp[id][ILE_POMIAROW - 1 - s];
	}
	temp[id][0] = tempret;

	if (pomiar[id] < ILE_POMIAROW) pomiar[id]++;

	for (p = 0; p < pomiar[id]; p++) {
		ret += temp[id][p];
	}

	return ret / p;

}
