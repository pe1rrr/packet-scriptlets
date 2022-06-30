#!/usr/bin/expect 
	send "Standby- Hacking into OrbitalNODE ...\n"
	spawn -noecho telnet horizons.jpl.nasa.gov 6775
	interact
	exit
