#!/bin/bash 
# Make sure you have common.sh in the same directory as this script and configured.
source common.sh

# Version 1.0 PE1RRR TLE Downloader Portal for Packet
# Configuration
# 
# For simplicity, I use openbsd's inetd which is available for Debian/Raspbian with sudo apt install openbsd-inetd
# 
# Add the following line to /etc/inetd.conf
# 
# tle		stream	tcp	nowait	bpq		/full/path/to/your/packet-scriptlets/tle.sh 
# 
# The word 'bpq' above refers to the userid that this process will run under.
#
# Add the following line to /etc/services and make a note of the port number you choose. Make sure the one you pick does
# not exist and is within the range of ports available.
#
# tle		63005/tcp   # Celestrak TLE Downloader
# 
# Enable inetd: sudo systemctl enable inetd
#               sudo service inetd start
#
# In bpq32.cfg add the port specified above (63005) to the BPQ Telnet port list, CMDPORT= 
#
# Note the port's position offset in the list as that will be referenced in the APPLICATION line next.
# The first port in the CMDPORT= line is position 0 (zero), the next would be 1, then 2 and so on.
#
# Locate your APPLICATION line definitions in bpq32.cfg and another one next in the sequence. Make
# sure it has a unique <app number> between 1-32.
#
# Syntax: 
# APPLICATION <app number 1-32>,<node command>,<node instruction>
#
# APPLICATION 24,TLE,C 10 HOST 2 S
#
# <node instruction> is where the command 'web' is told to use BPQ port 10 (the telnet port, yours may be different!)
# HOST is a command to tell BPQ to look at the BPQ Telnet CMDPORT= list, and '2' is to pick offset position 2, that
# in turn relsolves to TCP port 63005. The 'S' tells the node to return the user back to the node when they exit
# the web portal instead of disconnecting them, it refers to the word 'Stay'.

# Further config is at the bottom of the file for customization of the menu options as well as welcome message.
##### End of Config - Do not change anything below here.

quitcmds='^(B|b|q|Q).*$'

set -e
trap 'catch $? $LINENO' EXIT
catch() {
            if [ "$1" != "0" ]; then
                        # error handling goes here
                                exit
      fi
 }


function checkurl() {
	if $curl --output /dev/null --silent --head --fail "$url"; then
	  	echo "URL exists: $url - Loading..."
 	else
	        echo "URL does not exist: $url"
		startquery
	fi	

}
function menu() {
	echo "Welcome to Red's TLE Server"
	echo "This system pulls the latest up-to-date TLEs directly from NORAD"
	echo ""
	echo "[1] Last 30 Days' Launches"
	echo "[2] Space Stations"
	echo "[3] 100 Brightest Objects"
	echo "[4] Active Satellites"
	echo ""
	echo "Supplemental Data" 
	echo ""
	echo "[200] ISS Orbits"
	echo "[201] Starlink Satellites (Warning: HUGE list!)"
	echo "[202] Planet Satellites"
	echo "[203] OneWeb Satellites"
	echo "[204] GPS Satellites"
	echo "[205] GLONASS Satellites"
	echo "[206] MeteoSat Satellites"
	echo "[207] IntelSat Satellites"
	echo "[208] SES Satellites"
	echo "[209] Telesat Satellites"
	echo "[210] CPF Satellites"
	echo ""
	echo "[0] Quit"
	echo "[100] Redisplay Menu"
	chooser
}

function chooser() {
	echo "Enter choice: (Quit: 0, Menu: 100) "
	read userchoice
	choice=${userchoice//[$'\t\r\n']} && userchoice=${userchoice%%*( )}
        if [[ $choice =~ $quitcmds ]]
	then
		exit
	fi
	choice
}

function choice() {
	if [ $choice -eq 0 ]
	then
		exit
	elif [ $choice -eq 1 ]
	then url="https://www.celestrak.com/NORAD/elements/tle-new.txt" ; fetchdata
	elif [ $choice -eq 2 ]
		then url="https://www.celestrak.com/NORAD/elements/stations.txt" ; fetchdata
	elif [ $choice -eq 3 ]
		then url="https://www.celestrak.com/NORAD/elements/visual.txt" ; fetchdata
	elif [ $choice -eq 4 ]
		then url="https://www.celestrak.com/NORAD/elements/active.txt" ; fetchdata
	elif [ $choice -eq 99 ]
		then exit
	elif [ $choice -eq 100 ]
		then menu
	elif [ $choice -eq 200 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/iss.txt" ; fetchdata
	elif [ $choice -eq 201 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/starlink.txt" ; fetchdata
	elif [ $choice -eq 202 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/planet.txt" ; fetchdata
	elif [ $choice -eq 203 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/oneweb.txt" ; fetchdata
	elif [ $choice -eq 204 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/gps.txt" ; fetchdata
	elif [ $choice -eq 205 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/glonass.txt" ; fetchdata
	elif [ $choice -eq 206 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/meteosat.txt" ; fetchdata
	elif [ $choice -eq 207 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/intelsat.txt" ; fetchdata
	elif [ $choice -eq 208 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/ses.txt" ; fetchdata
	elif [ $choice -eq 209 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/telesat.txt" ; fetchdata
	elif [ $choice -eq 210 ]
		then url="https://celestrak.com/NORAD/elements/supplemental/cpf.txt" ; fetchdata
	fi
	exit
}

function fetchdata() {
	echo "Requesting Data..."
	$lynx -hiddenlinks=ignore -nolist -nomore -dump  $url 
	echo ""
	echo "End of Data"
	chooser
}


menu
