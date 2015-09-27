#include <stdio.h>
#include <util/delay.h>

#include "uart.h"

#define PAUSE_DELAY_MS 1000

int main(void) {    

    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
                
    while (1) {
        if (puts("Hello world!") == 0) {
            break;
        }
        _delay_ms(PAUSE_DELAY_MS);
    }
    return 0;
}
