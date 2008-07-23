#!/bin/sh

# run_flphone.sh


export LD_LIBRARY_PATH=/usr/flphone/lib:/usr/lib
export MWFONTDIR=/usr/flphone/share/fonts/
export PATH=/usr/flphone/sbin:/usr/flphone/bin:$PATH

# Start the TBUS server
tbus-daemon >/tmp/logtbus 2>/tmp/logtbus2 &

# start multiplexer daemon
# -r options is not working....
gsmMuxd -p /dev/ttyS1 -w -s /dev/mux  /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx /dev/ptmx

# start indication server daemon
ipc_dsy  2>/tmp/logdsy1 &
# start extension server
ipc_ext  2>/tmp/logext1 &
# start sound server daemon
ipc_sound  2>/tmp/logsnd1 &

# Start GSMD
gsmd -p /dev/mux1 -s 38400 -v siemens -m sx1 >/tmp/loggsmd 2>/tmp/loggsmd2 &

# Start the Nano-X server
nano-X &
# Start up the Nano-X window manager
nanowm &

# Start Indicator daemon
indicatord &

alarmserver
