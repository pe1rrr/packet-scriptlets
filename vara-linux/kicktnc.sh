#!/bin/bash
script="/home/pi/linbpq/scripts/kicktnc.expect"
user="MyBPQLogin"
pass="MyBPQPass!"
host="localhost"
port="8010"
remotehost="$1"
remoteport1="$2"
remoteport2="$3"
tncport="$4"
sshuser="$5"
lockfile=/tmp/varalock

function KillPorts {
	if ! sudo killcx "${remotehost}:${remoteport1}"; then
		CmdError=1
	else
		CmdError=0
	fi
	if ! sudo killcx "${remotehost}:${remoteport2}"; then
		DataError=1
	else
		DataError=0
	fi
}


function NCPorts {
	if ! nc -w 2 $remotehost $remoteport1; then
		NCDataError=1
	else
		NCDataError=0
	fi
	if ! nc -w 2 $remotehost $remoteport2; then
		NCCmdError=1
	else
		NCCmdError=0
	fi
}

if ! ssh $sshuser@$remotehost "test -e ${lockfile}"; then
    # your file exists
	if ! $script $host $port $user $pass $tncport ; then
		CmdError=1
		echo "Nothing to do"
	else
		echo "Problem..."
		KillPorts
		if [[ $DataError == 1 ]] && [[ $CmdError == 1 ]] ; then
			echo "No TCP connections Alive. Killed TNC. Locking..."
			ssh ${sshuser}@${remotehost} touch $lockfile
			ssh ${sshuser}@${remotehost} sudo service vara restart
			sleep 30
			echo "Removing Lockfile..."
			ssh ${sshuser}@${remotehost} rm $lockfile
		else
			echo "Attempting to Poke Ports"
			NCPorts
			if [[ $NCDatsError == 1 ]] && [[ $NCCmdError == 1 ]] ; then
				echo "Connections refused... TNC is unalive"
				ssh ${sshuser}@${remotehost} touch $lockfile
				ssh ${sshuser}@${remotehost} sudo service vara restart
				sleep 30
				echo "Removing Lockfile..."
				ssh ${sshuser}@${remotehost} rm $lockfile
			fi
		fi 
	fi
else
	echo "Busy"
fi
