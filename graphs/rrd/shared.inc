#!/bin/bash

# Wording 
Hourly="Last Hour"
Daily="Last 24 Hours"
Weekly="Last 7 Days"
Monthly="Last 4 Weeks"
InGraph="Incoming"
OutGraph="Outgoing"
Vertical="Bytes/s"

# Size
Height="200"
Width="1000"
FullSize="Y" # Y or N


# Graph Line Colours + Transparency
declare -A Shades

Shades[0]="ff0000ff"
Shades[1]="00ff0080"
Shades[2]="0000ff80"
Shades[3]="ff00ff80"
Shades[4]="00ffff80"
Shades[5]="ffff0080"
Shades[6]="fff00080"
