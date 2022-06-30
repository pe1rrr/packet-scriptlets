#!/usr/bin/expect 
gets stdin callsign
	spawn -noecho nc 192.168.1.32 3600
	send "/n $callsign\n"
	interact
	exit
