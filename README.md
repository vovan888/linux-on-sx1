linux-on-sx1
============

Linux on Siemens SX1 project

Uses: 
- FLNX GUI toolkit (this is a nano-x version of FLTK).
- Nano-X small X-server
- customized Nanowm window manager
- TPL serialization library (http://tpl.sourceforge.net)
- gsmmuxd - GSM multiplexing daemon
- Tiny message bus system - TBUS

Kernel support is already in kernel:
config MACH_SX1
http://lxr.free-electrons.com/source/arch/arm/mach-omap1/board-sx1.c
http://lxr.free-electrons.com/source/drivers/video/omap/lcd_sx1.c?v=2.6.25
