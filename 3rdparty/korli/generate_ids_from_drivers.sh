#!/bin/sh

# 3com
# atheros813x atheros81xx attansic_l1 attansic_l2
# broadcom440x broadcom570x
# dec21xxx
# ipro100 ipro1000
# intel22x
# jmicron2x0
# marvell_yukon
# nforce
# pcnet
# rtl8139 rtl81xx
# sis19x sis900 syskonnect
# via_rhine vt612x

TOPDIR=$PWD/..

pciBsdEtherDriver()
{
driverPath=src/add-ons/kernel/drivers/network/ether/$1
bsdname=$2
headername=$3
table=$4
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_${bsdname}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include $TOPDIR/$driverPath/dev/$headername/if_${headername}${5}.h - | sed -E -n "/${bsdname}_${table}\[/,/^$/p" | sed -r -e 's/.*0x([^ ,]+), 0x([^ ,]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | uniq

}

pciBsdWlanDriver()
{
driverPath=src/add-ons/kernel/drivers/network/wlan/$1
bsdname=$2
headername=$3
table=$4
headersuffix=$5
sourcesuffix=$6
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include $TOPDIR/$driverPath/dev/$headername/if_${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^\};$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ ,]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

}


pciBsdEtherDriver  3com xl xl devs reg
pciBsdEtherDriver  atheros813x alc alc ident_table reg
pciBsdEtherDriver  atheros81xx ale ale devs reg
pciBsdEtherDriver  attansic_l1 age age devs reg
pciBsdEtherDriver  attansic_l2 ae ae devs var
pciBsdEtherDriver  broadcom440x bfe bfe devs reg
pciBsdEtherDriver  broadcom570x bge bge devs reg


#dec21xxx
driverPath=src/add-ons/kernel/drivers/network/ether/dec21xxx
bsdname=dc
headername=dc
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_dcreg.h > /tmp/if_dcreg.h
echo "#undef DC_DEVID" >> /tmp/if_dcreg.h
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_${bsdname}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include /tmp/if_dcreg.h - | sed -E -n "/${bsdname}_devs\[|${bsdname}_products\[|${bsdname}_ident_table\[/,/^$/p" | sed -r -e 's/[^0x]*0x([^ ,]+).*0x([^\)]+)\).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | uniq
rm /tmp/if_dcreg.h


#intel22x
driverPath=src/add-ons/kernel/drivers/network/ether/intel22x
bsdname=igc
headername=
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/igc_hw.h > /tmp/igc_hw.h
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_igc.c | gcc -E -include /tmp/igc_hw.h - | sed -E -n "/igc_vendor_info_array\[/,/PVID_END/p" | sed -r -e 's/.*0x([^ ,]+), 0x([^,]+),.*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }'
rm /tmp/igc_hw.h

pciBsdEtherDriver  ipro100 fxp fxp ident_table reg


#e1000
driverPath=src/add-ons/kernel/drivers/network/ether/ipro1000
bsdname=e1000
headername=
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/e1000_hw.h > /tmp/e1000_hw.h
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_em.c | gcc -E -include /tmp/e1000_hw.h - | sed -E -n "/em_vendor_info_array\[/,/PVID_END/p" | sed -r -e 's/.*0x([^ ,]+), 0x([^,]+),.*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }'
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_em.c | gcc -E -include /tmp/e1000_hw.h - | sed -E -n "/igb_vendor_info_array\[/,/PVID_END/p" | sed -r -e 's/.*0x([^ ,]+), 0x([^,]+),.*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }'
rm /tmp/e1000_hw.h

pciBsdEtherDriver  jmicron2x0 jme jme devs reg
pciBsdEtherDriver  marvell_yukon msk msk products reg
pciBsdEtherDriver  nforce nfe nfe devs reg
pciBsdEtherDriver  pcnet pcn pcn devs reg
pciBsdEtherDriver  rtl81xx re rl devs reg
pciBsdEtherDriver  rtl8139 rl rl devs reg
pciBsdEtherDriver  sis19x sge sge devs reg
pciBsdEtherDriver  sis900 sis sis devs reg
pciBsdEtherDriver  syskonnect sk sk devs reg
pciBsdEtherDriver  via_rhine vr vr devs reg
pciBsdEtherDriver  vt612x vge vge devs reg

#pegasus
driverPath=src/add-ons/kernel/drivers/network/ether/pegasus
sed -e 's/#include.*//g' $TOPDIR/$driverPath/driver.c | gcc -E -include objects/common/libs/compat/freebsd_network/usbdevs.h - | sed -E -n "/supported_devices\[/,/^$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/.*0x([^ ,]+), 0x([^ \}]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "usb %s %s .. .. .. .. .. .. 0000 ffff : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

#usb_asix
driverPath=src/add-ons/kernel/drivers/network/ether/usb_asix
sed -e 's/#include.*//g' $TOPDIR/$driverPath/Driver.cpp | gcc -E - | sed -E -n "/gSupportedDevices\[/,/^$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \}]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "usb %s %s .. .. .. .. .. .. 0000 ffff : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

