#!/bin/bash
# include common configuration
DIR="${BASH_SOURCE%/*}"
if [[ ! -d "$DIR" ]]; then DIR="$PWD"; fi
. "$DIR/../config.sh"

# User parameters
#
wxfile1="https://www.cpc.ncep.noaa.gov/products/analysis_monitoring/regional_monitoring/wcmin1.png"
wxname1="eu_min"
wxfile2="https://www.cpc.ncep.noaa.gov/products/analysis_monitoring/regional_monitoring/wcmax1.png"
wxname2="eu_max"

#
# format: DEST @ AREA
destination="SB PIC @ EU"
#
# Message chunk/part size in bytes
msgchunk="10000"
#



######## Do not modify below this line
# Week number
week=`date +%U`
#
# Bulletin info
blurb="Week $week Min and Max temperatures for Europe"

# eventual zip file archive name with week number
wxzip="euwxwk${week}"

# tmp scratch disk
wxtmp="${tmp}/${wxzip}"
mkdir -p $wxtmp

# Fetch WX Map
wget $wxfile1 -O "${wxtmp}/${wxname1}.png"
wget $wxfile2 -O "${wxtmp}/${wxname2}.png"

# Convert PNG to JPG and reduce quality to minimize size
/usr/bin/convert -quality 40 -resize 500x500  "${wxtmp}/${wxname1}.png" "${wxtmp}/${wxname1}.jpg"
/usr/bin/convert -quality 40 -resize 500x500  "${wxtmp}/${wxname2}.png" "${wxtmp}/${wxname2}.jpg"

# Collate the images into 1 zip
zip -j -9 "euwxwk${week}" "${wxtmp}/${wxname1}.jpg" "${wxtmp}/${wxname2}.jpg"

# Produce the 7plus.upl file
${plusbin} -send2 "${destination} < ${mycall}" -t /ex -sb ${msgchunk} -j "${wxzip}.zip"

echo "Sending ${wxzip}.upl to BBS unless you CTRL-C... sleeping 10s"
sleep 10

# Send to the BBS
echo -e "${destination} < ${mycall}\r\n${wxzip}.zip info\r\n${blurb}\r\n/ex\r\n" >> "${importfile}"
cat "${wxzip}.upl" >> "${importfile}"

echo "Done, removing ${wxzip}.upl"
rm "${wxzip}.upl"
rm "${wxzip}.zip"
