#!/bin/bash 
# Make sure you have common.sh in the same directory as this script and configured.
source common.sh

regex='(https?|ftp|file)://[-A-Za-z0-9\+&@#/%?=~_|!:,.;]*[-A-Za-z0-9\+&@#/%=~_|]'
yesnoregex='^(Y|y)$'
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
