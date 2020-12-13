#!/bin/bash 
# Version 1.0 PE1RRR WEB Portal for Packet
#
# Configuration
# 
LynxBin="/usr/bin/lynx"  # sudo apt install lynx
CurlBin="/usr/bin/curl"  # sudo apt install curl
WebLogFile="/var/log/bpq-browser.log" # sudo touch /var/log/bpq-browser; sudo chmod bpq:bpq /var/log/bpq-browser
Debug="1"

# It is recommended to set up a proxy server locally to handle the requests from this script
# it adds a level of control over which content can and cannot be requested, squid proxy is
# utilized for this, but if you do not want to use it, comment the 2 lines out below.
# I have set up my squid proxy to use alternate DNS servers of OpenDNS FamilyShield.

myproxy="http://127.0.0.1:3128"
export http_proxy=$myproxy
# 
# Usage & Installation
#
# For simplicity, I use openbsd's inetd which is available for Debian/Raspbian with sudo apt install openbsd-inetd
# 
# Add the following line to /etc/inetd.conf
# 
# browse		stream	tcp	nowait	bpq		/full/path/to/your/packet-scriptlets/browse.sh client ax25
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
# In bpq32.cfg add the port specified above (63004) to the BPQ Telnet port list, CMDPORT= 
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
# APPLICATION 25,WEB,C 10 HOST 1 S
#
# <node instruction> is where the command 'web' is told to use BPQ port 10 (the telnet port, yours may be different!)
# HOST is a command to tell BPQ to look at the BPQ Telnet CMDPORT= list, and '1' is to pick offset position 1, that
# in turn relsolves to TCP port 63004. The 'S' tells the node to return the user back to the node when they exit
# the web portal instead of disconnecting them, it refers to the word 'Stay'.

# Further config is at the bottom of the file for customization of the menu options as well as welcome message.
##### End of Config - Do not change anything below here.
#
# Global Vars
LinkRegex='(https?|ftp|file)://[-A-Za-z0-9\+&@#/%?=~_|!:,.;]*[-A-Za-z0-9\+&@#/%=~_|]'
QuitRegex='^(0|B|b|q|Q)$'

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



function CheckURLSanity() {
	local CheckURL=$1
	local ContentType
	local ContentTypeRegex

	if ! [[ $CheckURL =~ $LinkRegex ]]
	then 
	    ReturnVal="Error: Address not valid"
	    return 1
	fi

	if $CurlBin --output /dev/null --silent --head --fail "${CheckURL}"; then
		ContentType=$($CurlBin -s -I -XGET "${CheckURL}" --output /dev/null -w '%{content_type}\n')
		ContentTypeRegex='^(text\/html|text\/plain).*$'
		if  ! [[ $ContentType =~ $ContentTypeRegex ]] 
		then
			ReturnVal="Error: Sorry, that content is not text!"
			return 1
		fi
		return 0
 	else
		ReturnVal="Error: Sorry, ${CheckURL} does not appear to exist."
		return 1
	fi	

}

function DownloadPage() {
        local Text
	local URL
	URL=$1

	# Maybe do sanity check here instead?

	Text=`$LynxBin -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -trim_input_fields -trim_blank_lines -justify -dump  ${URL} | sed '/^$/d'`

	# Global
	ReturnVal="${Text}"
}


