#!/usr/bin/env bash

if [ -z "$DEVICE" ]
then
  echo "Error: environment variable DEVICE is empty, but it should be equal to something like /dev/ttyACM0" 1>&2
  exit 1
fi

COMPILE_C_CMD="avr-gcc -DBAUD=9600 -DF_CPU=16000000UL -mmcu=atmega328p -c -std=c11"
COMPILE_C="$COMPILE_C_CMD -pedantic -Werror -Os -o"

cd lib
$COMPILE_C app.o app.c || exit 1
$COMPILE_C system.o system.c || exit 1
avr-gcc -mmcu=atmega328p system.o app.o -o ../app || exit 1
cd ..
avr-objcopy -O ihex -R .eeprom app app.hex || exit 1
sudo avrdude -F -V -c arduino -p ATMEGA328P -P "$DEVICE" -b 115200 -U flash:w:app.hex || exit 1
