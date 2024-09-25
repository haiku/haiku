#!/bin/bash
#
# For "official" images on https://www.haiku-os.org/guides/virtualizing/google
#
# Making a new Google Compute Engine image
#   * Create a raw disk 4GiB image dd if=/dev/zero of=disk.raw bs=1M count=4096
#   * Boot VM (qemu-system-x86_64 -cdrom (haiku-release.iso) -hda disk.raw -boot d --enable-kvm -m 4G
#     * Partition new disk
#       * 32 MiB EFI System Data. FAT32 named "ESP"
#       * Rest of disk, Haiku, BFS, named "Haiku"
#     * Install Haiku to it new disk
#     * Allow installer to Reboot, *boot again from CD*
#     * Setup EFI bootloader
#       * mount "haiku esp", mount "ESP"
#       * Copy all contents of "haiku esp" to "ESP"
#       * unmount "haiku esp", unmount "ESP"
#     * Mount new Haiku install.  (should mount to /Haiku1)
#     * Run this script (sysprep-gce.sh /Haiku1)
#     * If r1beta4
#       * Manually copy over latest r1beta4 haiku, haiku_devel, haiku_data_translations, haiku_loader
#         * Needed on r1b4 due to / permissions fix needed by sshd
#     * Shutdown VM.  DO NOT BOOT FROM NEW DISK!
#       * Booting from new disk will cause SSH host keys to generate! (#18186)
#   * Compress tar cvzf haiku-r1beta5-x64-v20241024.tar.gz disk.raw
#   * Upload to google cloud storage bucket for the haiku-inc project (ex: haiku-images/r1beta4/xxx)
#     ex: gcloud storage cp ./haiku-r1beta5-x64-v20241024.tar.gz  gs://haiku-images/master/haiku-r1beta5-x64-v20241024.tar.gz
#   * Import image (be sure to update version information below)
#     ex: gcloud compute images create haiku-r1beta5-x64-v20241024 \
#           --project=haiku-inc \
#           --description=Haiku\ R1/Beta5\ x86_64 \
#           --family=haiku-r1beta5-x64 \
#           --source-uri=https://storage.googleapis.com/haiku-images/r1beta5/haiku-r1beta5-x64-v20240924.tar.gz \
#           --labels=os=haiku,release=r1beta5 \
#           --storage-location=us \
#           --architecture=X86_64
#         gcloud compute images add-iam-policy-binding haiku-r1beta4-x64 \
#           --member='allAuthenticatedUsers' --role='roles/compute.imageUser' --project haiku-inc

if [ $# -ne 1 ]; then
		echo "usage: $0 <HAIKU ROOTFS>"
		echo "  example: $0 /Haiku1"
		exit 1;
fi

SMOL_RELEASE="0.1.1-1"
TARGET_ROOTFS="$1"

echo "Preparing $TARGET_ROOTFS for Google Compute Engine..."
echo "WARNING: DO NOT DIRECTLY BOOT FROM THIS HAIKU INSTALL!"
echo ""
echo "Installing basic authentication stuff..."
# Installs gce_metadata_ssh tool for sshd. This lets you control the keys
# of the "user" user from GKE.  ONLY "user" WORKS! We have no PAM for gce's os-login stuff
wget https://eu.hpkg.haiku-os.org/haikuports/master/$(uname -m)/current/packages/smolcloudtools-$SMOL_RELEASE-$(uname -m).hpkg \
	-O $TARGET_ROOTFS/system/packages/smolcloudtools-$SMOL_RELEASE-$(uname -m).hpkg

echo "Configuring ssh..."
# Configure SSHD (reminder, sshd sees "user" as root since it is UID 0)
echo "# For Google Compute Engine" >> $TARGET_ROOTFS/system/settings/ssh/sshd_config
echo "AuthorizedKeysCommand /bin/gce_metadata_ssh" >> $TARGET_ROOTFS/system/settings/ssh/sshd_config
echo "AuthorizedKeysCommandUser user" >> $TARGET_ROOTFS/system/settings/ssh/sshd_config
echo "PasswordAuthentication no" >> $TARGET_ROOTFS/system/settings/ssh/sshd_config
echo "PermitRootLogin without-password" >> $TARGET_ROOTFS/system/settings/ssh/sshd_config

echo "Configuring kernel..."
# GCP likes serial debug data on com0 (helps in troubleshooting)
sed -i "s/^serial_debug_output .*$/serial_debug_output true/g" $TARGET_ROOTFS/home/config/settings/kernel/drivers/kernel
sed -i "s/^serial_debug_port .*$/serial_debug_port 0/g" $TARGET_ROOTFS/home/config/settings/kernel/drivers/kernel

unmount $TARGET_ROOTFS

echo "Complete!  Please shutdown VM. DO NOT BOOT FROM NEW OS IMAGE!"
