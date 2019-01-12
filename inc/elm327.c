/*
 * elm327.c
 *
 *  Created on: 27 sie 2017
 *      Author: djraszit
 */

#include <avr/io.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "main-defines.h"
#include "funkcje.h"
#include "elm327.h"
#include "control_bits.h"


void elm327_send_at_cmd(char* cmd) { //bez przedrostka AT, funkcja dodaje przedrostek AT
	SET(PORT, USART_COMM_LED);
	usart_send_string("AT");
	usart_send_string(cmd);
	usart_send_string("\r\n");
	CLR(PORT, USART_COMM_LED);
}

void elm327_send(char* cmd) {
	SET(PORT, USART_COMM_LED);
	usart_send_string(cmd);
	CLR(PORT, USART_COMM_LED);
}

uint8_t elm327_request(int mode, ...) {
	uint8_t count = 1;
	char str[64];
	char buf[8];
	memset(str, 0, 64);
	int input_value;
	va_list ap;
	va_start(ap, mode);
	sprintf(buf, "%02X", mode);
	strcat(str, buf);
	do {
		input_value = va_arg(ap, int);
		if (input_value == 0xffff) {
			break;
		}
		sprintf(buf, " %02X", input_value);
		strcat(str, buf);
		count++;
	} while (1);
	strcat(str, "\r");
	elm327_send(str);
	va_end(ap);
	return count;
}

int16_t elm327_calculate_data(uint8_t * buf, char * text){
	int16_t value = 0;
	if(buf[0] == (show_current_data | 0x40)){
		if(buf[1] == ENGINE_RPM){
			value = buf[2] << 8;
			value += buf[3];
			value /=4;
			sprintf(text, "%4d rpm", value);
			return value;
		}
		if (buf[1] == VEHICLE_SPEED){
			value = buf[2];
			sprintf(text, "%3d km/h", value);
			return value;
		}
		if (buf[1] == ENGINE_COOLANT_TEMP){
			value = buf[2] - 40;
			sprintf(text, "%3d%cC ", value, 0x1d);
			return value;
		}
	}
	return value;
}
