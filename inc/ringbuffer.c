/*
 * ringbuffer.c
 *
 *  Created on: 4 lip 2017
 *      Author: djraszit
 */

#include <avr/io.h>
#include "ringbuffer.h"

void writeBuf(ringBuffer *rb, char inputData) {
	rb->data[rb->head] = inputData;
	rb->head++;
	if (rb->head == rb->size) {
		rb->head = 0;
	}
}

char readBuf(ringBuffer *rb) {
	char data = rb->data[rb->tail];
	rb->tail++;
	if (rb->tail == rb->size) {
		rb->tail = 0;
	}
	return data;
}

char is_empty(ringBuffer *rb) {
	return rb->head - rb->tail;
}

char is_full(ringBuffer *rb) {
	if ((rb->head - rb->tail) >= rb->size) {
		return 1;
	} else {
		return 0;
	}
}

void copyBuf(ringBuffer *src, ringBuffer *dst){
	while((src->head - src->tail)){
		writeBuf(dst, readBuf(src));
	}
}

uint8_t copy_line_from_buffer(ringBuffer *rb, char *str, char exit_char) {
	uint8_t count = 0;
	while (rb->head - rb->tail) {
		str[count] = readBuf(rb);
		if (str[count] == exit_char) {
			str[count] = 0;
			break;
		}
		count++;
	}
	return count+1;
}
