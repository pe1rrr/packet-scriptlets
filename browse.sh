#!/bin/bash 

linkregex='(https?|ftp|file)://[-A-Za-z0-9\+&@#/%?=~_|!:,.;]*[-A-Za-z0-9\+&@#/%=~_|]'
yesnoregex='^(Y|y)$'
quitcmds='^(B|b|q|Q)$'
lynx="/usr/bin/lynx"
myproxy="http://127.0.0.1:3128"
curl="/usr/bin/curl"
logfile="/var/log/bpq-browser.log"


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
	# Connection came from a linbpq client on 'ax25' port which sends callsign on connect.
	read callsignin
fi

export http_proxy=$myproxy
#export https_proxy=$myproxy


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
		#stuffreg='((^[0-9\.]+$)|(References))'
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

# Sanitize the callsign but not necessarily verify it (its only for logging).
callsign=${callsignin//[$'\t\r\n']} && callsignin=${callsignin%%*( )}

echo "Welcome to Simple Packet Web"
echo "All requests are logged, inappropriate sites are blocked by OpenDNS FamilyShield"

webmenu
