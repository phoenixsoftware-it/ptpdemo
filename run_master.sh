#!/bin/sh

./ptp4l -f /opt/ptp/master.cfg -i eth0 &
./phc2sys -c /dev/ptp0 -s CLOCK_REALTIME -O 1 -t 0.0001 &
