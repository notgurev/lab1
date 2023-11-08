#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32f4xx_hal.h"

/* change the size of the buffer */
#define UART_BUFFER_SIZE 64

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;


/* Initialize the ring buffer */
void Ringbuf_init(void);

/* reads the data in the rx_buffer and increment the tail count in rx_buffer */
int uart_read(void);

/* writes the data to the tx_buffer and increment the head count in tx_buffer */
void uart_write(int c);

/* function to send the string to the uart */
void uart_sendstring(const char *s);


/* Checks if the data is available to read in the rx_buffer */
int is_data_available(void);


/* the ISR for the uart. put it in the IRQ handler */
void uart_isr (UART_HandleTypeDef *huart);



/*** Depreciated For now. This is not needed, try using other functions to meet the requirement ***/
/* get the position of the given string within the incoming data.
 * It returns the position, where the string ends
 */
//uint16_t Get_position (char *string);

/* once you hit 'enter' (\r\n), it copies the entire string to the buffer*/
//void Get_string (char *buffer);



#endif /* UARTRINGBUFFER_H_ */
