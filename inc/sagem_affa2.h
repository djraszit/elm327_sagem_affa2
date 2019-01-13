/*
 * sagem_affa2.h
 *
 *  Created on: 14 sie 2017
 *      Author: djraszit
 */

#ifndef INC_SAGEM_AFFA2_H_
#define INC_SAGEM_AFFA2_H_

//#define DEBUG

#define SAGEM_MRQ			C, 3//MRQ pin
#define LCD_ON_OFF			C, 2//włączanie affa2++
#define LCD_COMM_LED		C, 1//dioda sygnalizująca komunikację z affa2++

#define LCD_READ_ADDR		0x47
#define LCD_WRITE_ADDR		0x46

#define MRQ_AS_INPUT		do {CLR(DDR,SAGEM_MRQ);SET(PORT,SAGEM_MRQ); }while(0)
#define MRQ_AS_OUTPUT_HIGH	do {SET(DDR,SAGEM_MRQ);SET(PORT,SAGEM_MRQ); }while(0)
#define MRQ_AS_OUTPUT_LOW	do {SET(DDR,SAGEM_MRQ);CLR(PORT,SAGEM_MRQ); }while(0)
#define MRQ_GET				GET(SAGEM_MRQ)

#define MRQ_WAIT_1			while(!(MRQ_GET))
#define MRQ_WAIT_0			while(MRQ_GET)

#define ICON_I_NEWS				(3 << 0)
#define ICON_I_NEWS_BLINK		(1 << 0)
#define ICON_I_TRAFFIC			(3 << 2)
#define ICON_I_TRAFFIC_BLINK	(1 << 2)
#define ICON_AF					(3 << 4)
#define ICON_AF_BLINK			(1 << 4)
#define ICON_TUNER_LIST			(3 << 6)
#define ICON_TUNER_LIST_BLINK	(1 << 6)

#define ICON_TUNER_PRESET_ON	(1 << 9)
#define ICON_MSS_ON				(1 << 10)
#define ICON_DOLBY_ON			(1 << 11)
#define ICON_TUNER_MANU_ON		(1 << 13)

#define ICON_ALL				0xffff


#define CHAN_BLINK			(1 << 7)
#define CHAN_CLR_DOT		(1 << 6)


#define NO_SCROLL			0x01
#define SWITCH_TEXT			0x02
#define SCROLL_TEXT			0x03

void sagem_affa2_set_icon(uint16_t icon);
void sagem_affa2_clr_icon(uint16_t icon);
void sagem_affa2_channel(uint8_t ch);

void sagem_affa2_init();
void sagem_read(uint8_t * buf);
void sagem_write(uint8_t * buf);

void sagem_read_keys(uint8_t *buf);

void sagem_write_text(volatile char * text, uint8_t scroll_type);


#endif /* INC_SAGEM_AFFA2_H_ */
