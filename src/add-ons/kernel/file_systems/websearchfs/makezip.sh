#!/bin/sh

FILES="home/config/settings/kernel/drivers/googlefs home/config/add-ons/kernel/file_systems/googlefs home/config/add-ons/userlandfs/googlefs home/config/bin/ndmount home/config/bin/mountgooglefs home/config/bin/imlucky home/config/bin/google home/Desktop/README.googlefs.txt home/config/settings/Tracker/DefaultQueryTemplates/application_x-vnd.Be-bookmark"

cd /boot
zip -r9 googlefs-test-bin.zip ${FILES}



