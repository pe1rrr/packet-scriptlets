#!/bin/bash
script="/home/bpq/nodes/pe1rrr/scripts/scanselect.expect"
user="pe1rrr"
pass="hereBdragonsmaybe!"
host="gigabox"
port="2323"

$script $host $port $user $pass $1 "$2" $3 
