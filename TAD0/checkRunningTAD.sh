#!/bin/bash
#
# V3.00 - Please update in case of changes

source /home/script/setVars.sh

NOW=$(date +"%Y-%m-%d")
FILE="checkLog_$NOW.txt"

logfile="$TAD/$FILE"

case "$(pidof tad | wc -w)" in

0)  echo "tad not running, restarting tad:     $(date)" >> $logfile
    sudo $TAD/execTAD.sh &
    ;;
1)  echo "tad running, all OK:     $(date)" >> $logfile
    ;;
*)  echo "multiple instances of tad running. Stopping & restarting tad:     $(date)" >> $logfile
    kill -9 $(pidof tad | awk '{print $1}')
    export defunct=$(ps aux | grep -w Z)
    echo ' processo defunto: $defunct'
    ;;
esac

sudo rm -f $TAD/webcritech.jrc.ec.europa.eu/SensorGrid/EnterData*
