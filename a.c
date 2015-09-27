#include <stdio.h>
#include "uart.h"

int main(void) {    

    uart_init();
    stdout = &uart_output;
    stdin  = &uart_input;
                
    int input;

    while (1) {
        if (puts("Enter integer:") == 0) {
            break;
        }
        if (scanf("%d", &input) == EOF) {
            break;
        }
        if (puts("Your integer, incremented:") == 0) {
            break;
        }
        if (printf("%d\n", input + 1) == EOF) {
            break;
        }
    }
    return 0;
}
