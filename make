#!/usr/bin/env bash

COMPILE="avr-gcc -pedantic -Werror -std=c11 -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o"

$COMPILE a.o a.c || exit 1
$COMPILE uart.o uart.c || exit 1
avr-gcc -mmcu=atmega328p uart.o a.o -o a || exit 1
avr-objcopy -O ihex -R .eeprom a a.hex || exit 1
avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:a.hex || exit 1
