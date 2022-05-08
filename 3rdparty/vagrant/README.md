# Haiku Vagrant boxes

> Haiku, Inc. has access to draft a new release at Vagrant

https://app.vagrantup.com/haiku-os

## Creating libvirt box

Create base image:
```qemu-image -f qcow2 libvirt/box.img 8G```

Install Haiku:
```qemu-system-x86_64 --enable-kvm -hda libvirt/box.img -cdrom <haiku.iso> -m 1G```

Boot Haiku:
```qemu-system-x86_64 --enable-kvm -hda libvirt/box.img -m 1G```

Update software:
```
pkgman update
<reboot>
pkgman install vim
```

Install vagrant public key:
```
mkdir ~/config/settings/ssh
wget https://raw.githubusercontent.com/hashicorp/vagrant/main/keys/vagrant.pub -O ~/config/settings/ssh/authorized_keys
chmod 700 ~/config/settings/ssh
chmod 600 ~/config/settings/ssh/authorized_keys
```

Rename "user" to "vagrant" by editing /etc/passwd. Change "user" to "vagrant"

### Packaging libvirt

```
cd libvirt
tar cvzf ../libvirt.box *
```

## Creating VirtualBox box

```
qemu-image convert -f qcow2 -O vmdk libvirt/box.img virtualbox/box-0001.vmdk
cd virtualbox
tar cvzf ../virtualbox.box *
```
