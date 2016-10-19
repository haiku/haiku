#!/bin/sh

notice () {
	notify --messageID QC$$ --type information \
		--icon /boot/system/apps/Installer \
		--title "Update" \
		"$@"
	echo "$@"
}

notice "Generating new SSH keys..."

SETTINGSSSHDIR=`finddir B_COMMON_SETTINGS_DIRECTORY`/ssh
hostKeyDir=${SETTINGSSSHDIR}
trash $hostKeyDir/ssh_host_*key*
/system/boot/post-install/sshd_keymaker.sh

notice "Please set your prefered locale…"

Locale

notice "Please set your prefered keymap…"

Keymap

notice "Please set your prefered editor…"

case `alert "Which editor do you want by default?" "Pe" "vim" "nano"` in
	Pe)
		echo 'export EDITOR=lpe' >> ~/.profile
		;;
	vim)
		echo 'export EDITOR=vim' >> ~/.profile
		;;
	nano|*)
		echo 'export EDITOR=nano' >> ~/.profile
		;;
esac


notice "Please fill in your name and email address for git commits…"

EDITOR=lpe git config --global -e

notice "Please fill in your name and email address for packaging…"

lpe ~/config/settings/haikuports.conf
