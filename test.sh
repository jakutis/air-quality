#!/usr/bin/env bash

cd test && npm install && ./node_modules/.bin/mocha --async-only --full-trace --timeout 300000 --ui bdd --bail --no-colors --reporter spec . || exit 1
