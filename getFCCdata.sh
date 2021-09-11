#!/bin/bash

set -x

# FCC_DATA_URL="https://transition.fcc.gov/fcc-bin/tvq?serv=DTV&list=4"
# curl --cipher 'DEFAULT:!DH' 'https://transition.fcc.gov/fcc-bin/tvq?serv=DTV&country=us&list=4' \
#  | cut --delimiter '|' --only-delimited -f2,11-12,39 \
#  | sort --unique > fccdata.txt

intermediateFile="/tmp/facility.zip"
fccUrl="https://transition.fcc.gov/Bureaus/MB/Databases/cdbs/facility.zip"

curl --verbose --cipher 'DEFAULT:!DH' -o "${intermediateFile}" "${fccUrl}" \
  && unzip "${intermediateFile}" \
  && rm "${intermediateFile}"
