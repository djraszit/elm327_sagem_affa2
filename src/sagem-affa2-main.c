/*
 * sagem-affa2-main.c
 *
 *  Created on: 23 sie 2017
 *      Author: djraszit
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>
#include <ctype.h>
#include <stdlib.h>

#include "../inc/control_bits.h"
#include "../inc/I2cbase.h"
#include "../inc/funkcje.h"
#include "../inc/sagem_affa2.h"
#include "../inc/1wire.h"
#include "../inc/1wire_basic.h"
#include "../inc/1wire_defines.h"
#include "../inc/DS18B20.h"
#include "../inc/one-wire.h"
#include "../inc/ringbuffer.h"
#include "../inc/elm327.h"
#include "../inc/main-defines.h"

#define IGNITION_LOW	0
#define IGNITION_HIGH	1

struct timer {
	uint32_t timer_ms;
	uint32_t timer_sec;
};

void init_timer_struct(struct timer * t) {
	t->timer_ms = 0;
	t->timer_sec = 0;
}

volatile struct timer global_timer;
struct timer current_timer, previous_timer;
struct timer timer_1, dspl_txt_timer, shutdown_timer;

static inline void get_timer(struct timer *timer) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		timer->timer_ms = global_timer.timer_ms;
		timer->timer_sec = global_timer.timer_sec;
	}
}

uint64_t get_total_ms(struct timer timer) {
	return (uint64_t) ((timer.timer_sec * 1000) + (uint64_t) timer.timer_ms);
}

enum {
	DISPLAY_RPM,
	DISPLAY_SPEED,
	DISPLAY_COOLANT_TEMP,
	DISPLAY_DS18B20_TEMP_IN,
	DISPLAY_DS18B20_TEMP_OUT,
	DISPLAY_VOLTAGE,
	DISPLAY_END
};
volatile uint8_t what_to_display = DISPLAY_VOLTAGE;

struct {
	volatile uint8_t works :1;
	volatile uint8_t init_ok :1;
	volatile uint8_t echo_off :2;
	volatile uint8_t linefeed_off :2;
	volatile uint8_t set_protocol :2;
	volatile uint8_t data_request :2;
	volatile uint8_t reset :2;
	volatile uint8_t last_error :4;
	volatile uint8_t current_error :4;
	volatile uint8_t protocol_op_cl :2;
} elm327;

enum {
	NOT_INITIALIZED, INITIALIZED,
};

enum {
	DENY_REENTRY, ALLOW_REENTRY
};

enum {
	ERR_NO_ERROR, ERR_UNABLE_TO_CONNECT, ERR_BUS_ERROR, ERR_NO_DATA
};

typedef struct {
	uint8_t init :1;
	uint8_t ign_pin_state :1;
	uint8_t write_text_to_sagem :1;
	uint8_t usart_new_data :1;
	uint8_t allow_reentry :1;
	uint8_t button_lock :1;
	uint8_t mrq :1;
}volatile flagIO_t;

flagIO_t * const flag = (flagIO_t*) &GPIOR0;

volatile uint8_t usart_lines;

#define RX_BUFOR_LEN	256
volatile uint8_t rx_bufor[RX_BUFOR_LEN];

#define BUFOR_ROBOCZY_LEN	64
volatile char bufor_roboczy[BUFOR_ROBOCZY_LEN];
volatile char text_to_display[64];
ringBuffer rb_rx_buffer;

volatile uint8_t sagem_buf[16];
volatile char sagem_text[128];
volatile uint8_t scroll_type = NO_SCROLL;
#define MAX_LICZNIK		500
volatile uint16_t * const licznik = (uint16_t*) &GPIOR1;
uint8_t elm327_buf[64];
volatile uint16_t button_debounce = 0;

void print_sagem_text(volatile char* text, uint8_t Scroll_Type) {
	memcpy(sagem_text, text, strlen(text));
	flag->write_text_to_sagem = 1;
	scroll_type = Scroll_Type;
}

uint8_t elm327_set_error(uint8_t error) {
	elm327.current_error = error;
	if (elm327.last_error != elm327.current_error) {
		elm327.last_error = elm327.current_error;
		return 1;
	}
	return 0;
}

//ISR(PCINT1_vect, ISR_BLOCK) {
//	if (!(MRQ_GET)) {
//		flag->mrq = 1;
//	}
//}

//volatile uint8_t * const fn = &GPIOR0;

ISR(TIMER0_COMPA_vect,ISR_NOBLOCK) {
	if (global_timer.timer_ms++ == 1000) {
		global_timer.timer_sec++;
		global_timer.timer_ms = 0;
	}

	TCNT0 = 0;
}

//ISR(TIMER1_COMPA_vect, ISR_BLOCK) {
//	if ((*licznik)++ == MAX_LICZNIK) {
//		*licznik = 0;
//	}
////	if (flag->allow_reentry == ALLOW_REENTRY) {
////		flag->allow_reentry = DENY_REENTRY;
////		if (flag->write_text_to_sagem) {
////			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
////			{
////				sagem_write_text(sagem_text, scroll_type);
////				flag->write_text_to_sagem = 0;
////				TCNT1 = 1;
////			}
////		}
////
////		flag->allow_reentry = ALLOW_REENTRY;
////	}
//}

ISR(USART_RX_vect,ISR_BLOCK) {
	SET(PORT, USART_COMM_LED);
	uint8_t data = UDR0;
	if (data == '\r') {
		usart_lines++;
	}
	if (data == '>') {
		flag->usart_new_data = true;
		goto exit;
	}
	writeBuf(&rb_rx_buffer, data);
	exit:
	CLR(PORT, USART_COMM_LED);
}

ISR(INT0_vect, ISR_BLOCK) {
	if (!(GET(BUTTON_PIN))) {
		what_to_display++;
		if (what_to_display >= DISPLAY_END) {
			what_to_display = 0;
		}
	}
}

inline void set_timer(uint16_t value) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		*licznik = value;
	}
}

//void timer_delay(uint16_t value) { //value * 10ms
//	uint16_t l = 0;
//	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//	{
//		*licznik = 0;
//	}
//	while (l < value) {
//		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//		{
//			l = *licznik;
//		}
//	}
//}

void low_power_mode() {
	NONATOMIC_BLOCK(NONATOMIC_FORCEOFF);
	CLR(PORT, LCD_ON_OFF);
	flag->write_text_to_sagem = false;
	flag->init = NOT_INITIALIZED;
	CLKPR = (1 << CLKPCE);
	CLKPR = (8 << CLKPS0);

}

int main() {
	flag->allow_reentry = ALLOW_REENTRY;
	flag->button_lock = ALLOW_REENTRY;
	usart_lines = 0;
	usart_lines = 0;
	rb_rx_buffer.size = RX_BUFOR_LEN; //inicjacja bufora cyklicznego
	rb_rx_buffer.data = rx_bufor;

//	TCCR1B = (1 << WGM12) | (1 << CS12); //prescaler /256
//	OCR1A = 62500 / 100;
//	TIMSK1 = (1 << OCIE1A);

	SET(DDR, USART_COMM_LED);//dioda sygnalizująca komunikacje USART
	CLR(DDR, IGNITION_PIN); //ignition pin jako wejscie

	CLR(DDR, BUTTON_PIN);
	EICRA |= (1 << ISC01); //przerwania od button
	EICRA &= ~(1 << ISC00);
	EIMSK |= (1 << INT0);

	//adc init
	ADCSRA = (1 << ADEN) | (7 << ADPS0);

	usart_init();

//	OW_init();
//	OneWireScan();

	I2C_Init();
	sagem_affa2_init();

	//set timer
	TCCR0A = 0;
	TCCR0B = (1 << CS00) | (1 << CS01);
	OCR0A = 250;
	TIMSK0 = (1 << OCIE0A);
	global_timer.timer_sec = 0;
	global_timer.timer_ms = 0;
	init_timer_struct(&current_timer);
	init_timer_struct(&previous_timer);
	init_timer_struct(&timer_1);
	init_timer_struct(&shutdown_timer);
	flag->ign_pin_state = IGNITION_LOW;
	sei();
	print_sagem_text("init", NO_SCROLL);

//	uint16_t l; //kopia zmiennej licznik
//	uint8_t wtd; //kopia zmiennej what to display

	low_power_mode();
	while (1) {
		if (flag->init == INITIALIZED) {
			if (flag->write_text_to_sagem == 1) {
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					sagem_write_text(sagem_text, scroll_type);
					flag->write_text_to_sagem = 0;
				}
			}
			if (!(MRQ_GET)) {
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					sagem_buf[0] = 0x01;
					sagem_buf[1] = 0x11;
					sagem_write((uint8_t*) sagem_buf);
					sagem_read((uint8_t*) sagem_buf);
				}
#ifdef DEBUG_MRQ_REQ

				usart_print_hex("MRQ_GET read", (uint8_t*) sagem_buf, sagem_buf[0] + 1);

#endif
			}

		}
		SET(PORT, USART_COMM_LED);
		CLR(PORT, USART_COMM_LED);
		if (GET(IGNITION_PIN)) {
			if (flag->ign_pin_state == IGNITION_LOW) {
				flag->ign_pin_state = IGNITION_HIGH;
			}
			if (flag->init == NOT_INITIALIZED) {
				CLKPR = (1 << CLKPCE);
				CLKPR = 0x00;
				init_timer_struct(&previous_timer);
				init_timer_struct(&current_timer);

				sagem_affa2_init();
				sagem_affa2_channel(0x40);
				ATOMIC_BLOCK(ATOMIC_FORCEON)
				{
					global_timer.timer_sec = 0;
					global_timer.timer_ms = 0;
					flag->init = INITIALIZED;
				}
				continue;
			} else {
				get_timer(&current_timer);
				if (get_total_ms(current_timer) > (get_total_ms(previous_timer) + 100)) {

					uint32_t adc = (uint32_t) adc_average(0) * 1870;
					uint32_t voltage = adc / 1024;
					uint32_t x = voltage - ((voltage / 100) * 100);
					sprintf((char*) text_to_display, "    %03lu", voltage);
					sagem_affa2_channel(0x00);
					print_sagem_text(text_to_display, NO_SCROLL);
					get_timer(&previous_timer);
				}
				//				if (get_total_ms(current_timer) > (get_total_ms(timer_1) + 500)) {
				//					print_sagem_text("deadbeef", NO_SCROLL);
				//					get_timer(&timer_1);
				//				}

				//		uint16_t sp = SP;
				//		sprintf((char*) bufor_roboczy, "SP %u  0x%04X\n\r", sp, sp);
				//		usart_send_string((char*) bufor_roboczy);
			}


		} else {
			if (flag->init == INITIALIZED) {
				get_timer(&current_timer);
				if (flag->ign_pin_state == IGNITION_HIGH) {
					flag->ign_pin_state = IGNITION_LOW;
					get_timer(&shutdown_timer);
					sagem_affa2_channel(0x40);
					print_sagem_text("SHUTDOWN", NO_SCROLL);
				}
				if (flag->ign_pin_state == IGNITION_LOW) {
					if (get_total_ms(current_timer) > (get_total_ms(shutdown_timer) + 4000)) {
						low_power_mode();
					}
				}
			} else {

			}
		}

	}

////////////////////////////////////////////

	/*
	 elm327_send_at_cmd(ELM327_RESET);

	 timer_delay(100);

	 elm327_send_at_cmd(ELM327_LOW_POWER_MODE);

	 timer_delay(25);

	 low_power_mode();

	 while (1) {
	 loop_start: get_timer(&current_timer);
	 if (GET(IGNITION_PIN)) {

	 if (flag->init == NOT_INITIALIZED) {
	 CLKPR = (1 << CLKPCE);
	 CLKPR = 0x00;
	 sagem_affa2_init();

	 sagem_affa2_channel(0x40);
	 sagem_write_text("INIT    ", NO_SCROLL);
	 ATOMIC_BLOCK(ATOMIC_FORCEON)
	 {
	 memset((uint8_t*) rx_bufor, 0, RX_BUFOR_LEN);
	 memset((uint8_t*) bufor_roboczy, 0, BUFOR_ROBOCZY_LEN);
	 }
	 timer_delay(100);
	 elm327.reset = ATCMD_REQUEST;
	 set_timer(0);
	 flag->init = INITIALIZED;
	 continue;
	 } else {
	 if (flag->ign_pin_change == true) {
	 flag->ign_pin_change = false;

	 sagem_write_text("INIT    ", NO_SCROLL);
	 }
	 }

	 if (elm327.reset == ATCMD_REQUEST) {
	 elm327_send_at_cmd(ELM327_RESET);
	 elm327.reset = ATCMD_SENT;
	 continue;
	 }

	 ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	 {
	 wtd = what_to_display;
	 l = *licznik;
	 }

	 if (!(MRQ_GET)) {
	 ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	 {
	 sagem_buf[0] = 0x01;
	 sagem_buf[1] = 0x11;
	 sagem_write((uint8_t*) sagem_buf);
	 sagem_read((uint8_t*) sagem_buf);
	 }
	 #ifdef DEBUG_MRQ_REQ

	 usart_print_hex("MRQ_GET read", (uint8_t*) sagem_buf, sagem_buf[0] + 1);

	 #endif
	 }

	 if (flag->usart_new_data) {
	 flag->usart_new_data = 0;

	 do {
	 usart_lines--;
	 uint8_t size = copy_line_from_buffer(&rb_rx_buffer, (char*) bufor_roboczy,
	 '\r');
	 if (size < 2) {
	 continue;
	 }
	 #ifdef DEBUG
	 usart_print_hex("bufor roboczy", (uint8_t*) bufor_roboczy, size);
	 usart_print_hex("rx bufor", (uint8_t*) rx_bufor, 128);
	 #endif
	 if (elm327.data_request == DATA_SENT) {
	 if (strncmp((char*) bufor_roboczy, "UNABLE TO CONNECT", 17) == 0) {

	 if (elm327_set_error(ERR_UNABLE_TO_CONNECT)) {
	 sagem_affa2_channel(0x40);
	 sprintf((char*) text_to_display, "UNABLE TO CONNECT");
	 goto print_on_lcd;
	 }
	 elm327.data_request = DATA_REQUEST;
	 continue;
	 }

	 if (strncmp((char*) bufor_roboczy, "BUS INIT: BUS ERROR", 19) == 0) {

	 if (elm327_set_error(ERR_BUS_ERROR)) {
	 sagem_affa2_channel(0x40);
	 sprintf((char*) text_to_display, "BUS ERROR");
	 goto print_on_lcd;
	 }
	 elm327.data_request = DATA_REQUEST;
	 continue;
	 }

	 if (strncmp((char*) bufor_roboczy, "NO DATA", 7) == 0) {
	 if (elm327_set_error(ERR_NO_DATA)) {
	 sagem_affa2_channel(0x40);
	 sprintf((char*) text_to_display, "NO DATA");
	 goto print_on_lcd;
	 }
	 elm327.data_request = DATA_REQUEST;
	 continue;
	 }

	 //						if (wtd == DISPLAY_VOLTAGE) {
	 //							int voltage = (int) (atof((char*) bufor_roboczy) * 10);
	 //							sprintf((char*) text_to_display, "    %03dV", voltage);
	 //							sagem_affa2_channel(0x00);
	 //							goto print_on_lcd;
	 //						}

	 parsehex_to_array((char*) bufor_roboczy, 1, elm327_buf, 64);

	 elm327_calculate_data(elm327_buf, (char*) text_to_display);

	 sagem_affa2_channel(0x40);

	 print_on_lcd: print_sagem_text(text_to_display, NO_SCROLL);

	 elm327.data_request = DATA_REQUEST;

	 continue;

	 }
	 if (strstr((char*) bufor_roboczy, "OK")) {
	 if (elm327.echo_off == ATCMD_SENT) {
	 elm327.echo_off = ATCMD_OK;
	 elm327.linefeed_off = ATCMD_REQUEST;
	 sagem_affa2_channel(0x40);
	 print_sagem_text("echo off", NO_SCROLL);
	 }
	 if (elm327.linefeed_off == ATCMD_SENT) {
	 elm327.linefeed_off = ATCMD_OK;
	 elm327.set_protocol = ATCMD_REQUEST;
	 sagem_affa2_channel(0x40);
	 print_sagem_text("linefeed off", NO_SCROLL);
	 }
	 if (elm327.set_protocol == ATCMD_SENT) {
	 elm327.set_protocol = ATCMD_OK;
	 elm327.data_request = DATA_REQUEST;
	 sagem_affa2_channel(0x40);
	 print_sagem_text("protocol set", NO_SCROLL);
	 }

	 if (elm327.init_ok == false) {
	 if (elm327.echo_off == ATCMD_OK && elm327.linefeed_off == ATCMD_OK
	 && elm327.set_protocol == ATCMD_OK) {
	 elm327.init_ok = true;
	 }
	 }
	 if (elm327.protocol_op_cl == PROTOCOL_CLOSE) {

	 }

	 continue;
	 }

	 if (strncmp((char*) bufor_roboczy, "\nELM327 v1.5", 12) == 0) {
	 elm327.works = 1;
	 //						elm327.set_protocol = ATCMD_REQUEST;
	 elm327.echo_off = ATCMD_REQUEST;

	 continue;
	 }
	 } while (usart_lines > 0);
	 set_timer(0);
	 continue;
	 }

	 if (wtd < DISPLAY_DS18B20_TEMP_IN && l > 10) {
	 if (elm327.works && !elm327.init_ok) {

	 if (elm327.echo_off == ATCMD_REQUEST) {
	 elm327_send_at_cmd(ELM327_ECHO "0");
	 elm327.echo_off = ATCMD_SENT;
	 goto kontynuuj;
	 }
	 if (elm327.linefeed_off == ATCMD_REQUEST) {
	 elm327_send_at_cmd(ELM327_LINEFEED "0");
	 elm327.linefeed_off = ATCMD_SENT;
	 goto kontynuuj;
	 }
	 if (elm327.set_protocol == ATCMD_REQUEST) {
	 elm327_send_at_cmd(ELM327_SET_PROTOCOL "5");
	 elm327.set_protocol = ATCMD_SENT;
	 goto kontynuuj;
	 }
	 kontynuuj: set_timer(0);
	 continue;
	 }

	 if (elm327.data_request == DATA_REQUEST) {

	 int pid = 0;
	 if (wtd == DISPLAY_RPM) {
	 pid = ENGINE_RPM;
	 goto request;
	 }
	 if (wtd == DISPLAY_SPEED) {
	 pid = VEHICLE_SPEED;
	 goto request;
	 }
	 if (wtd == DISPLAY_COOLANT_TEMP) {
	 pid = ENGINE_COOLANT_TEMP;
	 goto request;
	 }
	 //					if (wtd == DISPLAY_VOLTAGE) {
	 //						elm327_send_at_cmd(ELM327_READ_INPUT_VOLTAGE);
	 //						elm327.data_request = DATA_SENT;
	 //						set_timer(0);
	 //						continue;
	 //					}
	 request: elm327_request(show_current_data, pid, END_ARG);
	 elm327.data_request = DATA_SENT;
	 set_timer(0);
	 continue;
	 }
	 }

	 if (wtd == DISPLAY_VOLTAGE) {
	 if (l == 10) {
	 uint32_t adc = (uint32_t) adc_average(0) * 2090;
	 uint32_t voltage = adc / 1024;
	 uint32_t x = voltage - ((voltage / 100) * 100);
	 sprintf((char*) text_to_display, "    %03lu", voltage);
	 sagem_affa2_channel(0x00);
	 sagem_write_text(text_to_display, NO_SCROLL);
	 set_timer(0);
	 }
	 //				uint16_t sp = SP;
	 //				sprintf((char*) bufor_roboczy, "SP %u  0x%04X\n\r", sp, sp);
	 //				usart_send_string((char*) bufor_roboczy);
	 }
	 if (wtd >= DISPLAY_DS18B20_TEMP_IN) {
	 elm327_set_error(ERR_NO_ERROR);
	 }
	 if (wtd == DISPLAY_DS18B20_TEMP_IN) {
	 if (l == 25) {
	 set_timer(26);
	 OW_Start_Conversion(0);
	 continue;
	 }
	 if (l == 50) {
	 set_timer(0);
	 int temp = (int) (DS18B20_gettemp(0) * 10);
	 sprintf((char*) text_to_display, "in %+.3d\x1d", temp);
	 sagem_affa2_channel(0x12);
	 print_sagem_text(text_to_display, NO_SCROLL);
	 continue;
	 }
	 continue;
	 }
	 if (wtd == DISPLAY_DS18B20_TEMP_OUT) {
	 if (l == 25) {
	 set_timer(26);
	 OW_Start_Conversion(0);
	 continue;
	 }
	 if (l == 50) {
	 set_timer(0);
	 int temp = (int) (DS18B20_gettemp(1) * 10);
	 sprintf((char*) text_to_display, "out%+.3d\x1d", temp);
	 sagem_affa2_channel(0x12);
	 print_sagem_text(text_to_display, NO_SCROLL);
	 continue;
	 }
	 continue;
	 }

	 } else {
	 if (flag->init == INITIALIZED) {
	 flag->ign_pin_change = true;

	 sagem_affa2_channel(0x40);

	 print_sagem_text("SHUTDOWN", NO_SCROLL);

	 elm327.last_error = ERR_NO_ERROR;
	 timer_delay(2);
	 //				if (flag->init == INITIALIZED) {
	 set_timer(0);
	 uint16_t l1 = 0;
	 while (l1 < 400) {
	 if (!(MRQ_GET)) {

	 ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	 {
	 sagem_buf[0] = 0x01;
	 sagem_buf[1] = 0x11;
	 sagem_write((uint8_t*) sagem_buf);
	 sagem_read((uint8_t*) sagem_buf);
	 }
	 #ifdef DEBUG_MRQ_REQ

	 usart_print_hex("MRQ_GET read", (uint8_t*) sagem_buf, sagem_buf[0] + 1);

	 #endif
	 }
	 ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	 {
	 l1 = *licznik;
	 }
	 if (GET(IGNITION_PIN)) {
	 goto loop_start;
	 }
	 }
	 //			}

	 elm327.init_ok = false;
	 elm327.works = false;
	 elm327.reset = ATCMD_NOT_SENT;
	 elm327.data_request = DATA_REQUEST_NOT_SENT;

	 flag->init = NOT_INITIALIZED;

	 elm327_send_at_cmd(ELM327_LOW_POWER_MODE);

	 timer_delay(25);

	 low_power_mode();
	 }
	 }
	 } */
}
