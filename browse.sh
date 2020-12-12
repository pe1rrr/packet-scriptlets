#!/bin/bash 

# Version 1.0 PE1RRR WEB Portal for Packet
# Configuration
# 
# For simplicity, I use openbsd's inetd which is available for Debian/Raspbian with sudo apt install openbsd-inetd
# 
# Add the following line to /etc/inetd.conf
# 
# browse		stream	tcp	nowait	bpq		/full/path/to/your/packet-scriptlets//browse.sh client ax25
# 
# The word 'bpq' above refers to the userid that this process will run under.
#
# Add the following line to /etc/services and make a note of the port number you choose. Make sure the one you pick does
# not exist and is within the range of ports available.
#
# browse		63004/tcp   # Browser
# 
# Enable inetd: sudo systemctl enable inetd
#               sudo service inetd start
#
# In bpq32.cfg add the port specified above (63004) to the BPQ Telnet port list, CMDPORTS= 
#
# Note the port's position offset in the list as that will be referenced in the APPLICATION line next.
# The first port in the CMDPORTS= line is position 0 (zero), the next would be 1, then 2 and so on.
#
# Locate your APPLICATION line definitions in bpq32.cfg and another one next in the sequence. Make
# sure it has a unique <app number> between 1-32.
#
# Syntax: 
# APPLICATION <app number 1-32>,<node command>,<node instruction>
#
# APPLICATION 25,WEB,C 10 HOST 1 S
#
# <node instruction> is where the command 'web' is told to use BPQ port 10 (the telnet port, yours may be different!)
# HOST is a command to tell BPQ to look at the BPQ Telnet CMDPORTS list, and '1' is to pick offset position 1, that
# in turn relsolves to TCP port 63004. The 'S' tells the node to return the user back to the node when they exit
# the web portal instead of disconnecting them, it refers to the word 'Stay'.

lynx="/usr/bin/lynx"  # sudo apt install lynx
curl="/usr/bin/curl"  # sudo apt install curl
logfile="/var/log/bpq-browser.log" # sudo touch /var/log/bpq-browser; sudo chmod bpq:bpq /var/log/bpq-browser

# Logfile: Again the word 'bpq' above refers to the user and group that the inetd server starts the process under. It must be
# the same to have permission to write to this logfile.

# It is recommended to set up a proxy server locally to handle the requests from this script
# it adds a level of control over which content can and cannot be requested, squid proxy is
# utilized for this, but if you do not want to use it, comment the 2 lines out below.
# I have set up my squid proxy to use alternate DNS servers of OpenDNS FamilyShield.

myproxy="http://127.0.0.1:3128"
export http_proxy=$myproxy


# Further config is at the bottom of the file for customization of the menu options as well as welcome message.
##### End of Config - Do not change anything below here.
linkregex='(https?|ftp|file)://[-A-Za-z0-9\+&@#/%?=~_|!:,.;]*[-A-Za-z0-9\+&@#/%=~_|]'
yesnoregex='^(Y|y)$'
quitcmds='^(B|b|q|Q)$'

set -e
trap 'catch $? $LINENO' EXIT
catch() {
	    if [ "$1" != "0" ]; then
		        # error handling goes here
				exit
      fi
 }

if [ "$1" == "ip" ]
then
	# Connection from 2nd param of inetd after 'client' on 'ip' port so standard telnet user is prompted for a callsign.
	echo "Please enter your callsign:"
	read callsignin
elif	[ "$1" == "ax25" ]
then
	# Connection came from a linbpq client on 'ax25' port which by default sends callsign on connect.
	read callsignin
fi



function checkurl() {
	if $curl --output /dev/null --silent --head --fail "$url"; then
	  	echo "URL exists: $url - Loading..."
 	else
	        echo "URL does not exist: $url"
		startquery
	fi	

}

function startquery() {
	echo "Put in an exact web address, and it will be fetched and displayed"
	printf "Enter a web address (b = exit) : http://"
	read address
	urlfix=${address//[$'\t\r\n']} && address=${address%%*( )}
	url="http://${urlfix}"

        if [[ $urlfix =~ $quitcmds ]]
	then
		exit
	fi

	if [[ $url =~ $linkregex ]]
	then 
	    echo "Link valid"
	else
	    echo "Link not valid"
	    startquery
	    exit
	fi
	checkurl
	echo "Displaying Page..."
	$lynx -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -dump  "${url}" 
	loguser
	readlink
}

function loguser() {
	date=`date`
	if ! [ -e $logfile ]
	then
		touch $logfile
	fi
	echo "$date: $callsign requested $url" >> $logfile
}

function getpage() {
        $lynx -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -trim_input_fields -trim_blank_lines -justify -dump  $url |sed '/^$/d'
	readlink
}


function readlink() {
	GetStuff=`$lynx -hiddenlinks=ignore -dump -unique_urls -listonly $url`
	for stuff in $GetStuff
	do
		stuffreg='^[0-9\.]+$'
		if  ! [[ $stuff =~ $stuffreg ]]
		then
			Links+=("$stuff")
		fi
	done
	count=0
	unset list
	for value in "${Links[@]}"
	do
		     list+="Link $count = $value \n"
		     count=$((count+1))
        done
	echo "Enter Link Number: (0 = quit L = list)"
	read linkid
	linkidfix=${linkid//[$'\t\r\n']} && linkid=${linkid%%*( )}
	linkidreg='(^([0-9])+$|^(l|L)$)'
	if ! [[ $linkidfix =~ $linkidreg ]]
	then
		echo "Invalid Link Number"
		exit
	fi

	if [[ $linkidfix == 0 ]]
	then
		echo "Exiting"
		exit
	fi
	listregex='^(l|L)$'
	if [[ $linkidfix =~ $listregex ]]
	then
		echo -e $list
		unset list
		unset Links
		readlink
	fi


	linkurl=${Links[${linkidfix}]}
	if [[ $linkurl =~ $linkregex ]]
	then 
	    echo "Link valid"
	else
	    echo "Link not valid"
	    startquery
	    exit
	fi
	Links=("")
	url=$linkurl
	loguser
	getpage
}

function webmenu() {

	echo "[1] Enter your own web address"
	echo "[100] COVID Information gov.uk (UK)"
	echo "[400] COVID Information Rijksoverheid (NL)"

	read menuinput
	menuchoice=${menuinput//[$'\t\r\n']} && menuinput=${menuinput%%*( )}
	idreg='^([0-9])+$'
	if ! [[ $menuchoice =~ $idreg ]]
	then 
	    echo "Choice not valid, try again"
	    startquery
	    exit
	fi

	if [ $menuchoice -eq 0 ]
	then
		exit
	elif [ $menuchoice -eq 1 ]
	        then startquery
		exit
	elif [ $menuchoice -eq 100 ]
	then
		url="https://www.gov.uk/guidance/local-restriction-tiers-what-you-need-to-know"
		getpage
	elif [ $menuchoice -eq 400 ]
	then
		url="https://www.rijksoverheid.nl/onderwerpen/coronavirus-covid-19"
		getpage
	fi
}

# Trim whitespaces and newlines from the callsign but not necessarily verify it is sane (its only for logging).
# I should probably do full sanitization here, although the callsign is passed from BPQ itself automatically
# if you decide to run this script and expose direct access to it via telnet one could put any garbage in..

callsign=${callsignin//[$'\t\r\n']} && callsignin=${callsignin%%*( )}

echo "Welcome to Simple Packet Web"
echo "All requests are logged, inappropriate sites are blocked by OpenDNS FamilyShield"

webmenu
