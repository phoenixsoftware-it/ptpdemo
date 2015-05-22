#!/bin/sh

ptp4l -p /dev/ptp0 -f /opt/ptp/master.cfg -i eth0 &
