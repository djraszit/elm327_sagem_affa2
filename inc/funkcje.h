/*
 * funkcje.h
 *
 *  Created on: 10.09.2016
 *      Author: djraszit
 */

#ifndef INC_FUNKCJE_H_
#define INC_FUNKCJE_H_

#define true 	1
#define false 	0

#define EE(s) (__extension__({static uint8_t  __ee[] EEMEM = (s); &__ee[0];}))

void usart_init();
void usart_send_byte(uint8_t byte);
void usart_send_char(char c);
void usart_send_string(char *string);
void usart_send_eeprom(const uint8_t* string);
void usart_send_array(uint8_t *array, uint16_t size);
void usart_print_hex(char *name, uint8_t *buf, uint16_t lenght);

void SPI_Master_init();
uint8_t SPI_send_rec_byte(uint8_t byte);
void SPI_send(uint8_t data);
uint8_t SPI_read(uint8_t addr);
uint16_t adc_read(uint8_t input);
uint8_t adc_read8bit(uint8_t input);

void string_to_oktet(char* ipadres, uint8_t *pa, uint8_t *pb, uint8_t *pc,
		uint8_t *pd, uint8_t endsign);
uint8_t gamma_correction(uint8_t value,uint8_t max,double gamma);
void clear_buffer(volatile void *buf, uint16_t size);
uint8_t asciiHex_to_byte(char * c);
uint16_t parsehex_to_array(char *c, uint8_t pause, uint8_t *array, uint16_t size_of_array);

#endif /* INC_FUNKCJE_H_ */
