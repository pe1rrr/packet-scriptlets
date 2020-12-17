#!/bin/bash
# Original Script by G7TAJ, Modified by PE1RRR
# sudo apt install rrdtool
# Add this script to a crontab to run every minute
# Example 
# crontab -e
# * * * * * /home/bpq/nodes/pe1rrr/scripts/snmp.sh > /dev/null 2>&1
#
# BPQ needs to be configured with an IPGATEWAY, check the BPQ documentation for that as well as below.
#
# Make sure to add ENABLESNMP to the IPGATEWAY configuration
#
# Example bpq32.cfg Configuration: IPADDR must be an IP address from your LAN that is NOT in use.
# This will launch a numberless LinBPQTAP device. Adapter must be your actual LAN interface.
#
# IPGATEWAY
# Adapter eth0
# NODEFAULTROUTE
# IPADDR 192.168.1.9
# IPNETMASK 255.255.255.0
# ENABLESNMP
# ****

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


