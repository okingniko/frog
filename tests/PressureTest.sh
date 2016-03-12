#!/bin/bash
# 
## Program: Run Pressure test using "http_load".
## Usage: pressure_test.sh <url_file> <output_file>
## Author: okingniko
## Changelog:
##   1. 3/12/2016 Initialization.
# 

# Set Test rete here.(Access frequency per second)
test_rate=(5 10 20 40 80 160 320 640 1000)
# Set Test period here.(Seconds)
test_period=10 

if [ "$1" != '' ]; then
  url_file="$1"
else
  url_file="url_file.txt"
fi

if [ "$2" != '' ]; then
  output_file="$2"
else
  output_file="pressure_out.txt"
fi

echo "This is the pressure test result of frog server sample\n" > $output_file

# Begin test
for rate in ${test_rate[@]}; do
  echo  "Current Test rate: $rate(per second)"
  echo "##########################################################" >> $output_file
  echo "Test Rate: $rate(per second), seconds period: $test_period" >> $output_file
  echo "$(http_load -rate $rate -seconds $test_period $url_file)"   >> $output_file
  echo "##########################################################" >> $output_file
done

