#!/bin/bash
# V3.00 - Please update in case of changes

 pidof tad |xargs sudo kill 
cp $TAD/src/tad  $TAD
sudo $TAD/checkRunningTAD.sh
