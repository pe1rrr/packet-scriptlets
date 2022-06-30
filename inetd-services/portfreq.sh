#!/bin/bash
script="/home/bpq/nodes/pe1rrr/scripts/portfreq.expect"
user="pe1rrr"
pass="hereBdragonsmaybe!"
host="gigabox"
port="2323"

$script $host $port $user $pass "1 2 3 4 5 6 7 8 21 22 23 24"
echo "Bye!"
