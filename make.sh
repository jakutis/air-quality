#!/usr/bin/env bash

if [ -z "$DEVICE" ]
then
  echo "Error: environment variable DEVICE is empty, but it should be equal to something like /dev/ttyACM0" 1>&2
  exit 1
fi

COMPILE_C_CMD="avr-gcc -DF_CPU=16000000UL -mmcu=atmega328p -c -std=c11"
COMPILE_C="$COMPILE_C_CMD -pedantic -Werror -Os -o"

cd lib
$COMPILE_C a.o b.c || exit 1
$COMPILE_C uart.o uart.c || exit 1
$COMPILE_C micros.o micros.c || exit 1
avr-gcc -mmcu=atmega328p micros.o uart.o a.o -o a || exit 1
avr-objcopy -O ihex -R .eeprom a a.hex || exit 1
cd ..
sudo avrdude -F -V -c arduino -p ATMEGA328P -P "$DEVICE" -b 115200 -U flash:w:lib/a.hex || exit 1