echo "usb .... .... .. .. .. 02 06 00 0000 ffff : CONFIG__UNKNOWN__ : src/add-ons/kernel/drivers/network/ether/usb_ecm"
echo "usb .... .... .. .. .. e0 01 03 0000 ffff : CONFIG__UNKNOWN__ : src/add-ons/kernel/drivers/network/ether/usb_rndis"

#usb_davicom
driverPath=src/add-ons/kernel/drivers/network/ether/usb_davicom
sed -e 's/#include.*//g' $TOPDIR/$driverPath/Driver.cpp | gcc -E - | sed -E -n "/gSupportedDevices\[/,/^$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \}]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "usb %s %s .. .. .. .. .. .. 0000 ffff : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

echo "pci 1050 0840 .... .... ...... : CONFIG__UNKNOWN__ : src/add-ons/kernel/drivers/network/ether/wb840"
echo "pci 11f6 2011 .... .... ...... : CONFIG__UNKNOWN__ : src/add-ons/kernel/drivers/network/ether/wb840"


#aironetwifi atheroswifi
#broadcom43xx
#iaxwifi200 idualwifi7260
#iprowifi2100 iprowifi2200 iprowifi3945 iprowifi4965
#marvell88w8363 marvell88w8335
#ralinkwifi realtekwifi

pciBsdWlanDriver  aironetwifi an an devs reg _pci

#atheroswifi
driverPath=src/add-ons/kernel/drivers/network/wlan/atheroswifi
bsdname=ath
headername=ath
table=pci_id_table
headersuffix=_pci_devlist
sourcesuffix=_pci
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -DPCI_VENDOR_ID_ATHEROS=0x168c -include $TOPDIR/$driverPath/dev/$headername/if_${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^\};$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \)]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

pciBsdWlanDriver  broadcom43xx bwi bwi devices reg _pci

#iaxwifi200
driverPath=src/add-ons/kernel/drivers/network/wlan/iaxwifi200
bsdname=iwx
headername=iwx
table=devices
headersuffix=reg
sourcesuffix=
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/pci/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -D__FreeBSD_version -include $TOPDIR/$driverPath/dev/pci/if_${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^iwx_probe/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \},]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq


#idualwifi7260
driverPath=src/add-ons/kernel/drivers/network/wlan/idualwifi7260
bsdname=iwm
headername=iwm
table=devices
headersuffix=reg
sourcesuffix=
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/pci/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -D__FreeBSD_version -include $TOPDIR/$driverPath/dev/pci/if_${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \}]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq


pciBsdWlanDriver  iprowifi2100 ipw ipw ident_table reg
pciBsdWlanDriver  iprowifi2200 iwi iwi ident_table reg
pciBsdWlanDriver  iprowifi3945 wpi wpi ident_table reg
pciBsdWlanDriver  iprowifi4965 iwn iwn ident_table _devid
pciBsdWlanDriver  marvell88w8335 malo malo products hal _pci
pciBsdWlanDriver  marvell88w8363 mwl mwl pci_ids ioctl _pci


#ralinkwifi
driverPath=src/add-ons/kernel/drivers/network/wlan/ralinkwifi
bsdname=ral
table=pci_ids
sourcesuffix=_pci
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E - | sed -E -n "/${bsdname}_${table}\[/,/^\};$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ ,]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq
bsdname=ural
table=devs
sourcesuffix=
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/usb/wlan/if_${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include objects/common/libs/compat/freebsd_network/usbdevs.h - | sed -E -n "/${bsdname}_${table}\[/,/^\};$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ \)]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "usb %s %s .. .. .. .. .. .. 0000 ffff : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq


#realtekwifi
driverPath=src/add-ons/kernel/drivers/network/wlan/realtekwifi
bsdname=rtwn
headername=rtwn
table=pci_ident_table
headersuffix=_pci_attach
sourcesuffix=_pci_attach
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/pci/${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include $TOPDIR/$driverPath/dev/$headername/pci/${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^$/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ ,]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "pci %s %s .... .... ...... : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq

table=devs
headersuffix=_usb_attach
sourcesuffix=_usb_attach
sed -e 's/#include.*//g' $TOPDIR/$driverPath/dev/$bsdname/usb/${bsdname}${sourcesuffix}.c | awk '/VENDORID_.*,$/  { printf("%s\t", $0); next } 1' | gcc -E -include objects/common/libs/compat/freebsd_network/usbdevs.h -include $TOPDIR/$driverPath/dev/$headername/usb/${headername}${headersuffix}.h - | sed -E -n "/${bsdname}_${table}\[/,/^typedef/p" | sed -e 's/0X/0x/g' | sed -r -e 's/[^0x]*0x([^ ,]+), 0x([^ ,]+).*/\1\t\2/' -e '/^[[:alnum:]]/!d' | awk -F'\t' -v driverPath=$driverPath 'NF > 1 { printf "usb %s %s .. .. .. .. .. .. 0000 ffff : CONFIG__UNKNOWN__ : %s\n", $1, $2, driverPath }' | sort | uniq


exit 0


