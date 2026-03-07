#!/bin/sh

sshd_config_path=/system/settings/ssh/sshd_config
/bin/sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/;' $sshd_config_path
/bin/sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/;' $sshd_config_path
/bin/sed -i 's/#PermitEmptyPasswords no/PermitEmptyPasswords yes/' $sshd_config_path
/bin/passwd -d user
/bin/sshd -D & # sshd fails to start on first boot
