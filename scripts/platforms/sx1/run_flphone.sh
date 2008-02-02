#!/bin/sh

# run_flphone.sh


export LD_LIBRARY_PATH=/usr/flphone/lib:/usr/lib
export MWFONTDIR=/usr/flphone/share/fonts/

# Start the colosseum server
/usr/flphone/sbin/clserver &
# Start the Nano-X server
/usr/bin/nano-X &
# Start up the Nano-X window manager
/usr/flphone/sbin/nanowm &

sleep 1

# Start Indicator daemon
/usr/flphone/sbin/indicatord &

/usr/flphone/sbin/alarmserver &
