#!/bin/bash
HTMLDir="/home/bpq/nodes/pe1rrr/graphs"
RRDDir="/home/bpq/nodes/pe1rrr/rrd"

declare -A Ports

Ports[10]="Telnet"
Ports[11]="AX/UDP"
Ports[12]="AX/IP"

# Filename Prefix and Title
GraphFilename="InetPorts"
GraphTitle="Internet Ports"

source "${RRDDir}/shared.inc"
source "${RRDDir}/common.inc"

MakeAverage

GraphFilename="InetPortsOut"
GraphTitle="Internet Ports Outbound"

MakeOut

GraphFilename="InetPortsIn"
GraphTitle="Internet Ports Inbound"
 
MakeIn
