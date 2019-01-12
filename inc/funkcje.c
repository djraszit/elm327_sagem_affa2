/*
 * funkcje.c
 *
 *  Created on: 10.09.2016
 *      Author: djraszit
 */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <util/twi.h>
#include <ctype.h>
#include "control_bits.h"


void usart_init(void) {
	DDRD |= (1 << PD1);
	DDRD &= ~(1 << PD0);

#define BAUD 38400        //tutaj podaj żądaną prędkość transmisji
#include <util/setbaud.h> //linkowanie tego pliku musi być
	//po zdefiniowaniu BAUD
	//ustaw obliczone przez makro wartości
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
#if USE_2X
	UCSR0A |= (1 << U2X0);
#else
	UCSR0A &= ~(1<<U2X0);
#endif

	UCSR0B = (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

}

void usart_send_byte(uint8_t byte) {
	while (!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = byte;
}
void usart_send_char(char c) {
	while (!(UCSR0A & (1 << UDRE0)))
		;
	UDR0 = c;
}
void usart_send_string(char *string) {
	char c;
	while ((c = *string++)) {
		usart_send_byte(c);
	}
}

void usart_send_eeprom(const uint8_t* string) {
	uint8_t c;
	while ((c = eeprom_read_byte(string++))) {
		if (c == 0x00) break;
		usart_send_byte(c);
	}
}

void usart_send_array(uint8_t *array, uint16_t size) {
	for (int x = 0; x < size; x++) {
		usart_send_byte(*array++);
	}
}

void usart_print_hex(char *name, uint8_t *buf, uint16_t lenght) {
	char send_buf[64];
	sprintf(send_buf, "\n\rPacket name: %s, lenght: %d", name, lenght);
	usart_send_string(send_buf);
	usart_send_char(0x0a);
	usart_send_char(0x0d);
	sprintf(send_buf, "       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
	usart_send_string(send_buf);
	usart_send_char(0x0a);
	usart_send_char(0x0d);
	uint16_t l;
	usart_send_string("0000: ");
	for (l = 0; l < lenght; l++) {
		if (!(l % 16) & (l > 0)) {
			sprintf(send_buf, "\n\r%04x: ", l);
			usart_send_string(send_buf);
		}
		sprintf(send_buf, "%02x ", *(buf + l));
		usart_send_string(send_buf);
	}
	usart_send_char(0x0a);
	usart_send_char(0x0d);
}

void SPI_Master_init() {
	DDRB = (1 << PB4) | (1 << PB5) | (1 << PB7); //SS,MOSI,SCK as outputs
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
	SPSR;
	SPDR;
}

uint8_t SPI_send_rec_byte(uint8_t byte) {
	SPDR = byte;
	while (!(SPSR & (1 << SPIF)))
		;
	return SPDR;
}
#define SS_LOW    PORTB &= ~(1 << PB4)
#define SS_HIGH   PORTB |= (1 << PB4)

void SPI_send(uint8_t data) {
	SS_LOW;
	SPI_send_rec_byte(data);
	SS_HIGH;
}

uint8_t SPI_read(uint8_t addr) {
	SS_LOW;
	SPI_send_rec_byte(addr);
	uint8_t data = SPI_send_rec_byte(0x00);
	SS_HIGH;
	return data;
}

uint16_t adc_read(uint8_t input) {
	ADMUX = 0;
	ADMUX = (1 << REFS0) | (input << 0);
	ADCSRA |= (1 << ADSC);
	while ((ADCSRA & (1 << ADSC)))
		;
	uint16_t ret = ADC;
	return ret;
}

uint8_t adc_read8bit(uint8_t input) {
	ADMUX = 0;
	ADMUX = (1 << ADLAR) | (1 << REFS0) | (input << 0);
	ADCSRA |= (1 << ADSC);
	while ((ADCSRA & (1 << ADSC)))
		;
	uint8_t ret = ADCH;
	return ret;
}

void string_to_oktet(char* ipadres, uint8_t *pa, uint8_t *pb, uint8_t *pc,
		uint8_t *pd, uint8_t endsign) {
	char* temp;
	char x, y, s[4];
	temp = strchr(ipadres, '.');
	x = temp - ipadres + 1;
	strncpy(s, ipadres, x);
	*pa = atoi(s);
//	printf("str %d", x);
	y = x;
	temp = strchr(temp + 1, '.');
	x = temp - ipadres + 1;
	strncpy(s, ipadres + y, x - y);
	*pb = atoi(s);
//	printf("str %d", x);
	y = x;
	temp = strchr(temp + 1, '.');
	x = temp - ipadres + 1;
	strncpy(s, ipadres + y, x - y);
	*pc = atoi(s);
//	printf("str %d", x);
	y = x;
	temp = strchr(temp + 1, endsign);
	x = temp - ipadres + 1;
	strncpy(s, ipadres + y, x - y);
	*pd = atoi(s);
//	printf("str %d", x);
}

uint8_t gamma_correction(uint8_t value,uint8_t max,double gamma){
	return max * pow((double)value/max,1.0/gamma);
}

void clear_buffer(volatile void *buf, uint16_t size) {
	uint8_t *b = (uint8_t*) buf;
	while (size--) {
		*b++ = 0;
	}
}

uint8_t print_tw_status(char *name) {
	uint8_t tw_status = TW_STATUS;
	char *bufor = malloc(64);
	sprintf(bufor, "%s, status:%02x\n\r", name, tw_status);
	usart_send_string(bufor);
	free(bufor);
	return tw_status;
}

uint8_t asciiHex_to_byte(char * c) {
	if (isalnum(c[0]) == 0) {
		return 0;
	}
	int x;
	if (c[0] > '9') {
		x = (c[0] & 0x0f) + 9;
	} else {
		x = c[0] & 0x0f;
	}
	x <<= 4;
	if (c[1] > '9') {
		x += (c[1] & 0x0f) + 9;
	} else {
		x += c[1] & 0x0f;
	}
	return x;
}

uint16_t parsehex_to_array(char *c, uint8_t pause, uint8_t *array, uint16_t size_of_array) {
	char i = 0;
	while (*c) {
		if (*c == '\n' || *c == '\r') { //jesli CR lub NL to zakoncz i zwroc ile zparsowano
			return i;
		}
		if (*c == ' ') {     //jeśli spacja to zwieksz wskaznik i pomin iteracje
			c++;
			continue;
		}
		*array = asciiHex_to_byte(c);             //konwersja ascii hex do bajta
		c += 2;
		if (i++ == size_of_array) { //jesli liczba zparsowanych liczb jest rowna rozmiarowi tablicy to zakoncz i
			return i;              //zwroc ilosc zparsowanych liczb
		}
		if (*c == '\n' || *c == '\r') { //jesli CR lub NL to zakoncz i zwroc ile zparsowano
			return i;
		}
		array++;
		c += pause;
	}
	return i;
}

