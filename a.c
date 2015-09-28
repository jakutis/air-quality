#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "uart.h"

#define PAUSE_DELAY_MS 1000
#define BLINK_DELAY_MS 100

void blink_init() {
    /* set pin 5 of PORTB for output*/
    DDRB |= _BV(DDB5);
}

void blink(int times) {
    for(int i = 0; i < times; i++) {
        /* set pin 5 high to turn led on */
        PORTB |= _BV(PORTB5);
        _delay_ms(BLINK_DELAY_MS);

        /* set pin 5 low to turn led off */
        PORTB &= ~_BV(PORTB5);
        _delay_ms(BLINK_DELAY_MS);
    }
}

int main(void) {    

    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
                
    int input;
    int error_code = 0;

    while (1) {
        if (scanf("%d", &input) != 1) {
            error_code = 1;
            break;
        }
        if (printf("%d", input + 1) == EOF) {
            error_code = 2;
            break;
        }
    }

    blink_init();
    while (1) {
        blink(error_code);
        _delay_ms(PAUSE_DELAY_MS);
    }
    return 0;
}
