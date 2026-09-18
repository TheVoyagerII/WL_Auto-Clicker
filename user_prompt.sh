#!/bin/bash

# Making a hotkey file in advance
mkdir -p ~/.config/WL_Auto-Clicker
echo "ctrl+\`" > ~/.config/WL_Auto-Clicker/HOTKEY.txt
# This is part 1: Getting Permissions to build the device
uinput_perms_file="77-WLautoclicker.rules"
echo "Welcome to WL Autoclicker! Choose if you wish to add the autoclicker functionality to just this account [A] (default option), or multiple accounts [m] on this device.
### You will be prompted to enter your password.
## If m is selected, the script makes a new group on your device and adds your username to it.
## The instructions to add more users are listed in the README.
Enter [A]/[m]: "
read usr_or_grp_option
if [[ "$usr_or_grp_option" == "m" || "$usr_or_grp_option" == "M" ]]; then
    read -p "Enter desired group name: " grpname
    while getent group "$grpname" ; do
        members=$(getent group "$grpname"  | cut -d: -f4)
        if [ -z "$members" ]; then
            echo "Group exists and is empty, using $grpname ..."
            break
            else
            read -p "Group has members, please use different name: " grpname
        fi
    done
    sudo groupadd -f "$grpname"
    sudo usermod -aG "$grpname" "$USER"
    echo "ACTION==\"add\",KERNEL=\"uinput\",MODE=\"660\",GROUP=\"$grpname\"" > "$uinput_perms_file"
else
    echo "ACTION==\"add\",KERNEL=\"uinput\",MODE=\"600\",OWNER=\"$USER" > "$uinput_perms_file"
fi
echo "The Auto-Clicker needs to know which keyboard device is the input received from. \
The setup will run a command that lists available devices and some information pertaining to them. \
One of them will be Handlers flag, which contains an event entry next to them. \
Please enter the number specified next to the event entry on the next prompt.\nPress c to continue...."
read -n 1 -s continued
echo
grep -A6 -i "keyboard" "/proc/bus/input/devices"
read -p "Please enter the number next to 'event': " event_num
while [ ! -e "/dev/input/event$event_num" ]; do
    echo "\n"
    read -p "Entry event$event_num not found. Please try again: " event_num
done
echo "Entry successful! Proceeding...\n"
echo $event_num > ~/.config/WL_Auto-Clicker/EVENT_NUM.txt
sudo mv "$uinput_perms_file" "/etc/udev/rules.d/"
sudo udevadm control --reload
sudo udevadm trigger -c "add" -y "uinput"

if groups "$USER" | grep -qw "input"; then
    echo "$USER in input group already. Skip new session..."
else
    sudo usermod -aG "input" "$USER"
    new_session_need = true
fi

if [[ "$new_session_need" = true ]]; then
    if [ -z "$ran_from_appimg" ]; then
        echo "Group permissions updated. Starting new terminal session..."
        echo "Note: You are in a subshell of the previous session. Do not exit until the process is done, or open a fresh terminal session instead."
        exec bash
    else
        echo "Group permissions updated. Please log out, then log in again, and run Appimage again.\n"
    fi
fi