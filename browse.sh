#!/bin/bash 
# Version 0.2 PE1RRR WEB Portal for Packet
#
# Configuration
# 
LynxBin="/usr/bin/lynx"  # sudo apt install lynx
CurlBin="/usr/bin/curl"  # sudo apt install curl
WebLogFile="/var/log/bpq-browser.log" # sudo touch /var/log/bpq-browser; sudo chmod bpq:bpq /var/log/bpq-browser
Version="0.2"

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
# in turn resolves to TCP port 63004. The 'S' tells the node to return the user back to the node when they exit
# the web portal instead of disconnecting them, it refers to the word 'Stay'.

# Further config is at the bottom of the file for customization of the menu options as well as welcome message.
##### End of Config - Do not change anything below here.
#
# Global Vars
LinkRegex='[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*)'
QuitCommandRegex='^(0|q|Q)$'
MenuCommandRegex='^(m|M)$' 
ListCommandRegex='^(l|L)$'
BackCommandRegex='^(B|b)$'
NewCommandRegex='^(n|N)$'
BackPage="none"
Referrer="none"

set -e
trap 'catch $? $LINENO' EXIT
catch() {
	    if [ "$1" != "0" ]; then
		        # error handling goes here
				exit
      fi
 }




function CheckURLSanity() {
	local CheckURL=$1
	local ContentType
	local ContentTypeRegex

	# Ignore case with ,, below
	if ! [[ ${CheckURL,,} =~ $LinkRegex ]]
	then 
	    ReturnVal="Error: Address provided is not a valid URL"
	    return 1
	fi

	# Ignore case with ,, below
	if [[ ${CheckURL,,} =~ ^(gopher:|mailto:|ftp:|file:).*$ ]]
	then
		ReturnVal="Error: That type of URL is not allowed. Supported: http or https."
		return 1
	fi

	if $CurlBin --output /dev/null --silent --head --fail "${CheckURL}"; then
		ContentType=$($CurlBin -s -L -I -XGET "${CheckURL}" --output /dev/null -w '%{content_type}\n')
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
	local ReferrerFunc
	local ReferrerURL
	URL=$1
	ReferrerFunc=$2
	ReferrerURL=$3

	# Sanity Check the URL
	if ! CheckURLSanity "${URL}"
	then 
		echo $ReturnVal
		unset ReturnVal
		$ReferrerFunc "${ReferrerURL}"
	fi

	Text=`$LynxBin -useragent=SimplePktPortal_L_y_n_x -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -trim_input_fields -trim_blank_lines -justify -dump  ${URL} | sed '/^$/d'`

	# Global
	ReturnVal="${Text}"
}

function Quit() {
	echo "Exiting... Bye!"
	exit
}

