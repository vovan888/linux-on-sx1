#!/bin/sh

# run_flphone.sh


export LD_LIBRARY_PATH=/usr/flphone/lib:/usr/lib
export MWFONTDIR=/usr/flphone/share/fonts/

# Start the colosseum server
/usr/flphone/sbin/clserver &

# start multiplexer daemon
# -r options is not working....
/usr/flphone/sbin/gsmMuxd -p /dev/ttyS1 -w -s /dev/mux  /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx

# start indication server daemon
/usr/flphone/sbin/ipc_dsy  2>/tmp/logdsy1 &
# start extension server
/usr/flphone/sbin/ipc_ext  2>/tmp/logext1 &
# start sound server daemon
/usr/flphone/sbin/ipc_sound  2>/tmp/logsnd1 &

sleep 1
# Start GSMD
/usr/flphone/sbin/gsmd -p /dev/mux1 -s 38400  &

sleep 10

# Start the Nano-X server
/usr/bin/nano-X &
sleep 1
# Start up the Nano-X window manager
/usr/flphone/sbin/nanowm &

sleep 1

# Start Indicator daemon
/usr/flphone/sbin/indicatord &

/usr/flphone/sbin/alarmserver
