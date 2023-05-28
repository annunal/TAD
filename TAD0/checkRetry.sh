#!/bin/bash
#
# V3.00 - Please update in case of changes

source /home/script/setVars.sh

cd $TAD
echo "$(date) retrying to transmit" >> retryLog.txt
sudo nice ./tad-retry retry 

