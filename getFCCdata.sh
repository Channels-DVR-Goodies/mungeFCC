#!/bin/bash

FCC_DATA_URL="https://transition.fcc.gov/fcc-bin/tvq?serv=DTV&list=4"

set -x

curl --cipher 'DEFAULT:!DH' 'https://transition.fcc.gov/fcc-bin/tvq?serv=DTV&country=us&list=4' \
  | cut --delimiter '|' --only-delimited -f2,11-12,39 \
  | sort --unique > fccdata.txt
