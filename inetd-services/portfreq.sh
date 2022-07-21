#!/bin/bash
script="/home/pi/nodes/pe1rrr/scripts/portfreq.expect"
user="mycall"
pass="mypassword"
host="127.0.0.1"
port="8010"

$script $host $port $user $pass "1 2 3 4 5 6 7 8 21 22 23 24"
echo "Bye!"
