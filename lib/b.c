#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "micros.h"
#include "uart.h"

void blink() {
  DDRB |= (1u << 5);
  PORTB = 1u << 5;
  _delay_ms(100);
  PORTB = 0;
  _delay_ms(100);
}

const uint8_t STATE_ERROR = 0;
const uint8_t STATE_VYTAS = 1;

void writeD(uint8_t pin, uint8_t newValue) {
  uint8_t pinMask = 1u << pin;

  DDRD |= pinMask;
  uint8_t oldValue = (PORTD & pinMask) >> pin;

  if (newValue != oldValue) {
    PORTD ^= pinMask;
  }
}

void pullUpD(uint8_t pin) {
  uint8_t pinMask = 1u << pin;
  if ((DDRD & pinMask) >> pin) {
    DDRD ^= pinMask;
  }

  PORTD |= pinMask;
}

uint8_t readD(uint8_t pin) {
  return (PIND & (1u << pin)) >> pin;
}

uint8_t blink_an_error() {
  for (int i = 0; i < 3; i += 1) {
    blink();
  }
  _delay_ms(1000);

  return STATE_ERROR;
}

/*
 * TIMSK1 |= 1 << TOIE1;
 * TCCR1B |= (1 << CS10);
 *
 * 2^4 * 1 000 000 ticks per 1 second
 * 16 ticks per 1 microsecond
 * 1 overflow per 2^16 ticks
 * 244 overflows per 1000 milliseconds
 * 1 overflow per 4 milliseconds
 */

volatile uint32_t timer1_micros = 0;
volatile uint32_t timer1_overflows = 0;
uint64_t prev = 0;
uint8_t vytas() {
  /*
  writeD(2, 0);
  _delay_ms(20);
  pullUpD(2, 0);
  */

  uint64_t value;
  /*
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    value = timer1_micros / 1000000u;
  }
  */
  if (printf("%lu\n", 1) == EOF) {
    return STATE_ERROR;
  }
  prev = value;
  _delay_ms(1000u);
  return STATE_VYTAS;
}

/*
ISR(TIMER1_COMPA_vect) {
  timer1_micros++;
  return;
  if (timer1_micros % 1000000 == 0) {
    blink();
  }
  timer1_micros = !timer1_micros;
  PORTB = timer1_micros ? 0 : 1u << PB5;

  timer1_micros += 1;
  if (timer1_micros % 1000 == 0) {
    blink();
  }
}

ISR(TIMER1_OVF_vect) {
  timer1_overflows += 1;
}
*/

void init() {
  uart_init();
  stdout = &uart_output;
  stdin  = &uart_input;
  return;

  cli();// turn off interrupt mechanism
  OCR1A = 64;// set interruption interval to 1 microsecond = (16000000 Hz * (1 / 1000000) s)
  TCCR1A = 0;// reset A flags
  TCCR1B = 0;// reset B flags
  TCCR1B |= (1 << WGM12);// enable CTC mode
  TCCR1B |= (1 << CS10);// enable 1 prescaler
  TIMSK1 = 0;// reset interruption flags
  TIMSK1 |= (1 << OCIE1A);// enable TIMER1_COMPA_vect interrupt
  sei();// turn on interrupt mechanism
  return;

  TCCR1B |= (1 << CS10);
  TIMSK1 |= 1 << TOIE1;
  sei();
  return;

  //micros_init();

  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = (1 << TOIE1);
  TCCR1B |= (1 << CS10);
  sei();

}

int check() {
  uint64_t a = 0;
  a--;
  if (sizeof(a) != 8) {
    return 0;
  }
  if (a != 18446744073709551615u) {
    return 0;
  }
  return 1;
}

int main() {
  uint8_t state = check() ? STATE_VYTAS : STATE_ERROR;
  uint32_t timeout;
  uint32_t cycles;
  uint32_t N = 50;
  uint32_t high_durations[N];
  uint32_t low_durations[N];
  uint8_t data[40];
  uint32_t start;
  init_arduino();
  init();

  while (1) {
    printf("demo start\n");
    writeD(2, 0);
    start = micros();
    while ((micros() - start) < 20000);

    pullUpD(2);

    cycles = 0;
    while (1) {
      start = micros();
      while (readD(2) && !(timeout = ((micros() - start) > 1000000)));
      if (timeout) {
        break;
      }
      high_durations[cycles] = micros() - start;
      //printf("became LOW in %" PRIu32 "\n", micros() - start);

      start = micros();
      while (!readD(2) && !(timeout = ((micros() - start) > 1000000)));
      if (timeout) {
        break;
      }
      low_durations[cycles] = micros() - start;
      //printf("became HIGH in %" PRIu32 "\n", micros() - start);
      cycles++;
    }
    printf("LOW and HIGH cycles: %" PRIu32 "\n", cycles);
    for(uint32_t i = 0; i < N; i++) {
      printf("durations for #%" PRIu32 " cycle are: high=%" PRIu32 " low=%" PRIu32 "\n", i, high_durations[i], low_durations[i]);
    }
    uint16_t h = 0;
    uint16_t t = 0;
    for(uint32_t i = 0; i < 40; i++) {
      data[i] = high_durations[i + 2] > 50;
      printf("%" PRIu8 "\n", data[i]);
      if (i < 16) {
        h |= data[i] << (15 - (i));
      } else if (i < 32) {
        t |= data[i] << (15 - (i - 16));
      }
    }
    printf("humidity: %" PRIu16 "\n", h);
    printf("temperature: %" PRIu16 "\n", t);

    start = micros();
    while (micros() - start < 5000000);
  }

  while (1) {
    uint32_t start = micros();
    while (micros() - start < 1000000);
    DDRB |= (1u << 5);
    PORTB ^= 1u << 5;
    printf("second %" PRIu32 "\n", start);

    continue;
    if (state == STATE_VYTAS) {
      state = vytas();
    }
    if (state == STATE_ERROR) {
      state = blink_an_error();
    }
  }

  return 0;
}
