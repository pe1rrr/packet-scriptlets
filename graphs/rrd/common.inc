#!/bin/bash
if [ $FullSize = "Y" ]
then
	Options+="--full-size-mode"
fi

function MakeAverage() {
	local Cheese
	local Cheesed
	Cheesed=0

	Cheesed=0
	IFS=,

	Defs="\\"
	for Port in ${!Ports[@]};
		do
		PortName=${Ports[$Port]}
		Shade=`echo ${Shades[$Cheesed]}`
		Title=$(printf %q "${PortName}") # Sanitize spaces and special chars
		Defs+="DEF:Port${Port}In=${RRDDir}/port${Port}.rrd:in:AVERAGE \\" 
		Defs+="DEF:Port${Port}Out=${RRDDir}/port${Port}.rrd:out:AVERAGE \\"
		Defs+="CDEF:Port${Port}Total=Port${Port}In,Port${Port}Out,2,/,+ \\"
		if [ ${Cheesed} -eq 0 ];
		then
			Cheese+="AREA:Port${Port}Total#${Shade}:${Title} \\"
		else
			Cheese+="AREA:Port${Port}Total#${Shade}:${Title}:STACK \\"
		fi
		Cheesed=$((${Cheesed}+1))
	 done

	 Port="HF Ports"

	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Hourly}\\\n' ${HTMLDir}/${GraphFilename}_1hr.png --end now --start end-3600s ${Defs}${Cheese}"
	echo ""

	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Daily}\\\n' ${HTMLDir}/${GraphFilename}_24hr.png --end now --start end-86400s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Weekly}\\\n' ${HTMLDir}/${GraphFilename}_1wk.png --end now --start end-604800s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Monthly}\\\n' ${HTMLDir}/${GraphFilename}_1mo.png --end now --start end-672h ${Defs}${Cheese}"

	echo ""
	IFS=$Field_Separator
}


function MakeIn() {
	local Defs
	local Cheese
	local Cheesed
	Cheesed=0
	IFS=,
	Defs="\\"

	for Port in ${!Ports[@]};
		do
		PortName=${Ports[$Port]}
		Shade=`echo ${Shades[$Cheesed]}`
		Title=$(printf %q "${PortName}") # Sanitize spaces and special chars
		Defs+="DEF:Port${Port}In=${RRDDir}/port${Port}.rrd:in:AVERAGE \\" 
		if [ ${Cheesed} -eq 0 ];
		then
			Cheese+="AREA:Port${Port}In#${Shade}:${Title} \\"
		else
			Cheese+="AREA:Port${Port}In#${Shade}:${Title}:STACK \\"
		fi
		Cheesed=$((${Cheesed}+1))
	 done
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Hourly}\\\n' ${HTMLDir}/${GraphFilename}_1hr.png --end now --start end-3600s ${Defs}${Cheese}"
	echo ""

	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Daily}\\\n' ${HTMLDir}/${GraphFilename}_24hr.png --end now --start end-86400s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Weekly}\\\n' ${HTMLDir}/${GraphFilename}_1wk.png --end now --start end-604800s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Monthly}\\\n' ${HTMLDir}/${GraphFilename}_1mo.png --end now --start end-672h ${Defs}${Cheese}"

	echo ""
	IFS=$Field_Separator
 }


function MakeOut() {
	local Defs
	local Cheese
	local Cheesed
	Cheesed=0
	IFS=,
	Defs="\\"

	for Port in ${!Ports[@]};
		do
		PortName=${Ports[$Port]}
		Shade=`echo ${Shades[$Cheesed]}`
		Title=$(printf %q "${PortName}") # Sanitize spaces and special chars
		Defs+="DEF:Port${Port}Out=${RRDDir}/port${Port}.rrd:out:AVERAGE \\" 
		if [ ${Cheesed} -eq 0 ];
		then
			Cheese+="AREA:Port${Port}Out#${Shade}:${Title} \\"
		else
			Cheese+="AREA:Port${Port}Out#${Shade}:${Title}:STACK \\"
		fi
		Cheesed=$((${Cheesed}+1))
	 done
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Hourly}\\\n' ${HTMLDir}/${GraphFilename}_1hr.png --end now --start end-3600s ${Defs}${Cheese}"
	echo ""

	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Daily}\\\n' ${HTMLDir}/${GraphFilename}_24hr.png --end now --start end-86400s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Weekly}\\\n' ${HTMLDir}/${GraphFilename}_1wk.png --end now --start end-604800s ${Defs}${Cheese}"

	echo ""
	echo -e "rrdtool graph -v '$Vertical' -h ${Height} -w ${Width} ${Options} --title '${GraphTitle} - ${Monthly}\\\n' ${HTMLDir}/${GraphFilename}_1mo.png --end now --start end-672h ${Defs}${Cheese}"

	echo ""
	IFS=$Field_Separator
 }

