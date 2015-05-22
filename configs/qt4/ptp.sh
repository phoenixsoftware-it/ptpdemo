#/bin/bash

pipe=/tmp/ptpFifo

trap "rm -f $pipe" EXIT

if [[ ! -p $pipe ]]; then
    mkfifo $pipe
fi
   
killall -9 ptp4l

cd "$(dirname "$0")/"
./ptp4l -m -f /opt/ptp/slave.cfg -i eth0 > /tmp/ptpFifo &
./phc2sys -c CLOCK_REALTIME -s /dev/ptp0 -O 1 -t 0.0001 &
./PTPDemo $1 
                                  
echo "PTP Demo finished succesfully"
