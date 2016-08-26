#!/usr/bin/env bash

URL="$1"
cat "$DEVICE" | while IFS='' read -r line
do
  echo -- $line
  [ -n "$line" ] && (echo "$line" | jq "{date:\"$(date)\",temperature,humidity}" | curl -H "Content-Type: application/json" --data @/dev/stdin "$URL" &)
done
