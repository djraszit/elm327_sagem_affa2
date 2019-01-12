/*
 * sagem_affa2.h
 *
 *  Created on: 14 sie 2017
 *      Author: djraszit
 */

#ifndef INC_SAGEM_AFFA2_H_
#define INC_SAGEM_AFFA2_H_

#define MRQ				C, 3
#define LCD_ON_OFF		C, 2
#define LCD_COMM_LED	C, 1


#define I_NEWS				3 << 0
#define I_NEWS_BLINK		1 << 0
#define I_TRAFFIC			3 << 2
#define I_TRAFFIC_BLINK		1 << 2
#define AF					3 << 4
#define AF_BLINK			1 << 4
#define TUNER_LIST			3 << 6
#define TUNER_LIST_BLINK	1 << 6

#define TUNER_PRESET_ON		1 << 9
#define MSS_ON				1 << 10
#define DOLBY_ON			1 << 11
#define TUNER_MANU_ON		1 << 13

#define ALL_ICON			0xffff

#define NO_SCROLL			0x01
#define SWITCH_TEXT			0x02
#define SCROLL_TEXT			0x03

void sagem_affa2_set_icon(uint16_t icon);
void sagem_affa2_clr_icon(uint16_t icon);
void sagem_affa2_channel(uint8_t ch);

void sagem_affa2_init();
void read_sagem(uint8_t * buf);
void write_sagem(uint8_t * buf);
uint8_t make_address(uint8_t ile, uint8_t ktora, uint8_t typ);
void write_text_sagem(volatile char * text, uint8_t scroll_type);


#endif /* INC_SAGEM_AFFA2_H_ */
