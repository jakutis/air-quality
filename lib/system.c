#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
volatile unsigned long timer0_overflow_count = 0;

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{
  timer0_overflow_count++;
}

void system_uart_init(void) {
  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;

#if USE_2X
  UCSR0A |= _BV(U2X0);
#else
  UCSR0A &= ~(_BV(U2X0));
#endif

  UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */ 
  UCSR0B = _BV(RXEN0) | _BV(TXEN0);   /* Enable RX and TX */    
}

void system_uart_putchar(char c, FILE *stream) {
  if (c == '\n') {
    system_uart_putchar('\r', stream);
  }
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
}

char system_uart_getchar(FILE *stream) {
  loop_until_bit_is_set(UCSR0A, RXC0);
  return UDR0;
}

void system_timers_init(void) {
  // this needs to be called before setup() or some functions won't
  // work there
  sei();
  
  // on the ATmega168, timer 0 is also used for fast hardware pwm
  // (using phase-correct PWM would mean that timer 0 overflowed half as often
  // resulting in different millis() behavior on the ATmega8 and ATmega168)
#if defined(TCCR0A) && defined(WGM01)
  sbi(TCCR0A, WGM01);
  sbi(TCCR0A, WGM00);
#endif

  // set timer 0 prescale factor to 64
#if defined(__AVR_ATmega128__)
  // CPU specific: different values for the ATmega128
  sbi(TCCR0, CS02);
#elif defined(TCCR0) && defined(CS01) && defined(CS00)
  // this combination is for the standard atmega8
  sbi(TCCR0, CS01);
  sbi(TCCR0, CS00);
#elif defined(TCCR0B) && defined(CS01) && defined(CS00)
  // this combination is for the standard 168/328/1280/2560
  sbi(TCCR0B, CS01);
  sbi(TCCR0B, CS00);
#elif defined(TCCR0A) && defined(CS01) && defined(CS00)
  // this combination is for the __AVR_ATmega645__ series
  sbi(TCCR0A, CS01);
  sbi(TCCR0A, CS00);
#else
  #error Timer 0 prescale factor 64 not set correctly
#endif

  // enable timer 0 overflow interrupt
#if defined(TIMSK) && defined(TOIE0)
  sbi(TIMSK, TOIE0);
#elif defined(TIMSK0) && defined(TOIE0)
  sbi(TIMSK0, TOIE0);
#else
  #error  Timer 0 overflow interrupt not set correctly
#endif

  // timers 1 and 2 are used for phase-correct hardware pwm
  // this is better for motors as it ensures an even waveform
  // note, however, that fast pwm mode can achieve a frequency of up
  // 8 MHz (with a 16 MHz clock) at 50% duty cycle

#if defined(TCCR1B) && defined(CS11) && defined(CS10)
  TCCR1B = 0;

  // set timer 1 prescale factor to 64
  sbi(TCCR1B, CS11);
#if F_CPU >= 8000000L
  sbi(TCCR1B, CS10);
#endif
#elif defined(TCCR1) && defined(CS11) && defined(CS10)
  sbi(TCCR1, CS11);
#if F_CPU >= 8000000L
  sbi(TCCR1, CS10);
#endif
#endif
  // put timer 1 in 8-bit phase correct pwm mode
#if defined(TCCR1A) && defined(WGM10)
  sbi(TCCR1A, WGM10);
#endif

  // set timer 2 prescale factor to 64
#if defined(TCCR2) && defined(CS22)
  sbi(TCCR2, CS22);
#elif defined(TCCR2B) && defined(CS22)
  sbi(TCCR2B, CS22);
//#else
  // Timer 2 not finished (may not be present on this CPU)
#endif

  // configure timer 2 for phase correct pwm (8-bit)
#if defined(TCCR2) && defined(WGM20)
  sbi(TCCR2, WGM20);
#elif defined(TCCR2A) && defined(WGM20)
  sbi(TCCR2A, WGM20);
//#else
  // Timer 2 not finished (may not be present on this CPU)
#endif

#if defined(TCCR3B) && defined(CS31) && defined(WGM30)
  sbi(TCCR3B, CS31);    // set timer 3 prescale factor to 64
  sbi(TCCR3B, CS30);
  sbi(TCCR3A, WGM30);    // put timer 3 in 8-bit phase correct pwm mode
#endif

#if defined(TCCR4A) && defined(TCCR4B) && defined(TCCR4D) /* beginning of timer4 block for 32U4 and similar */
  sbi(TCCR4B, CS42);    // set timer4 prescale factor to 64
  sbi(TCCR4B, CS41);
  sbi(TCCR4B, CS40);
  sbi(TCCR4D, WGM40);    // put timer 4 in phase- and frequency-correct PWM mode  
  sbi(TCCR4A, PWM4A);    // enable PWM mode for comparator OCR4A
  sbi(TCCR4C, PWM4D);    // enable PWM mode for comparator OCR4D
#else /* beginning of timer4 block for ATMEGA1280 and ATMEGA2560 */
#if defined(TCCR4B) && defined(CS41) && defined(WGM40)
  sbi(TCCR4B, CS41);    // set timer 4 prescale factor to 64
  sbi(TCCR4B, CS40);
  sbi(TCCR4A, WGM40);    // put timer 4 in 8-bit phase correct pwm mode
#endif
#endif /* end timer4 block for ATMEGA1280/2560 and similar */  

#if defined(TCCR5B) && defined(CS51) && defined(WGM50)
  sbi(TCCR5B, CS51);    // set timer 5 prescale factor to 64
  sbi(TCCR5B, CS50);
  sbi(TCCR5A, WGM50);    // put timer 5 in 8-bit phase correct pwm mode
#endif

#if defined(ADCSRA)
  // set a2d prescaler so we are inside the desired 50-200 KHz range.
  #if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
    sbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);
  #elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
    cbi(ADCSRA, ADPS2);
    sbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the prescaler is 2
    cbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    sbi(ADCSRA, ADPS0);
  #endif
  // enable a2d conversions
  sbi(ADCSRA, ADEN);
#endif

  // the bootloader connects pins 0 and 1 to the USART; disconnect them
  // here so they can be used as normal digital i/o; they will be
  // reconnected in Serial.begin()
#if defined(UCSRB)
  UCSRB = 0;
#elif defined(UCSR0B)
  UCSR0B = 0;
#endif
}

uint32_t system_micros() {
  uint32_t m;
  uint8_t oldSREG = SREG, t;

  cli();
  m = timer0_overflow_count;
#if defined(TCNT0)
  t = TCNT0;
#elif defined(TCNT0L)
  t = TCNT0L;
#else
  #error TIMER 0 not defined
#endif

#ifdef TIFR0
  if ((TIFR0 & _BV(TOV0)) && (t < 255))
    m++;
#else
  if ((TIFR & _BV(TOV0)) && (t < 255))
    m++;
#endif

  SREG = oldSREG;

  return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}

void system_init(void) {
  system_timers_init();
  system_uart_init();
}
