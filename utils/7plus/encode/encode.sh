#!/bin/bash
# 7plus encoder fudge by PE1RRR
# V0.4
# Edit vars in config.sh
# recommend setting config.sh to read-only to prevent overwrite on subsequent updates.
# assumptions are that config.sh and this script are kept in the same directory.
#
# Do not change anything below this line.
function abort {
	echo "Script aborted"
	exit
}

parentdir=$(dirname ${PWD})
. "${parentdir}/config.sh"

if [ ! -d "${backupdir}" ]; then
	mkdir -p ${backupdir}
fi

if  [ -z "${mycall}" ]; then
	echo "You haven't set up the mycall configuration properly, aborting"
	abort
fi
 
if test ! -f ${plusbin}; then
	echo "7plus binary not found... please fix your plusbin path setting, aborting"
	abort
fi


infilepath=$1
function fileprompt {
	if [ -z "${infilepath}" ]; then
		read -ep "Enter Filename: " -i "file.zip" infilepath
		infilename=$(basename $infilepath)
		if test -f "${infilepath}"; then
			echo "${infilename} found... continuing..."
		else
			echo "${infilepath} not found... "
			fileprompt
		fi
	else 
		infilename=$(basename $infilepath)
	fi
}


function msgtype {
	read -ep "Enter Message Type (B  for Bulletin or P for Private): " -i "P" msgtype
	if [ "${msgtype}" == "P" ] 
	then 
		echo "Selected Private Message."
	elif [ "${msgtype}" == "B" ]
	then
		echo "Selected Bulletin."
	else
		echo "${msgtype} is invalid, try again"
		msgtype
	fi
}


function msgroute {
	if [ "${msgtype}" == "B" ]
	then
 		read -ep "Enter Target Area: @" -i "WW" msgroute
		length=${#msgroute}
		if [ $length -gt 6 ] || [ $length -eq 0 ]
		then
			echo "For bulletins, target address must be between 1 and 6 characters, yours was ${length} characters - try again"
			msgroute
		fi

	elif [ "${msgtype}" == "P" ]
	then
		read -ep "Enter Full HROUTE: @" msgroute
		# could add some sanity checking here.
	else
		abort
	fi
}

function msgdest {
	if [ "${msgtype}" == "B" ]
	then
		read -ep "Enter Destination Topic: " msgdest
	elif [ "${msgtype}" == "P" ]
	then
		read -ep "Enter Destination Callsign: " msgdest
	fi
		
	length=${#msgdest}
	if [ $length -gt 6 ] || [ $length -eq 0 ]
	then
		echo "Destination must be between 1 and 6 characters, yours was ${length} characters - try again"
		msgdest
	fi
}

function msgsize {
	read -ep "Enter message split chunk Size in bytes : " -i "16000" msgchunk
	if [ -z "$msgchunk" ]
	then
		echo "This setting can't be empty. Try again"
		msgsize
	fi
	if [ "$msgchunk" -gt "31000" ] || [ "$msgchunk" -eq "0" ] 
	then
		echo "Chunks should not exceed 31000 bytes or be null, yours was ${msgchunk} - try again"
		msgsize
	fi
}


function dostuff {
	clear
	fileprompt
	msgtype
	msgdest
	msgroute
	msgsize

	# Format the mail header
 	destination="S${msgtype} ${msgdest} @ ${msgroute}"
 	checkit="Message type: ${msgtype}\nSize: ${msgchunk}\nDestination: ${msgdest}@${msgroute}\n\nFrom: $mycall\nFile: $infilename\n\n"

	clear
	echo -e "------------------------------------------------------------\n\n"
	echo -e "Please double check the paramters:"
	echo -e "$checkit"
	echo -e "------------------------------------------------------------\n\n"
	read -ep "Is the above correct? : " -i "Y" ready
	
	if [ "${ready}" != "Y" ]
	then
		dostuff
	fi
	# Generate the mail into one joined file for the BBS to import.
	${plusbin} -send2 "${destination} < ${mycall}" -t /ex -sb ${msgchunk} -j ${infilepath}

	# Ugly workaround for the fact 7plus always generates lowercase .upl files regardless of input.
	uplfile=`echo "${infilename}" | tr '[:upper:]' '[:lower:]' | cut -d'.' -f1`

	mv "${uplfile}.upl" "${tmp}/${uplfile}.upl"

	ls -l "${tmp}/${uplfile}.upl"

	echo "Does this look ok? CTRL-C to abort"
	sleep 5

	echo "Sleeping... to be sure... CTRL-C to abort or I will send this to the ${importfile} file".
	sleep 10


	cat "${tmp}/${uplfile}.upl" >> ${importfile}

	mv "${tmp}/${uplfile}.upl" "${backupdir}"
}

dostuff

