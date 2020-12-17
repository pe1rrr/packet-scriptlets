#!/bin/bash

IpAddr="192.168.1.9"
PortList="2,3,4,5,6,7,10,11,12,16"
Field_Separator=$IFS
RRDDir="/home/bpq/nodes/pe1rrr/rrd"

# set comma as internal field separator for the string list
IFS=,
for val in $PortList;
 do
# echo snmpget -Oqv -v1 -c public 44.131.253.72  1.3.6.1.2.1.2.2.1.10.$val
 echo update ${RRDDir}/port$val.rrd

  in=$(snmpget -r5 -Oqv -v1 -c public $IpAddr  1.3.6.1.2.1.2.2.1.10.$val )
  sleep 0.25
  out=$(snmpget -r5 -Oqv -v1 -c public $IpAddr 1.3.6.1.2.1.2.2.1.16.$val )
  echo $in $out

  rrdtool update ${RRDDir}/port${val}.rrd N:$in:$out
  sleep 0.75
 done

IFS=$Field_Separator


