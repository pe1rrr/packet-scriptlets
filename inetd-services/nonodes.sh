#!/bin/bash
script="/home/bpq/nodes/pe1rrr/scripts/nonodes.expect"
user="pe1rrr"
pass="hereBdragonsmaybe!"
host="gigabox"
port="2323"


OldIFS=$IFS
QuitCommandRegex='^(B|b|q|Q)$'

echo "one moment please..."
NodeListString=`$script $host $port $user $pass`
declare -a NodeArray
IFS=" "

declare -a NodeArray=( $NodeListString )

#Sorted=($(sort <<<"${NodeArray[*]}"))

IFS=$OldIFS

for i in "${NodeArray[@]}"
do
	CleanupString=`echo $i | sed -e 's/\n//g' | sed -e 's/\r//g' | sed -e 's/ //g'`
	CleanedArray+=("$CleanupString")
done
function Prompt {
	echo "nonodesearch 0.0 by pe1rrr"
	echo "search--->"
	read  UserQuery
	CleanString=${UserQuery//[$'\t\r\n']} && UserQuery=${UserQuery%%*( )}
	CleanString=${CleanString//_/}
	CleanString=${CleanString// /_}
	CleanString=${CleanString//[^a-zA-Z0-9_\-]/}
	echo "you entered: $CleanString"
	if [[ $CleanString =~ $QuitCommandRegex ]]
	then
		Quit
	elif [[ -z $CleanString ]]
	then
		echo "thats empty!"
		Prompt	
	else
		DoSearch
	fi
}

function Quit() {
	echo "Exiting... Bye!"
	exit
}

function DoSearch {
	printf '%s\n' "${CleanedArray[@]}" | grep -i -- "$CleanString"
	Prompt
}

# Main loop
Prompt
