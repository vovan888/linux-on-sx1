#!/bin/sh

# run_flphone.sh


export LD_LIBRARY_PATH=/lib:/usr/flphone/lib:/usr/lib
export MWFONTS=/usr/flphone/share/fonts/
export PATH=/usr/flphone/sbin:/usr/flphone/bin:$PATH

# Start the T-BUS server
t-bus >/tmp/logtbus 2>/tmp/logtbus2 &
# Start the T-HAL server
t-hal >/tmp/logthal 2>/tmp/logthal2 &
# start multiplexer daemon
# -r options is not working....
gsmMuxd -n 6 -p /dev/ttyS1 >/tmp/logmux 2>/tmp/logmux2 &
sleep 1
# start indication server daemon
ipc_dsy  2>/tmp/logdsy1 &
# start extension server
ipc_ext  2>/tmp/logext1 &
# start sound server daemon
ipc_sound  2>/tmp/logsnd1 &

# Start GSMD2
gsmd2 -p /dev/mux1 -P /dev/mux2 -s 38400 -v siemens -m sx1 >/tmp/loggsmd 2>/tmp/loggsmd2 &

# Start the Nano-X server
nano-X &
# Start up the Nano-X window manager
nanowm &

# Start Indicator daemon
indicatord &
# start alarm server
alarmserver &

# ask for PIN-code
pin-enter
#start phone app
phone

# shutdown all the things...
killall alarmserver
killall indicatord
killall nanowm
#killall gsmd2
killall nano-X

sync
umount -a -r
sx1_poweroff
