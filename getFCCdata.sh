#!/bin/bash

FCC_DATA_URL="https://transition.fcc.gov/fcc-bin/tvq?chan=0.0&cha2=69&type=3&list=4"

set -x

curl "${FCC_DATA_URL}" > fccdata.txt
# curl "${FCC_DATA_URL}" | cut --delimiter '|' -f2,10-13 > fccdata.txt