function Begin() {
	local Address
	local URL
	local URLFix
	local Text

	Referrer="Begin"
	echo "Enter a web address (Q = quit, M = main menu):"
	read Address

	# Trim Input
	URLFix=${Address//[$'\t\r\n']} && Address=${Address%%*( )}

	if [[ $URLFix =~ $QuitCommandRegex ]]
	then
		Quit
		exit
	elif [[ $URLFix =~ $MenuCommandRegex ]]
	then
		MainMenu
	elif [[ $URLFix =~ ^https?:\/\/ ]]
	then
		URL="${URLFix}"
	else
		URL="http://${URLFix}"
	fi

	echo "Requesting ${URL}..."

	# Update Last Page Global
	BackPage="${URL}"

	GetPage "${URL}" "Begin" "${URL}"

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
	echo -e "The previous page was: ${BackPage}"
	return 0
}

function GetPage() {
	local URL
	local ReferrerFunc
	local ReferrerURL
	URL=$1
	ReferrerFunc=$2
	ReferrerURL=$3

	# Generate Initial Page
	DownloadPage "${URL}" "${ReferrerFunc}" "${ReferrerURL}"
	Text=$ReturnVal

	# Display The Page
	DisplayPage "${URL}" "${Text}"

	# Clear Global
	unset ReturnVal

	Prompt "${URL}"
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
	GetLinksOnly=`${LynxBin} -useragent=SimplePktPortal_L_y_n_x -hiddenlinks=ignore -dump -unique_urls -listonly ${URL}`

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
	echo "Enter Link Number: (Q = quit, L = list links, B = back, N = open new link, M = main menu)"
	read LinkID

	# Trim Input
	local LinkIDFix
	LinkIDFix=${LinkID//[$'\t\r\n']} && LinkID=${LinkID%%*( )}

	# Handle Link Choice
	local LinkIDRegex
	LinkIDRegex='(^([0-9])+$|^(l|L|q|b|B|Q|n|N|m|M)$)' # First Pass Regex

	if ! [[ $LinkIDFix =~ $LinkIDRegex ]]  # First Pass
	then
		echo "Error: Oops! Invalid Link Number."
		Prompt "${URL}" # Again
		exit
	elif [[ $LinkIDFix =~ $QuitCommandRegex ]] # Second Pass
	then
		Quit
		exit
	elif [[ $LinkIDFix =~ $ListCommandRegex ]] # Second Pass
	then
		echo -e ${LinkList} # Display Links
		unset Links # Kill the String to prevent looping additions.
		Prompt "${URL}"# Again
	elif [[ $LinkIDFix =~ $NewCommandRegex ]] # Second Pass
	then
		Begin
	elif [[ $LinkIDFix =~ $MenuCommandRegex ]] # Second Pass
	then
		MainMenu
	elif [[ $LinkIDFix =~ $BackCommandRegex ]]
	then
		if [[ ${BackPage} == "none" ]]
		then
			Prompt "${URL}" # Again
			exit
		else
			GetPage "${BackPage}" "Prompt" "${URL}"
			Prompt "${BackPage}"
		fi
			
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

	# Update Back-Page Global
	BackPage="${LinkURL}"

	GetPage "${LinkURL}" "Prompt" "${URL}"

	LogUser "${LinkURL}"

	# Loop Back
	Prompt "${LinkURL}"

}


function MainMenu() {
	echo "This portal is a work in progress. Please report bugs to pe1rrr@pe1rrr.#nbw.nld.euro"
	echo ""
	echo "[1] Enter your own web address"
	echo ""
	echo "Bookmarks"
	echo "[100] COVID Information gov.uk (UK)"
	echo "[200] COVID Information CDC (US & Canada)"
	echo "[400] COVID Information Rijksoverheid (Netherlands)"
	echo ""
	echo "[101] RSGB.org"
	echo "[201] ARRL.org [202] AMSAT.org"
	echo "[401] Veron.nl [402] VRZA.nl [403] DARES"

	echo "Enter Choice: (Q = quit)"

	local IDRegex
	local Choice
	local URL

	read Choice

	# Trim
	Selection=${Choice//[$'\t\r\n']} && Choice=${Choice%%*( )}
	IDRegex='^([0-9]|q|Q)+$'

	if ! [[ $Selection =~ $IDRegex ]]
	then 
	    echo "Error: Sorry, that selection was not valid, please check and try again."
	    MainMenu
	    exit
	fi

	if [[ $Selection =~ $QuitCommandRegex ]]
	then
		Quit
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
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 101 ]
	then
		URL="https://rsgb.org"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 200 ]
	then
		URL="https://www.cdc.gov/coronavirus/2019-ncov/index.html"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 201 ]
	then
		URL="http://www.arrl.org"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 202 ]
	then
		URL="https://www.amsat.org"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 400 ]
	then
		URL="https://www.rijksoverheid.nl/onderwerpen/coronavirus-covid-19"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 401 ]
	then
		URL="https://veron.nl"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 402 ]
	then
		URL="https://www.VRZA.nl/wp"
		GetPage "${URL}" "MainMenu"
	elif [ $Selection -eq 403 ]
	then
		URL="https://dares.nl"
		GetPage "${URL}" "MainMenu"
	else
		echo "Error: Sorry, you must make a selection from the menu!"
		MainMenu
	fi


}

function WelcomeMsg() {
	local Callsign
	Callsign=$1
	echo "Hello ${Callsign}, welcome to Simple Packet Web - by PE1RRR Version ${Version}"
	echo "All requests are logged, inappropriate sites are blocked by OpenDNS FamilyShield"
	return 0
}

function CheckCallsign() {
	local Call
	local CallsignRegex

	Call=$1
	CallsignRegex="[a-zA-Z0-9]{1,3}[0123456789][a-zA-Z0-9]{0,3}[a-zA-Z]"

	if [[ $Call =~ $CallsignRegex ]] 
	then
		return 0
	else
		return 1
	fi
}


# Inetd Connectivity- BPQ Node Connect and Telnet IP connect are handled differently.
Client=$1
if [ -z $Client ] 
then
	echo "Misconfigured, please ensure this script is called with the 'client <ip/ax25>' argument from inetd"
	exit
	fi

if [ ${Client} == "ip" ]
then
	# Connection from 2nd param of inetd after 'client' on 'ip' port so standard telnet user is prompted for a callsign.
	echo "Please enter your callsign:"
	read CallsignIn
elif	[ ${Client} == "ax25" ]
then
	# Connection came from a linbpq client on 'ax25' port which by default sends callsign on connect.
	read CallsignIn
fi

# Trim BPQ-Term added CRs and Newlines.
Callsign=${CallsignIn//[$'\t\r\n']} && CallsignIn=${CallsignIn%%*( )}
# Get rid of SSIDs
CallsignNOSSID=`echo ${Callsign} | cut -d'-' -f1`


# Check Validity of callsign
if ! CheckCallsign "${CallsignNOSSID}"
then
	echo "Error: Invalid Callsign..."
	Quit
	exit
fi

WelcomeMsg "${CallsignNOSSID}"
MainMenu
