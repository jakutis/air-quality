#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "system.h"

void blink() {
  DDRB |= (1u << PB5);
  PORTB = 1u << PB5;
  _delay_ms(100);
  PORTB = 0;
  _delay_ms(100);
}

uint8_t decode(uint8_t * bits) {
  return (bits[0] << 7)
    | (bits[1] << 6)
    | (bits[2] << 5)
    | (bits[3] << 4)
    | (bits[4] << 3)
    | (bits[5] << 2)
    | (bits[6] << 1)
    | (bits[7] << 0);
}

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

const uint8_t STATE_BLINK_FAILURE = 0;
const uint8_t STATE_READ_SENSORS = 1;

uint8_t state_blink_failure() {
  for (int i = 0; i < 3; i += 1) {
    blink();
  }
  _delay_ms(1000);

  return STATE_BLINK_FAILURE;
}

uint8_t state_read_sensors() {
  uint32_t timeout;
  uint8_t data[40];
  uint32_t start;
  uint32_t cycles;
  uint16_t h = 0;
  uint16_t t = 0;
  uint8_t ce = 0;
  uint8_t ca = 0;

  writeD(2, 0);
  start = system_micros();
  while ((system_micros() - start) < 20000);

  pullUpD(2);

  for(cycles = 0; cycles < 42; cycles += 1) {
    start = system_micros();
    while (readD(2) && !(timeout = ((system_micros() - start) > 1000000)));
    if (timeout) {
      break;
    }
    if (cycles >= 2) {
      data[cycles - 2] = (system_micros() - start) > 50;
    }

    start = system_micros();
    while (!readD(2) && !(timeout = ((system_micros() - start) > 1000000)));
    if (timeout) {
      break;
    }
  }

  if (cycles < 42) {
    return STATE_BLINK_FAILURE;
  }

  h = decode(data);
  h = (h << 8) | decode(data + 8);
  t = decode(data + 16);
  t = (t << 8) | decode(data + 24);
  ce = decode(data + 32);
  ca = decode(data) + decode(data + 8) + decode(data + 16) + decode(data + 24);

  fprintf(&system_uart_output, "{\"humidity\":%" PRIu16 ",\"temperature\":%" PRIu16 "}\n", h, t);

  if (ce != ca) {
    return STATE_BLINK_FAILURE;
  }

  start = system_micros();
  while (system_micros() - start < 350000);

  return STATE_READ_SENSORS;
}

int main() {
  system_init();
  uint8_t state = STATE_READ_SENSORS;

  while (1) {
    if (state == STATE_READ_SENSORS) {
      state = state_read_sensors();
    }
    if (state == STATE_BLINK_FAILURE) {
      state = state_blink_failure();
    }
  }

  return 0;
}
