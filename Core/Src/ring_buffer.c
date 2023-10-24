#include "ring_buffer.h"
#include <string.h>
#include <usart.h>

#define TIMEOUT_DEF 500

uint16_t timeout;

ring_buffer *_rx_buffer;
ring_buffer *_tx_buffer;

void store_char(unsigned char c, ring_buffer *buffer);

void ring_buffer_init(void) {
  ring_buffer rx_buffer = { { 0 }, 0, 0};
  ring_buffer tx_buffer = { { 0 }, 0, 0};

  _rx_buffer = &rx_buffer;
  _tx_buffer = &tx_buffer;
}

void store_char(unsigned char c, ring_buffer *buffer) {
  int i = (unsigned int)(buffer->head + 1) % UART_BUFFER_SIZE;

  if(i != buffer->tail) {
    buffer->buffer[buffer->head] = c;
    buffer->head = i;
  }
}

int buf_read(void) {
  if(_rx_buffer->head == _rx_buffer->tail) {
    return -1;
  } else {
    unsigned char c = _rx_buffer->buffer[_rx_buffer->tail];
    _rx_buffer->tail = (unsigned int)(_rx_buffer->tail + 1) % UART_BUFFER_SIZE;
    return c;
  }
}

void buf_write(int c) {
	if (c>=0) {
		int i = (_tx_buffer->head + 1) % UART_BUFFER_SIZE;
		while (i == _tx_buffer->tail);

		_tx_buffer->buffer[_tx_buffer->head] = (uint8_t)c;
		_tx_buffer->head = i;
	}
}

int data_available(void) {
  return (uint16_t)(UART_BUFFER_SIZE + _rx_buffer->head - _rx_buffer->tail) % UART_BUFFER_SIZE;
}

void buf_sendstring (const char *s) {
	while(*s) buf_write(*s++);
}