function Begin() {
	local Address
	local URL
	local URLFix
	local Text

	printf "Enter a web address (0 = Quit) : http://"
	read Address

	# Trim Input
	URLFix=${Address//[$'\t\r\n']} && Address=${Address%%*( )}

	# Force http regardless.
	URL="http://${URLFix}"

	# Offer an escape
        if [[ $URLFix =~ $QuitRegex ]]
	then
		echo "Exiting...Bye!"
		exit
	fi

	# Sanity Check the URL
	if ! CheckURLSanity "${URL}"
	then 
		echo $ReturnVal
		unset ReturnVal
		Begin # Again
	fi

	# Generate Initial Page
	DownloadPage "${URL}"
	Text=$ReturnVal

	# Display The Page
	DisplayPage "${URL}" "${Text}"

	# Clear Global
	unset ReturnVal

	# Write Request to Log
	LogUser "${Address}"

	Prompt "${URL}"
}

function DisplayPage() {
	local URL
	local Text

	URL=$1
	Text=$2

	echo -e "Displaying ${URL}"
	echo -e "${Text}"
	return 0
}

function LogUser() {
	local URL
	local Date

	URL=$1
	Date=`date`

	if ! [ -e ${WebLogFile} ]
	then
		touch ${WebLogFile}
	fi
	echo "${Date}: ${Callsign} requested ${URL}" >> ${WebLogFile}
}



function Prompt() {
	local URL
	local GetLinksOnly
	local RestartURL

	URL=$1
	RestartURL=$1
	# Fetch the same URL used in display but only return with the links listed
	GetLinksOnly=`${LynxBin} -hiddenlinks=ignore -dump -unique_urls -listonly ${URL}`

	# Compile list of results into an array.
	local Links
	local IndexRegex
	for Results in ${GetLinksOnly}
	do
		IndexRegex='^[0-9\.]+$'
		if  ! [[ $Results =~ $IndexRegex ]]
		then
			Links+=("$Results")
		fi
	done

	# Build the human readable list of links
	local Count
	local LinkList
	local Value
	Count=0
	for Value in "${Links[@]}"
	do
		     LinkList+="Link $Count = $Value \n"
		     Count=$((Count+1))
        done


	# Prompt
	local LinkID
	echo "Enter Link Number: (0 = quit L = list)"
	read LinkID

	# Trim Input
	local LinkIDFix
	LinkIDFix=${LinkID//[$'\t\r\n']} && LinkID=${LinkID%%*( )}

	# Handle Link Choice
	local LinkIDRegex
	local ListCommandRegex
	LinkIDRegex='(^([0-9])+$|^(l|L|b|B)$)'
	ListCommandRegex='^(l|L)$'

	if ! [[ $LinkIDFix =~ $LinkIDRegex ]]
	then
		echo "Error: Oops! Invalid Link Number."
		Prompt "${URL}" # Again
		exit
	elif [[ $LinkIDFix == 0 ]]
	then
		echo "Exiting...Bye!" # Escape
		exit
	elif [[ $LinkIDFix =~ $ListCommandRegex ]]
	then
		echo -e ${LinkList} # Display Links
		unset Links # Kill the String to prevent looping additions.
		Prompt "${URL}"# Again
	fi


	# So an actual link ID has been requested...
	# LinkRegex is Global
	local LinkURL 

	LinkURL=${Links[${LinkIDFix}]}
	if ! [[ $LinkURL =~ $LinkRegex ]]
	then 
	    echo "Error: Sorry, ${LinkURL} cannot be accessed via this portal."
	    unset Links
	    Prompt "${RestartURL}" # Again
	    exit
	fi
		
	unset Links # Just to be sure.

	if ! CheckURLSanity "${LinkURL}" 
	then 
		echo $ReturnVal
		unset ReturnVal
		Prompt "${RestartURL}" # Again - might not work!
		exit
	fi
	local Text
	DownloadPage "${LinkURL}"

	Text="${ReturnVal}"

	DisplayPage "${LinkURL}" "${Text}"

	# Clear up Global
	unset ReturnVal

	LogUser "${LinkURL}"

	# Loop Back
	Prompt "${LinkURL}"

}

function GetPage() {
	local URL
	URL=$1

	# Generate Initial Page
	DownloadPage "${URL}"
	Text=$ReturnVal

	# Display The Page
	DisplayPage "${URL}" "${Text}"

	# Clear Global
	unset ReturnVal

	Prompt "${URL}"
}

function MainMenu() {
	echo "Please note this portal is a work in progress and as such isn't yet fully featured and is still a little buggy!"
	echo "[1] Enter your own web address"
	echo "[100] COVID Information gov.uk (UK)"
	echo "[400] COVID Information Rijksoverheid (NL)"

	echo "Enter Choice: (Quit: 0)"

	local IDRegex
	local Choice
	local URL

	read Choice

	# Trim
	Selection=${Choice//[$'\t\r\n']} && Choice=${Choice%%*( )}
	IDRegex='^([0-9])+$'

	if ! [[ $Selection =~ $IDRegex ]]
	then 
	    echo "Error: Sorry, that selection was not valid, please check and try again."
	    Begin
	    exit
	fi

	if [ $Selection -eq 0 ]
	then
		exit
	elif [ $Selection -eq 1 ]
	        then Begin
		exit
	elif [ $Selection -eq 100 ]
	then
		URL="https://www.gov.uk/guidance/local-restriction-tiers-what-you-need-to-know"
		GetPage "${URL}"
	elif [ $Selection -eq 400 ]
	then
		URL="https://www.rijksoverheid.nl/onderwerpen/coronavirus-covid-19"
		GetPage "${URL}"
	else
		echo "Error: Sorry, you must make a selection from the menu!"
		Begin
	fi


}


# Trim whitespaces and newlines from the callsign but not necessarily verify it is sane (its only for logging).
# I should probably do full sanitization here, although the callsign is passed from BPQ itself automatically
# if you decide to run this script and expose direct access to it via telnet one could put any garbage in..

callsign=${callsignin//[$'\t\r\n']} && callsignin=${callsignin%%*( )}

echo "Welcome to Simple Packet Web"
echo "All requests are logged, inappropriate sites are blocked by OpenDNS FamilyShield"

MainMenu
