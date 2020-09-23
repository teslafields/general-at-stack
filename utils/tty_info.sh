#!/bin/bash

num_of_ifaces=0
start_iface=100
foo[${#foo[*]}]="e"
for devpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
    syspath="${devpath%/dev}"
    devname="$(udevadm info -q name -p $syspath)"
    [[ "$devname" == "bus/"* ]] && continue
    eval "$(udevadm info -q property --export -p $syspath)"
    [[ -z "$ID_SERIAL" || "$ID_VENDOR" != "SimTech"* ]] && continue
    # echo "/dev/$devname $ID_VENDOR"
    devnum[${#devnum[*]}]=$ID_USB_INTERFACE_NUM
    num_of_ifaces=$((num_of_ifaces + 1))
    if [ $ID_USB_INTERFACE_NUM -lt $start_iface ]; then
        start_iface=$ID_USB_INTERFACE_NUM
    fi
done
#echo ${devnum[@]}
echo $start_iface $num_of_ifaces
