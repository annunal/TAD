#!/bin/bash
#
# V3.00 - Please update in case of changes

source /home/script/setVars.sh

cd $TAD
NOW=$(date +"%Y-%m-%d")
FILE="execLog_$NOW.txt"
echo $date >> $FILE
sudo nice ./tad  >> $FILE 
