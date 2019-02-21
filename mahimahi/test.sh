#!/bin/bash
# this will be the script for getting stuff run
cd ~
mkdir takehome_test
cd takehome_test
echo "Jiacheng Wang _ Take_home_test"

use warnings;
use strict;
use POSIX;

sub usage
{
  die qq{Usage: $0 [filename]\n};
}

if ( scalar @ARGV > 1 ) {
  usage;
}

# Question A
./mm-delay 10 
./mm-link /usr/share/mahimahi/traces/ATT-LTE-driving-2016.up /usr/share/mahimahi/traces/ATT-LTE-driving-2016.down --uplink-queue=droptail --downlink-queue=droptail --uplink-queue-args="packets=200" --downlink-queue-args="packets=200"

# Question B

timestamp ="date +%S"
./run-iperf3 -s
./run-iperf3 -c 100.64.0.3  -R
# Specially use -R to let server to send message to client

# Question C
./mm-delay-graph
./mm-link --meter-downlink /usr/share/mahimahi/traces/ATT-LTE-driving-2016.up /usr/share/mahimahi/traces/ATT-LTE-driving-2016.down
./run-iperf -s -t 60
./run-iperf -c 100.64.0.3 -t 60
# Shut down the iperf service after 60 seconds.

# Question D
./mm-webrecord /tmp/youtube chromium-browser --ignore-certificate-errors --user-data-dir=/tmp/nonexistent$(date +%s%N) https://www.youtube.com/watch?v=LXb3EKWsInQ
./mm-delay-graph
./mm-link --meter-downlink /usr/share/mahimahi/traces/ATT-LTE-driving-2016.up /usr/share/mahimahi/traces/ATT-LTE-driving-2016.down

# Question E

# For Question E. I can use the chromium to save the HAZ files
# And Use the extension application of chromium, HTTP Archive Viewer to separate different part
# But I could not figure out a way to write it as a script (Sorry, I am a new script writer)