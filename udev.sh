#!/bin/sh
# too clever for its own good..
#eval "./ev3duder test 2>/dev/null | head -1 > /etc/udev/rules.d"
sudo cp 99-ev3duder.rules /etc/udev/rules.d/

sudo udevadm control --reload-rules
sudo udevadm trigger