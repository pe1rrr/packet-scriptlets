#!/bin/bash
# ASCII Graphs! By PE1RRR
# - sudo apt install gnuplot
#
# This works only in conjunction with a properly configured BPQ IPGATEWAY
# with ENABLESNMP added, and another script that collects the data from BPQ
# storing it into RRD database files. Those scripts are under construction for
# general release eventually. Meanwhile, oogle all you want. 
#
#
RRDDir="/home/bpq/nodes/pe1rrr/rrd"
TMPDir="${RRDDir}/tmp"
#
#
#
declare -A Ports

Ports[1]="IC756 AX300 Port 1"
Ports[2]="IC756 AX300 Port 2"
Ports[3]="IC756 VARA"
Ports[4]="IC756 ARDOP"
Ports[5]="G90 AX300 Port 1"
Ports[6]="G90 AX300 Port 2"
Ports[7]="G90 VARA"
Ports[8]="G90 ARDOP"
Ports[21]="IC703 AX300 Port 1"
Ports[22]="IC703 AX300 Port 2"
Ports[23]="IC703 VARA"
Ports[24]="IC703 ARDOP"
Ports[10]="Telnet"
Ports[11]="AX/UDP"
Ports[12]="AX/IP"
Ports[15]="APRS 144.800MHz"
Ports[16]="KISS_TCP"

# Globals
QuitRegex='^(Q|q|B|b)$'
MenuRegex='^(M|m)$'
Width="79"
Height="30"

function PortMenu() {
	echo "...it thickens!"
	echo ""
	echo "Welcome, please choose a port:"
	echo "AX300 Packet: 1,2,5,6,21,22"
	echo "VARA: 3,7,23"
	echo "ARDOP: 4,8,24"
	echo "AXIP: 11,12"
	echo "Telnet: 10"
	echo "APRS: 15"
	Prompt
}

function Prompt() {
	local Choice
	local ChoiceTrimmed

	echo "Port? (Q=quit, M=menu) >"

	read ChoiceTrim

	Choice=${ChoiceTrim//[$'\t\r\n']} && ChoiceTrim=${ChoiceTrim%%*( )}

	if [[ $Choice =~ $QuitRegex ]];
	then
		Quit
	elif [[ $Choice =~ $MenuRegex ]];
	then
		PortMenu
	fi


	if [[ ${Choice} =~ ^(1|2|3|4|5|6|7|8|10|11|12|15|16)$ ]];
	then
		AsciiPort "${Choice}"
	else
		Error
	fi
}

function Quit() {
	echo "Okay, thanks, bye!"
	exit
}
function Error() {
	echo "Sorry, you can't do that."
	Prompt
}

function AsciiPort() {
	local Port
	local CutCrap
	local Tag
	Port=$1
	CutCrap=$(($Height-2))
	Tag=`echo $(openssl rand -hex 9)`
	#
	PortName=${Ports[$Port]}
	# Export Data
	rrdtool fetch --end now --start end-24h ${RRDDir}/port${Port}.rrd AVERAGE | tail -n +3 | awk -F ' ' '{print $1,$2}'  | grep -v 'nan' | sed 's/^/"/g;s/:/"/g' > ${TMPDir}/${Tag}_Out.csv 
	rrdtool fetch --end now --start end-24h ${RRDDir}/port${Port}.rrd AVERAGE | tail -n +3 | awk -F ' ' '{print $1,$3}'  | grep -v 'nan' | sed 's/^/"/g;s/:/"/g' > ${TMPDir}/${Tag}_In.csv 

	# Smush data
	paste <(for i in {1..$(wc -l "${TMPDir}/${Tag}_Out.csv" | cut -d' ' -f1)}; do echo $i; done) <(cut -d' ' -f 2 "${TMPDir}/${Tag}_Out.csv") > "${TMPDir}/${Tag}2_Out.csv"

	paste <(for i in {1..$(wc -l "${TMPDir}/${Tag}_In.csv" | cut -d' ' -f1)}; do echo $i; done) <(cut -d' ' -f 2 "${TMPDir}/${Tag}_In.csv") > "${TMPDir}/${Tag}2_In.csv"

	paste <(for j in $(cut -f1 ${TMPDir}/${Tag}_In.csv -d' ' | sed -e 's/"//g'); do date --date="@$j" +%H:%M; done) <(cut -d' ' -f2 "${TMPDir}/${Tag}_In.csv")  > "${TMPDir}/${Tag}_In.csv"
	paste <(for k in $(cut -f1 ${TMPDir}/${Tag}_Out.csv -d' ' | sed -e 's/"//g'); do date --date="@$k" +%H:%M; done) <(cut -d' ' -f2 "${TMPDir}/${Tag}_Out.csv")  > "${TMPDir}/${Tag}_In.csv"


	# Plot Graph
	gnuplot <<< "set terminal dumb ${Width} ${Height};  set ytics border nomirror out ; set xtics border nomirror out ;plot \"${TMPDir}/${Tag}_Out.csv\" using 2:xticlabels(1) notitle with lines" | head -n ${CutCrap} 
	echo "    Port ${Port} ${PortName} - Inbound Traffic"

	echo "Next Graph? (Y/n)"
	read AskThem
	AskThemClean=${AskThem//[$'\t\r\n']} && AskThem=${AskThem%%*( )}
	if [[ $AskThemClean =~ (N|n) ]]
	then
		PortMenu
	else
		gnuplot <<< "set terminal dumb ${Width} ${Height};  set ytics border nomirror out ; set xtics border nomirror out ;plot \"${TMPDir}/${Tag}_In.csv\" using 2:xticlabels(1) notitle with lines" | head -n ${CutCrap} 
	echo "    Port ${Port} ${PortName} - Outbound Traffic"
	fi	
	echo ""
	echo "Legend has it that the Vertical Axis is adaptive Bytes/s"
	echo "The Horizontal Axis shows the last 24 hours of activity"

	# Clean up temp files
	rm "${TMPDir}/${Tag}_In.csv" "${TMPDir}/${Tag}2_In.csv" "${TMPDir}/${Tag}2_Out.csv" "${TMPDir}/${Tag}_Out.csv"
	echo "That was fun, what next?"
	Prompt
}

PortMenu
