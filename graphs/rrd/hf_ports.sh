#!/bin/bash
HTMLDir="/home/bpq/nodes/pe1rrr/graphs"
RRDDir="/home/bpq/nodes/pe1rrr/rrd"

declare -A Ports

Ports[3]="IC756 Pro Net105 50w"
Ports[4]="IC756 Pro BBS Fwd 50w"
Ports[5]="G90 Net105 15w"
Ports[6]="G90 BBS Fwd 15w"

# Filename Prefix and Title
GraphFilename="HFPorts"
GraphTitle="HF Ports"

source "${RRDDir}/shared.inc"
source "${RRDDir}/common.inc"

MakeAverage

GraphFilename="HFPortsOut"
GraphTitle="HF Ports Outbound"

MakeOut

GraphFilename="HFPortsIn"
GraphTitle="HF Ports Inbound"
 
MakeIn
