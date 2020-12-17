#!/bin/bash
# ASCII Graphs! By PE1RRR
# - sudo apt install gnuplot
#
RRDDir="/home/bpq/nodes/pe1rrr/rrd"
TMPDir="${RRDDir}/tmp"
#
#
#
declare -A Ports

Ports[2]="APRS 144.800MHz"
Ports[3]="IC756 Pro Net105 50w"
Ports[4]="IC756 Pro BBS Fwd 50w"
Ports[5]="G90 Net105 15w"
Ports[6]="G90 BBS Fwd 15w"
Ports[10]="Telnet"
Ports[11]="AX/UDP"
Ports[12]="AX/IP"

# Globals
QuitRegex='^(Q|q|B|b)$'
Width="60"
Height="20"

function PortMenu() {
	echo "...it thickens!"
	echo "iNotice: This tool is currently completely meaningless as I am still working on it"
	echo "         Some charts may show a warning & are temporarily empty. That's all your fault."
	echo ""
	echo "Welcome, please choose a port:"
	echo "[2]  APRS 144.800 Mhz"
	echo "[3]  HF /Net105/ 14.102 USB @ 1300hz Center"
	echo "[4]  HF BBS Fwd 14.102 USB @ 1900hz Center"
	echo "[5]  HF QRP (interchanged w/Port 3) /Net105/ 14.102 USB @ 1300hz Center"
	echo "[6]  HF QRP (interchanged w/Port 4) BBS Fwd 14.102 USB @ 1900hz Center"
	echo "[10] Telnet Interface"
	echo "[11] Internet AXUDP Links"
	echo "[12] Internet AXIP Proto 93 Links"
	echo ""
	Prompt
}

function Prompt() {
	local Choice
	local ChoiceTrimmed

	echo "Port? (Q=quit) >"

	read ChoiceTrim

	Choice=${ChoiceTrim//[$'\t\r\n']} && ChoiceTrim=${ChoiceTrim%%*( )}

	if [[ $Choice =~ $QuitRegex ]];
	then
		Quit
	fi


	if [[ ${Choice} =~ ^(2|3|4|5|6|10|11|12)$ ]];
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
	rrdtool fetch --end now --start end-1h ${RRDDir}/port${Port}.rrd AVERAGE | tail -n +3 | awk -F ' ' '{print $1,$2}'  | grep -v 'nan' | sed 's/^/"/g;s/:/"/g' > ${TMPDir}/${Tag}_Out.csv 
	rrdtool fetch --end now --start end-1h ${RRDDir}/port${Port}.rrd AVERAGE | tail -n +3 | awk -F ' ' '{print $1,$3}'  | grep -v 'nan' | sed 's/^/"/g;s/:/"/g' > ${TMPDir}/${Tag}_In.csv 

	# Smush data
	paste <(for i in {1..$(wc -l "${TMPDir}/${Tag}_Out.csv" | cut -d' ' -f1)}; do echo $i; done) <(cut -d' ' -f 2 "${TMPDir}/${Tag}_Out.csv") > "${TMPDir}/${Tag}2_Out.csv"
	paste <(for i in {1..$(wc -l "${TMPDir}/${Tag}_In.csv" | cut -d' ' -f1)}; do echo $i; done) <(cut -d' ' -f 2 "${TMPDir}/${Tag}_In.csv") > "${TMPDir}/${Tag}2_In.csv"

	# Plot Graph
	gnuplot <<< "set terminal dumb ${Width} ${Height};  plot \"${TMPDir}/${Tag}_Out.csv\" using 2:xticlabels(1) notitle with lines" | head -n ${CutCrap} 
	echo "    Port ${Port} ${PortName} - Inbound Traffic"
	gnuplot <<< "set terminal dumb ${Width} ${Height}; plot \"${TMPDir}/${Tag}_In.csv\" using 2:xticlabels(1) notitle with lines" | head -n ${CutCrap} 
	echo "    Port ${Port} ${PortName} - Outbound Traffic"
	echo ""
	echo "Legend has it that the Vertical Axis is adaptive Bytes/s"
	echo "The Horizontal Axis shows the last 60 mins of activity, each column is 1 minute"        

	# Clean up temp files
	rm "${TMPDir}/${Tag}_In.csv" "${TMPDir}/${Tag}2_In.csv" "${TMPDir}/${Tag}2_Out.csv" "${TMPDir}/${Tag}_Out.csv"
	echo "That was fun, what next?"
	Prompt
}

PortMenu
