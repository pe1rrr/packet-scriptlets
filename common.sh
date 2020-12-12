#!/bin/bash
# Configuration file
lynx="/usr/bin/lynx"  # sudo apt install lynx
curl="/usr/bin/curl"  # sudo apt install curl
weblogfile="/var/log/bpq-browser.log" # sudo touch /var/log/bpq-browser; sudo chmod bpq:bpq /var/log/bpq-browser

# It is recommended to set up a proxy server locally to handle the requests from this script
# it adds a level of control over which content can and cannot be requested, squid proxy is
# utilized for this, but if you do not want to use it, comment the 2 lines out below.
# I have set up my squid proxy to use alternate DNS servers of OpenDNS FamilyShield.

myproxy="http://127.0.0.1:3128"
export http_proxy=$myproxy
