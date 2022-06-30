#!/usr/bin/expect 
#send "Please connect to RRRNOS and issue the RPG command there\n"
#send "73!\n"
#exit

	spawn -noecho telnet 192.168.1.42 62007
	interact
	exit
