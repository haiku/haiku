#!/bin/bash
# Copyright 2014 Alexander von Gluck IV <kallisti5@unixzen.com>
# Released under the terms of the MIT license.

# Prepare a graphics card to be attached to a qemu virtual machine
# Requires Linux, and IOMMU support enabled on the motherboard.

echo ""
echo "Introduction"
echo "  This script will unhook the specified graphics card from the operating system so it can"
echo "  be safely attached to a virtual machine with IOMMU"
echo ""
echo "Installed graphics cards:"
echo "-----------------------------------------"
echo "Slot    Type                       Vendor"
lspci | grep "VGA compatible"
echo "-----------------------------------------"

echo ""
echo "WARNING: Selecting your current video device will result in it no longer being used by"
echo "the operating system.  Please ensure you choose your secondary video card."
echo ""
echo "Please enter the slot of the graphics card you wish to attach to a virtual machine:"
echo -n " > "

read PCISLOT

lspci -s $PCISLOT &> /dev/null
if [ $? -ne 0 ]; then
	echo "Error: Invalid PCI SLOT! ($PCISLOT)"
	exit 1
fi;

PCIID=$(lspci -n -s $PCISLOT | awk '{ print $3 }')
VENDOR=$(echo "$PCIID" | cut -d':' -f1)
DEVICE=$(echo "$PCIID" | cut -d':' -f2)

echo ""

sudo -v

# Enable unsafe assigned interrupts
sudo su -c "echo 1 > /sys/module/kvm/parameters/allow_unsafe_assigned_interrupts"
sudo su -c "modprobe pci_stub"

# cleanup (just incase)
sudo su -c "echo 0000:$PCISLOT > /sys/bus/pci/drivers/pci-stub/unbind"
sudo su -c "echo $VENDOR\ $DEVICE > /sys/bus/pci/drivers/pci-stub/remove_id"

# bind
sudo su -c "echo $VENDOR\ $DEVICE > /sys/bus/pci/drivers/pci-stub/new_id"
sudo su -c "echo 0000:$PCISLOT > /sys/bus/pci/devices/0000:$PCISLOT/driver/unbind"
sudo su -c "echo 0000:$PCISLOT > /sys/bus/pci/drivers/pci-stub/bind"

echo "Unbinding complete.  Feel free to use the GPU in qemu/kvm via '-device pci-attach,host=$PCISLOT'"
