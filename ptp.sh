#/bin/bash

pipe=/tmp/ptpFifo

trap "rm -f $pipe" EXIT

if [[ ! -p $pipe ]]; then
    mkfifo $pipe
fi
   
killall -9 ptp4l

cd "$(dirname "$0")/"
./ptp4l -m -p /dev/ptp0 -f /opt/ptp/slave.cfg -i eth0 > /tmp/ptpFifo &
./PTPDemo 
                                  
echo "PTP Demo finished succesfully"
