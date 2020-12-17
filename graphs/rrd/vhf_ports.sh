#!/bin/bash
HTMLDir="/home/bpq/nodes/pe1rrr/graphs"
RRDDir="/home/bpq/nodes/pe1rrr/rrd"

declare -A Ports

Ports[2]="144.800Mhz APRS"

# Filename Prefix and Title
GraphFilename="VHFPorts"
GraphTitle="VHF Ports"

source "${RRDDir}/shared.inc"
source "${RRDDir}/common.inc"

MakeAverage

GraphFilename="VHFPortsOut"
GraphTitle="VHF Ports Outbound"

MakeOut

GraphFilename="VHFPortsIn"
GraphTitle="VHF Ports Inbound"
 
MakeIn
