#!/bin/sh
# too clever for its own good..
#eval "./ev3duder test 2>/dev/null | head -1 > /etc/udev/rules.d"
sudo cp ev3-udev.rules /etc/udev/rules.d/ev3.rules

