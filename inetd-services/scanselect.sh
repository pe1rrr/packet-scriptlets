#!/bin/bash
script="/home/pi/nodes/pe1rrr/scripts/scanselect.expect"
user="mycall"
pass="mypassword"
host="127.0.0.1"
port="8010"

$script $host $port $user $pass $1 "$2" $3 
