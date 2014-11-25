efika-mpd
========
Efika MPD is a headless network music player built on [Efika PPC](http://genesi.company/products/efika/5200b). Although the Efika PCC has been discontinued I think the project might be useful for someone who'd like to built a simillar player using a Raspberry Pi. 

This project is the player's user interface which is used to control the MPD. The user interface uses a VFD as a display and a RC5 compatible remote control as an input.

Hardware
--------
The user interface board is custom PCB that holds a 16x2 alphanumeric VFD and an IR receiver. An ATmega8 MCU is used to control the VFD and decode RC5 signals. Efika is interfaced over UART. Cadsoft Eagle schematics for the ui board are provided in `schematics` folder. Firmware source for the MCU can be found in `firmware` folder.

Software
--------
The ui board is controled by a pice of Linux software. The program is a user space daemon which acts as MPD client and interfaces the ui board over a serial port. It simply takes information from MPD and displays on the VFD and reacts to remote control codes send by the ui board.
