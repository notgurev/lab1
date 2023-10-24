#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32f4xx_hal.h"

#define UART_BUFFER_SIZE 64

typedef struct {
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;


// Initialize ring buffer
void ring_buffer_init(void);

/* reads the data in the rx_buffer and increment the tail count in rx_buffer */
// Read data from rx_buffer and increment its tail count
int buf_read(void);

// Write data to tx_buffer and increment its head count
void buf_write(int c);

// Send string to UART
void buf_sendstring(const char *s);

// Check if the data is available to read from rx_buffer
int data_available(void);

#endif /* UARTRINGBUFFER_H_ */
