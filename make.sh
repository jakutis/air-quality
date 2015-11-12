#!/usr/bin/env bash

if [ -z "$DEVICE" ]
then
  echo "Error: environment variable DEVICE is empty, but it should be equal to something like /dev/ttyACM0" 1>&2
  exit 1
fi

COMPILE="avr-gcc -pedantic -Werror -std=c11 -Os -DF_CPU=16000000UL -mmcu=atmega328p -c -o"

sudo apt-get install gcc-avr avr-libc avrdude || exit 1
$COMPILE a.o a.c || exit 1
$COMPILE uart.o uart.c || exit 1
avr-gcc -mmcu=atmega328p uart.o a.o -o a || exit 1
avr-objcopy -O ihex -R .eeprom a a.hex || exit 1
sudo avrdude -F -V -c arduino -p ATMEGA328P -P "$DEVICE" -b 115200 -U flash:w:a.hex || exit 1
