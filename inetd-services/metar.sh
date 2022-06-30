#!/bin/bash
phpbin="/usr/bin/php"
LynxBin="/usr/bin/lynx"
WgetBin="/usr/bin/wget"
Figlet="/usr/bin/figlet"
MailFile="/home/bpq/nodes/pe1rrr/fbb/var/ax25/fbb/mail/mail.in"
FtpRoot="ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded"

Code=$1
Location=$2
MailTo=$3
MailFrom=$4
date=`date +'%D %H:00'`

function GetMetar {
	Text=`$LynxBin -useragent=WxMetar_L_y_n_x -unique_urls -number_links -hiddenlinks=ignore -nolist -nomore -trim_input_fields -justify -dump  ${URL} | sed -e 's/Station name not available//g'`
	#	sed '/^$/d'`

}

function MakeHeader {
	Header+=`${Figlet} -ptrf mini -w 60 $MailTo -  ${Location}  METAR`
}


URL="${FtpRoot}/${1}.TXT"

MakeHeader
GetMetar
OldIFS=$IFS
IFS=$''

Bulletin=`cat <<EOT
SB METAR @ ${MailTo}  < ${MailFrom}
WX ${Location} ${date}
$Header
$Text
/EX
EOT`

echo $Bulletin 
echo $Bulletin >> $MailFile
IFS=$OldIFS
