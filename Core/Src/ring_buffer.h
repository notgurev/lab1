#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32f4xx_hal.h"

#define UART_BUFFER_SIZE 64

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;


/* initialize the ring buffer */
void ring_buffer_init(void);

/* reads the data in the rx_buffer and increment the tail count in rx_buffer */
int buf_read(void);

/* writes the data to the tx_buffer and increment the head count in tx_buffer */
void buf_write(int c);

/* function to send the string to the uart */
void buf_sendstring(const char *s);

/* checks if the data is available to read in the rx_buffer */
int data_available(void);

#endif /* UARTRINGBUFFER_H_ */
