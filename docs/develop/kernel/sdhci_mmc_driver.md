# SDHCI MMC Driver
This driver project is a part of GSoC'18 and is aimed at providing support for PCI devices with class 8 and subclass 5 over x86 architecture. This document will make you familiar with the [code produced during GSoC](https://review.haiku-os.org/#/c/haiku/+/318/), loading and testing the driver(including hardware emulation), insight into the code and future tasks.

For detailed explanations about the project, you can refer the [weekly reports](https://www.haiku-os.org/blog/krish_iyer) and comment issues if any. For this project we have referred [SD Host Controller Spec Version 1.00](https://www.sdcard.org/downloads/pls/pdf/index.php?p=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.jpg&f=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.pdf&e=EN_A2100) and [Physical Layer Spec Version 1.10](https://www.sdcard.org/downloads/pls/pdf/index.php?p=Part1_Physical_Layer_Simplified_Specification_Ver1.10.jpg&f=Part1_Physical_Layer_Simplified_Specification_Ver1.10.pdf&e=EN_P1110).

## Loading and testing the  driver
### Emulating the hardware

We will emulate a SDHC device using qemu as all system may not have the device. These days systems provide transfer to SD/ MMC card over USB. The document will not instruct you on how to build haiku but you can refer the link to [compile and build the haiku images](https://www.haiku-os.org/guides/building/) or the [week #1 and #2](https://www.haiku-os.org/blog/krish_iyer/2018-05-06_gsoc_2018_sdhci_mmc_driver_week_1_and_2/) project report will also work. You can also cherry pick changes from another [ticket](https://review.haiku-os.org/#/c/haiku/+/448/)

After building the image, we will emulate the hardware and host haiku on top of that.
#### Installing Qemu
    apt-get install qemu
#### Creating Virtual Harddrive
    qemu-img create haiku-vm.img 10G
#### Booting OS in virtual drive
Note: if you generated a raw image during build then creating virtual hard drive and booting the OS in hard drive is not required.

    qemu-system-x86_64 -boot d -cdrom haiku/generated.x86_64/haiku-nightly-anyboot.iso -m 512 -hda haiku-vm.img
#### Hosting Haiku on x86
    qemu-system-x86_64 haiku-vm.img
#### Emulation
For emulating a sdhci-pci device

    qemu-img create sd-card.img 10G
    qemu-system-x86_64 ~/haiku/generated.x86_64/haiku.image -hdd haiku_drive_2.img -device sdhci-pci -device sd-card,drive=mydrive -drive if=sd,index=0,file=sd.img,format=raw,id=mydrive -m 512M -enable-kvm

### Testing and loading the driver
Note: This project's code has already been merged but currently disabled because it's not useful en user currently. In order to add binary to kernel and load it, we need to mention the package in build files.

##### :*build/jam/images/definitions/minimum*

In SYSTEM_ADD_ONS_BUS_MANAGERS and add following line under that.

    mmc_bus
##### :*haiku/build/jam/packages/Haiku*
Under # modules add

    AddFilesToPackage add-ons kernel busses mmc : sdhci_pci ;
Under #  boot module links

    sdhci_pci
Under # drivers add

    AddNewDriversToPackage disk mmc : mmc_disk ;
Above following code in the build files will include your module, in order to build with the kernel you need to execute

    cd generated.x86_64
    jam -q update-image kernel sdhci_pci mmc_disk mmc_bus
Now host haiku with sdhci-pci device, open the terminal and execute following commands:
Note: Don't worry if you don't understand the log output of the commands. 

    cat /var/log/syslog | grep "sdhci_pci\|mmc_bus\|mmc_disk"
If you are able to see some output of above-mentioned command then it means that you have successfully loaded the driver.

## Insight into the code and future tasks
### Directory and files where all the code related to the project resides

#### MMC Bus
*    src/add-ons/kernel/busses/mmc
    *    sdhci-pci.cpp
    *    sdhci-pci.h
    *    Jamfile
#### The Bus Manager
* src/add-ons/kernel/bus_managers/mmc
    * mmc_bus.cpp
    * mmc_module.cpp
    * mmc_bus.h
    * Jamfile
#### Disk Driver
* src/add-ons/kernel/drivers/disk/mmc
    * mmc_disk.cpp
    * mmc_disk.h
    * Jamfile
#### Hardcoding the driver
* src/system/kernel/device_manager
    * device_manager.cpp
### Insight into the code
#### MMC Bus
So there's some called PCI bus and it something which was developed in order to manage the load on the CPU and easy for the CPU to manage the devices. Now our controller, SDHC will be plugged to the PCI bus, also which has the control over card. Now the first job will be finding the controller form PCI bus from all the devices connected. Every device is a node which has certain info about the device. So we need to get the PCI bus node(parent node) and get the device and register another node which will be called a child node or MMC bus. So if suppose I have another driver which get data from MMC bus and register another node then in that case MMC bus will be a parent node and the node I will be registering with the information will be the child node. 

So **supports_device()** will do that job of finding the device of particular class and subclass. Now with **register_child_devices()** we will register the bus node and attach certain info like slot info etc. Now that we have a bus, we can read number of slots and map the register set for each one of them, these register will help us to operate the controller and drive the card. We mapped the register using MMUIO method where we take register address form physical memory and assign to virtual one. In order to do that we have declared structure(*struct registers*) with no zero padding so that mapping will be precise.

After we mapped the registers, resetting the register sets confirmed that register mapping is functional. We have then declared functions for setting up the clock, reset etc.

    static void sdhci_register_dump(uint8_t, struct registers*);
    static void sdhci_reset(struct registers*);
    static void sdhci_set_clock(struct registers*, uint16_t);
    static void sdhci_set_power(struct registers*);
    static void sdhci_stop_clock(struct registers*);
After this much we need to setup a interrupt handler which will call a function in the driver when something occurs

    status_t sdhci_generic_interrupt(void*);
#### The Bus Manager
We need something to take up the job of certain tasks like data transfer and return back to driver when job is done. Class was a ideal tool for us because as soon as the job is done we no longer any of the data which stays with the variable, we have destructor for deleting the object.

It is not linked for time being but will be useful at the time of data transfer etc.

#### Disk Driver
So it's main job is to publish slots somewhere like /dev/mmc/.. So after disk gets published, we can mount the device and do transfer function on the memory device(SD/ MMC).

#### Hardcoding the driver
In order to make the driver loadable and to tell the kernel that we really has built the support for a particular device, we need to mention the class and location of the driver module to the device_manager.
### Tasks to be completed
We have to get the response from the card of commands which controller issues. We have tried a lot and looked into more deeper aspect but nothing worked, I am sure we have really missed something. The detailed report on responses and testing will found in last few weeks report.

Once we get the response from the card, we can complete the power sequence and so does other sequence which are mentioned in [third phase outline](https://www.haiku-os.org/blog/krish_iyer/2018-07-12_gsoc_2018_sdhci_mmc_driver_third_phase_plan/).

If you find it difficult to understand the driver development and it's functioning and role, please refer *docs/develop/kernel/device_manager_introduction.html*
