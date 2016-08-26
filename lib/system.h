#include <stdio.h>

void system_init(void);
uint32_t system_micros(void);
void system_uart_putchar(char c, FILE *stream);
char system_uart_getchar(FILE *stream);
FILE system_uart_output = FDEV_SETUP_STREAM(system_uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE system_uart_input = FDEV_SETUP_STREAM(NULL, system_uart_getchar, _FDEV_SETUP_READ);
