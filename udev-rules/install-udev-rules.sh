#!/bin/bash

#
# Verify that script is running under root privileges.
#
# @author ptierno (https://stackoverflow.com/users/2671317/ptierno)
# @link https://stackoverflow.com/a/18216122
# @attention licensed under CC BY-SA 4.0 (https://creativecommons.org/licenses/by-sa/4.0/).
#
if [ "$EUID" -ne 0 ]
then
	echo "Please run as root."
  	exit 1
fi

#
# Verify that udev directory exists.
#
# @author Grundlefleck (https://stackoverflow.com/users/4120/grundlefleck)
# @link https://stackoverflow.com/a/59839
# @attention licensed under CC BY-SA 4.0 (https://creativecommons.org/licenses/by-sa/4.0/).
#
DIRECTORY="/etc/udev/rules.d"
if [ ! -d "$DIRECTORY" ]; 
then
	echo "Directory $DIRECTORY does not exist."
  	exit 1
fi

#
# Copy file to udev directory.
#
CURRENT_DIRECTORY=$(dirname "$0")
cp "$CURRENT_DIRECTORY/100-kommpot.rules" "$DIRECTORY"

#
# Restart udev service to load new rules.
#
# @author Ignacio Vazquez-Abrams (https://unix.stackexchange.com/users/9785/ignacio-vazquez-abrams)
# @link https://unix.stackexchange.com/a/39371
# @attention licensed under CC BY-SA 3.0 (https://creativecommons.org/licenses/by-sa/3.0/).
#
udevadm control --reload-rules
udevadm trigger
