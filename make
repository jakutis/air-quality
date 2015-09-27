#!/usr/bin/env bash

avr-gcc -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o a.o a.c || exit 1
avr-gcc -mmcu=atmega328p a.o -o a || exit 1
avr-objcopy -O ihex -R .eeprom a a.hex || exit 1
avrdude -F -V -c arduino -p ATMEGA328P -P /dev/ttyACM0 -b 115200 -U flash:w:a.hex || exit 1
