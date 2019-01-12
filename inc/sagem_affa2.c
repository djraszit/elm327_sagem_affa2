/*
 * sagem_affa2.c
 *
 *  Created on: 14 sie 2017
 *      Author: djraszit
 */

#include "I2cbase.h"
#include <util/delay.h>
#include <string.h>
#include "funkcje.h"
#include "control_bits.h"
#include "sagem_affa2.h"

#define DELAY1			_delay_us(500)
#define DELAY2			_delay_us(500)
#define DELAY_RW		_delay_ms(3)

char buff[64];

static uint8_t data_to_send[16];
static uint8_t data_read[16];


//@46,0C,90,7E,76,01,56,4F,58,20,46,4D,20,20,00,00,00,00
//@46,0F,90,7F,55,FF,DF,71,01,4C,20,20,32,30,37,20,20,00


//@46,0C,90,76,71,01,52,61,64,69,6F,5A,45,54,00,00,00,00
//@46,0F,90,77,65,FF,DF,20,01,20,20,20,20,39,37,39,20,00



//preambules
//
//                            size              icons       icons chan  		addr
uint8_t lcd_preambule1[8] = { 0x0f, 0x90, 0x7f, 0xff, 0x00, 0xff, CHAN_CLR_DOT, 0x01 };

//                            size              chan          addr
uint8_t lcd_preambule2[5] = { 0x0c, 0x90, 0x76, CHAN_CLR_DOT, 0x01 };

//                            size        mask  icons       icons
uint8_t lcd_preambule3[6] = { 0x05, 0x90, 0x71, 0xff, 0xff, 0xff };



void sagem_test(uint8_t arg){
	lcd_preambule3[2] = arg;
	sagem_write(lcd_preambule3);
}

void sagem_affa2_set_icon(uint16_t icon) {
	lcd_preambule3[3] &= ~(icon & 0xff);
	lcd_preambule3[5] &= ~(icon >> 8);
	sagem_write(lcd_preambule3);
}

void sagem_affa2_clr_icon(uint16_t icon) {
	lcd_preambule3[3] |= (icon & 0xff);
	lcd_preambule3[5] |= (icon >> 8);
	sagem_write(lcd_preambule3);
}

void sagem_affa2_channel(uint8_t ch) {
	lcd_preambule1[6] = ch;
	lcd_preambule2[3] = ch;
}

void sagem_read_keys(uint8_t *buf) {
	data_to_send[0] = 0x01;
	data_to_send[1] = 0x11;
	sagem_write(data_to_send);
	sagem_read(buf);
}


void sagem_affa2_init() {
	SET(DDR, LCD_COMM_LED);
	CLR(PORT, LCD_COMM_LED);
	SET(DDR, LCD_ON_OFF);
	SET(PORT, LCD_ON_OFF);
	MRQ_AS_INPUT
	;
	_delay_ms(500);

	data_to_send[0] = 0x00;
	data_to_send[1] = 0x00;
	sagem_write(data_to_send);
	sagem_write(data_to_send);

	do {
		data_to_send[0] = 0x01;
		data_to_send[1] = 0x10;
		sagem_write(data_to_send); //send 0x01 0x10 and read
		MRQ_WAIT_0
			;
		sagem_read(data_read);
#ifdef DEBUG
		usart_print_hex("FIRST read sagem", data_read, 16);
#endif
		sagem_write(data_to_send);
		data_to_send[0] = 0x01;
		data_to_send[1] = 0x11;
		sagem_write(data_to_send);
		sagem_write(data_to_send);
		MRQ_WAIT_0
			;
		sagem_read(data_read);
#ifdef DEBUG
		usart_print_hex("SECOND read sagem", data_read, 16);
#endif
	} while (data_read[1] != 0x01);
}

void sagem_read(uint8_t * buf) {
	SET(PORT, LCD_COMM_LED);
	uint8_t i;
	start:
	MRQ_AS_OUTPUT_LOW
	;
	DELAY1;
	I2C_Start();
	I2C_SendAddr(LCD_READ_ADDR);
	if (TW_STATUS == TW_MR_SLA_NACK) {
#ifdef DEBUG
		usart_send_string("read NACK\n\r");
#endif
		MRQ_AS_INPUT
		;
		DELAY_RW;
		goto start;
	}
	buf[0] = I2C_ReceiveData_ACK();

	for (i = 0; i < buf[0]; i++) {
		buf[i + 1] = I2C_ReceiveData_ACK();
	}
	I2C_Stop();
	DELAY2;
	MRQ_AS_INPUT
	;
	CLR(PORT, LCD_COMM_LED);
	DELAY_RW;
}

void sagem_write(uint8_t * buf) {
	SET(PORT, LCD_COMM_LED);
	uint8_t i = 0;
	start:
	MRQ_AS_OUTPUT_LOW
	;
	DELAY1;
	I2C_Start();
	I2C_SendAddr(LCD_WRITE_ADDR);
	if (TW_STATUS == TW_MT_SLA_NACK) {
#ifdef DEBUG
		usart_send_string("write NACK\n\r");
#endif
		MRQ_AS_INPUT
		;
		DELAY_RW;
		goto start;
	}
	for (i = 0; i < buf[0] + 1; i++) {
		I2C_SendByte(buf[i]);
	}
	I2C_Stop();

	DELAY2;
	MRQ_AS_INPUT
	;;
	CLR(PORT, LCD_COMM_LED);
	DELAY_RW;
}

static uint8_t make_address(uint8_t ile, uint8_t ktora, uint8_t typ) {
	return ((ile - 1) << 5) | (ktora << 2) | (typ & 0x03);
}

void sagem_write_text(volatile char * text, uint8_t scroll_type) {
	uint8_t buf[16];
	uint8_t i = 0;
	uint8_t how_many_lines = 0;
	uint8_t current_line = 0;
	uint8_t s = strlen((char*)text);
	if (scroll_type == NO_SCROLL) {
		how_many_lines = 1;
	}
	while (i++ < s) {
		if (i % 8 == 1 && i > 0) {
			how_many_lines++;
		}
	}
	if (s <= 8) {
		how_many_lines = 1;
		scroll_type = NO_SCROLL;
	}

	memcpy(&buf, &lcd_preambule2, lcd_preambule2[0] - 7);

	while (current_line < how_many_lines) {

		uint8_t preamb_size = buf[0] - 7;

		buf[preamb_size - 1] = make_address(how_many_lines, current_line, scroll_type);
		i = 0;
		while (i < 8) {
			buf[preamb_size + i] = *text++;
			i++;
			if (*text == 0) {
				break;
			}
		}
		while (i < 8) {
			buf[preamb_size + i] = 0x20;
			i++;
		}
		sagem_write(buf);
		current_line++;
	}
}
